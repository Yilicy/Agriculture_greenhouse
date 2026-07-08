#include "stdio.h"
#include "stdlib.h"
#include "lcd.h"
#include "lcd_draw.h"
#include "touch.h"
#include "24cxx.h"
#include "main.h"
#include "FreeRTOS.h"

_m_tp_dev tp_dev =
{
    tp_init,
    tp_scan,
    tp_adjust,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

/**
 * @brief       SPI写数据
 *   @note      向触摸屏IC写入1 byte数据
 * @param       data: 要写入的数据
 * @retval      无
 */
static void tp_write_byte(uint8_t data)
{
    uint8_t count = 0;

    for (count = 0; count < 8; count++)
    {
        if (data & 0x80)    /* 发送1 */
        {
            T_MOSI(1);
        }
        else                /* 发送0 */
        {
            T_MOSI(0);
        }

        data <<= 1;
        T_CLK(0);
        delay_us(1);
        T_CLK(1);           /* 上升沿有效 */
    }
}

/**
 * @brief       SPI读数据
 *   @note      从触摸屏IC读取adc值
 * @param       cmd: 指令
 * @retval      读取到的数据,ADC值(12bit)
 */
static uint16_t tp_read_ad(uint8_t cmd)
{
    uint8_t count = 0;
    uint16_t num = 0;
    uint32_t timeout = 50000;   // 超时计数
    
    T_CLK(0);           /* 先拉低时钟 */
    T_MOSI(0);          /* 拉低数据线 */
    T_CS(0);            /* 选中触摸屏IC */
    tp_write_byte(cmd); /* 发送命令字 */
    delay_us(6);        /* ADS7846的转换时间最长为6us */
    T_CLK(0);
    delay_us(1);
    T_CLK(1);           /* 给1个时钟，清除BUSY */
    delay_us(1);
    T_CLK(0);

    for (count = 0; count < 16; count++)    /* 读出16位数据,只有高12位有效 */
    {
        num <<= 1;
        T_CLK(0);       /* 下降沿有效 */
        delay_us(1);
        T_CLK(1);

        if (T_MISO) num++;
        if (--timeout == 0) break;   // 超时退出
    }

    num >>= 4;          /* 只有高12位有效. */
    T_CS(1);            /* 释放片选 */
    return num;
}

/* 电阻触摸驱动芯片 数据采集 滤波用参数 */
#define TP_READ_TIMES   5       /* 读取次数 */
#define TP_LOST_VAL     1       /* 丢弃值 */

/**
 * @brief       读取一个坐标值(x或者y)
 *   @note      连续读取TP_READ_TIMES次数据,对这些数据升序排列,
 *              然后去掉最低和最高TP_LOST_VAL个数, 取平均值
 *              设置时需满足: TP_READ_TIMES > 2*TP_LOST_VAL 的条件
 *
 * @param       cmd : 指令
 *   @arg       0XD0: 读取X轴坐标(@竖屏状态,横屏状态和Y对调.)
 *   @arg       0X90: 读取Y轴坐标(@竖屏状态,横屏状态和X对调.)
 *
 * @retval      读取到的数据(滤波后的), ADC值(12bit)
 */
static uint16_t tp_read_xoy(uint8_t cmd)
{
    uint16_t i, j;
    uint16_t buf[TP_READ_TIMES];
    uint16_t sum = 0;
    uint16_t temp;

    for (i = 0; i < TP_READ_TIMES; i++)     /* 先读取TP_READ_TIMES次数据 */
    {
        buf[i] = tp_read_ad(cmd);
    }

    for (i = 0; i < TP_READ_TIMES - 1; i++) /* 对数据进行排序 */
    {
        for (j = i + 1; j < TP_READ_TIMES; j++)
        {
            if (buf[i] > buf[j])   /* 升序排列 */
            {
                temp = buf[i];
                buf[i] = buf[j];
                buf[j] = temp;
            }
        }
    }

    sum = 0;

    for (i = TP_LOST_VAL; i < TP_READ_TIMES - TP_LOST_VAL; i++)   /* 去掉两端的丢弃值 */
    {
        sum += buf[i];  /* 累加去掉丢弃值以后的数据. */
    }

    temp = sum / (TP_READ_TIMES - 2 * TP_LOST_VAL); /* 取平均值 */
    return temp;
}

/**
 * @brief       读取x, y坐标
 * @param       x,y: 读取到的坐标值
 * @retval      无
 */
static void tp_read_xy(uint16_t *x, uint16_t *y)
{
    uint16_t xval, yval;

    if (tp_dev.touchtype & 0X01)    /* X,Y方向与屏幕相反 */
    {
        xval = tp_read_xoy(0X90);   /* 读取X轴坐标AD值, 并进行方向变换 */
        yval = tp_read_xoy(0XD0);   /* 读取Y轴坐标AD值 */
    }
    else                            /* X,Y方向与屏幕相同 */
    {
        xval = tp_read_xoy(0XD0);   /* 读取X轴坐标AD值 */
        yval = tp_read_xoy(0X90);   /* 读取Y轴坐标AD值 */
    }

    *x = xval;
    *y = yval;
}

/* 连续两次读取X,Y坐标的数据误差最大允许值 */
#define TP_ERR_RANGE    50      /* 误差范围 */

/**
 * @brief       连续读取2次触摸IC数据, 并滤波
 *   @note      连续2次读取触摸屏IC,且这两次的偏差不能超过ERR_RANGE,满足
 *              条件,则认为读数正确,否则读数错误.该函数能大大提高准确度.
 *
 * @param       x,y: 读取到的坐标值
 * @retval      0, 失败; 1, 成功;
 */
static uint8_t tp_read_xy2(uint16_t *x, uint16_t *y)
{
    uint16_t x1, y1;
    uint16_t x2, y2;

    tp_read_xy(&x1, &y1);   /* 读取第一次数据 */
    tp_read_xy(&x2, &y2);   /* 读取第二次数据 */

    /* 前后两次采样在+-TP_ERR_RANGE内 */
    if (((x2 <= x1 && x1 < x2 + TP_ERR_RANGE) || (x1 <= x2 && x2 < x1 + TP_ERR_RANGE)) &&
            ((y2 <= y1 && y1 < y2 + TP_ERR_RANGE) || (y1 <= y2 && y2 < y1 + TP_ERR_RANGE)))
    {
        *x = (x1 + x2) / 2;
        *y = (y1 + y2) / 2;
        return 1;
    }

    return 0;
}

/**
 * @brief       触摸按键扫描
 * @param       mode: 坐标模式
 *   @arg       0, 屏幕坐标;
 *   @arg       1, 物理坐标(校准等特殊场合用)
 *
 * @retval      0, 触屏无触摸; 1, 触屏有触摸;
 */
static uint8_t tp_scan(uint8_t mode)
{
    if (T_PEN == 0)     /* 有按键按下 */
    {
        if (mode)       /* 读取物理坐标, 无需转换 */
        {
            tp_read_xy2(&tp_dev.x[0], &tp_dev.y[0]);
        }
        else if (tp_read_xy2(&tp_dev.x[0], &tp_dev.y[0]))     /* 读取屏幕坐标, 需要转换 */
        {
            /* 修改后的坐标转换，支持负数比例因子 */
            int16_t x_temp, y_temp;
            
            /* X轴转换 */
            x_temp = (signed short)(tp_dev.x[0] - tp_dev.xc);
            tp_dev.x[0] = (signed short)(x_temp / tp_dev.xfac + lcddev.width / 2);
            
            /* Y轴转换 */
            y_temp = (signed short)(tp_dev.y[0] - tp_dev.yc);
            tp_dev.y[0] = (signed short)(y_temp / tp_dev.yfac + lcddev.height / 2);
            
            /* 边界检查 */
            if(tp_dev.x[0] >= lcddev.width) tp_dev.x[0] = lcddev.width - 1;
            if(tp_dev.y[0] >= lcddev.height) tp_dev.y[0] = lcddev.height - 1;
        }

        if ((tp_dev.sta & TP_PRES_DOWN) == 0)   /* 之前没有被按下 */
        {   
            tp_dev.sta = TP_PRES_DOWN;  /* 按键按下 */
			tp_dev.x[CT_MAX_TOUCH - 1] = tp_dev.x[0];   /* 记录第一次按下时的坐标 */
            tp_dev.y[CT_MAX_TOUCH - 1] = tp_dev.y[0];
        }
    }
    else
    {
        if (tp_dev.sta & TP_PRES_DOWN)      /* 之前是被按下的 */
        {
            tp_dev.sta &= ~TP_PRES_DOWN;    /* 标记按键松开 */
        }
        else     /* 之前就没有被按下 */
        {
            tp_dev.x[CT_MAX_TOUCH - 1] = 0;
            tp_dev.y[CT_MAX_TOUCH - 1] = 0;
            tp_dev.x[0] = 0xFFFF;
            tp_dev.y[0] = 0xFFFF;
        }
    }

    return tp_dev.sta & TP_PRES_DOWN; /* 返回当前的触屏状态 */
}

/* TP_SAVE_ADDR_BASE定义触摸屏校准参数保存在EEPROM里面的位置(起始地址)
 * 占用空间 : 13字节.
 */
#define TP_SAVE_ADDR_BASE   40

/**
 * @brief       保存校准参数
 *   @note      参数保存在EEPROM芯片里面(24C02),起始地址为TP_SAVE_ADDR_BASE.
 *              占用大小为13字节
 * @param       无
 * @retval      无
 */
void tp_save_adjust_data(void)
{
    uint8_t *p = (uint8_t *)&tp_dev.xfac;   /* 指向首地址 */

    /* p指向tp_dev.xfac的地址, p+4则是tp_dev.yfac的地址
     * p+8则是tp_dev.xoff的地址,p+10,则是tp_dev.yoff的地址
     * 总共占用12个字节(4个参数)
     * p+12用于存放标记电阻触摸屏是否校准的数据(0X0A)
     * 往p[12]写入0X0A. 标记已经校准过.
     */
    at24cxx_write(TP_SAVE_ADDR_BASE, p, 12);                /* 保存12个字节数据(xfac,yfac,xc,yc) */
    at24cxx_write_one_byte(TP_SAVE_ADDR_BASE + 12, 0X0A);   /* 保存校准值 */
}

/**
 * @brief       获取保存在EEPROM里面的校准值
 * @param       无
 * @retval      0，获取失败，要重新校准
 *              1，成功获取数据
 */
uint8_t tp_get_adjust_data(void)
{
    uint8_t *p = (uint8_t *)&tp_dev.xfac;
    uint8_t temp = 0;

    /* 由于我们是直接指向tp_dev.xfac地址进行保存的, 读取的时候,将读取出来的数据
     * 写入指向tp_dev.xfac的首地址, 就可以还原写入进去的值, 而不需要理会具体的数
     * 据类型. 此方法适用于各种数据(包括结构体)的保存/读取(包括结构体).
     */
    at24cxx_read(TP_SAVE_ADDR_BASE, p, 12);                 /* 读取12字节数据 */
    temp = at24cxx_read_one_byte(TP_SAVE_ADDR_BASE + 12);   /* 读取校准状态标记 */

    if (temp == 0X0A)
    {
        return 1;
    }

    return 0;
}

/**
 * @brief       触摸屏校准代码
 *   @note      使用五点校准法(具体原理请百度)
 *              本函数得到x轴/y轴比例因子xfac/yfac及物理中心坐标值(xc,yc)等4个参数
 *              我们规定: 物理坐标即AD采集到的坐标值,范围是0~4095.
 *                        逻辑坐标即LCD屏幕的坐标, 范围为LCD屏幕的分辨率.
 * @param       无
 * @retval      无
 */
void tp_adjust(void)
{
    uint16_t pxy[5][2];     /* 物理坐标缓存值 */
    uint8_t  cnt = 0;
    short s1, s2, s3, s4;   /* 4个点的坐标差值 */
    double px, py;          /* X,Y轴物理坐标比例,用于判定是否校准成功 */
    uint16_t outtime = 0;
    cnt = 0;

    tp_dev.sta = 0;         /* 消除触发信号 */

    while (1)               /* 如果连续10秒钟没有按下,则自动退出 */
    {
        tp_dev.scan(1);     /* 扫描物理坐标 */

        if((tp_dev.sta&TP_PRES_DOWN)&&(T_PEN!=0))
		{
            outtime = 0;
            pxy[cnt][0] = tp_dev.x[0];      /* 保存X物理坐标 */
            pxy[cnt][1] = tp_dev.y[0];      /* 保存Y物理坐标 */
            cnt++;

			if(cnt==5)
			{
				s1 = pxy[1][0] - pxy[0][0]; /* 第2个点和第1个点的X轴物理坐标差值(AD值) */
				s3 = pxy[3][0] - pxy[2][0]; /* 第4个点和第3个点的X轴物理坐标差值(AD值) */
				s2 = pxy[3][1] - pxy[1][1]; /* 第4个点和第2个点的Y轴物理坐标差值(AD值) */
				s4 = pxy[2][1] - pxy[0][1]; /* 第3个点和第1个点的Y轴物理坐标差值(AD值) */

				px = (double)s1 / s3;       /* X轴比例因子 */
				py = (double)s2 / s4;       /* Y轴比例因子 */

				if (px < 0) px = -px;       /* 负数改正数 */
				if (py < 0) py = -py;       /* 负数改正数 */

				if (px < 0.95 || px > 1.05 || py < 0.95 || py > 1.05 ||     /* 比例不合格 */
					abs(s1) > 4095 || abs(s2) > 4095 || abs(s3) > 4095 || abs(s4) > 4095 ||  /* 差值不合格, 大于坐标范围 */
					abs(s1) == 0 || abs(s2) == 0 || abs(s3) == 0 || abs(s4) == 0             /* 差值不合格, 等于0 */
				    )
				{
					cnt = 0;
					continue;
				}
											
                tp_dev.xfac = (float)(s1 + s3) / (2 * (lcddev.width - 40));
                tp_dev.yfac = (float)(s2 + s4) / (2 * (lcddev.height - 40));

                tp_dev.xc = pxy[4][0];      /* X轴,物理中心坐标 */
                tp_dev.yc = pxy[4][1];      /* Y轴,物理中心坐标 */

                LCD_Clear(WHITE);   /* 清屏 */
                HAL_Delay(1000);
                tp_save_adjust_data();

                LCD_Clear(WHITE);   /* 清屏 */
                return; /* 校正完成 */
            }
        }

        HAL_Delay(10);
        outtime++;

        if (outtime > 1000)
        {
            tp_get_adjust_data();
            break;
        }
    }
}

/**
 * @brief       触摸屏初始化
 * @param       无
 * @retval      0,没有进行校准
 *              1,进行过校准
 */
uint8_t tp_init(void)
{
    tp_dev.touchtype = 0;    /* 默认设置(电阻屏 & 竖屏) */
    tp_read_xy(&tp_dev.x[0], &tp_dev.y[0]); /* 第一次读取初始化 */
    
    at24cxx_init();         /* 初始化24CXX */

    if (tp_get_adjust_data())
    {
        printf("Found saved calibration data\r\n");
        printf("xfac=%.2f, yfac=%.2f\r\n", tp_dev.xfac, tp_dev.yfac);
        
        /* 检查校准数据是否有效 */
        if(tp_dev.xfac == 0 || tp_dev.yfac == 0)
        {
            printf("Invalid calibration data, recalibrating...\r\n");
            LCD_Clear(WHITE);
            tp_adjust();
            tp_save_adjust_data();
        }
        else
        {
            return 0;  /* 已校准 */
        }
    }
    else                    /* 未校准 */
    {
        printf("No calibration data, start calibration...\r\n");
        LCD_Clear(WHITE);   /* 清屏 */
        tp_adjust();        /* 屏幕校准 */
        tp_save_adjust_data();
    }

    tp_get_adjust_data();
    return 1;
}

/**
 * @brief       触摸画线功能
 * @note        在屏幕上画线，并串口打印起始点和结束点的坐标
 * @param       color: 线条颜色
 * @retval      无
 */
void tp_draw_line(uint16_t color)
{
    uint16_t x_start = 0, y_start = 0;
    uint16_t x_end = 0, y_end = 0;
    uint8_t first_point = 1;  /* 标记是否是第一个点 */
    uint8_t key_pressed = 0;   /* 按键按下标记 */
    
    while(1)
    {
        tp_dev.scan(0);  /* 扫描触摸屏，获取屏幕坐标 */
        
        if(tp_dev.sta & TP_PRES_DOWN)  /* 有触摸按下 */
        {
            if(!key_pressed)  /* 新的一次按下 */
            {
                key_pressed = 1;
                
                if(first_point)  /* 第一个点（线的起始点） */
                {
                    x_start = tp_dev.x[0];
                    y_start = tp_dev.y[0];
                    x_end = x_start;
                    y_end = y_start;
                    first_point = 0;
                    
                    /* 在起始点画一个点标记 */
                    LCD_DrawPoint(x_start, y_start, color);
                    
                    printf("Line start point: X=%d, Y=%d\r\n", x_start, y_start);
                }
                else  /* 不是第一个点，画线到新位置 */
                {
                    x_end = tp_dev.x[0];
                    y_end = tp_dev.y[0];
                    
                    /* 从上一点画线到当前点 */
                    LCD_DrawLine(x_start, y_start, x_end, y_end, color);
                    
                    /* 更新起始点为当前点，继续画线 */
                    x_start = x_end;
                    y_start = y_end;
                }
            }
            else  /* 持续按下状态，更新终点坐标（用于实时画线） */
            {
                /* 如果需要实时画线效果，可以在这里添加代码 */
                /* 这里简单处理，只在移动时更新终点 */
                x_end = tp_dev.x[0];
                y_end = tp_dev.y[0];
            }
        }
        else  /* 触摸松开 */
        {
            if(key_pressed)  /* 之前有按下，现在松开了 */
            {
                key_pressed = 0;
                
                if(!first_point)  /* 画线完成，输出终点坐标 */
                {
                    printf("Line end point: X=%d, Y=%d\r\n", x_end, y_end);
                    printf("Line drawn from (%d,%d) to (%d,%d)\r\n", x_start, y_start, x_end, y_end);
                    first_point = 1;  /* 重置，等待下一次画线 */
                }
            }
        }
        
        HAL_Delay(10);  /* 延时防抖 */
    }
}

/**
 * @brief       触摸画线功能（连续画线版）
 * @note        手指移动时连续画线，松开手指结束当前线段
 * @param       color: 线条颜色
 * @retval      无
 */
void tp_draw_line_continuous(uint16_t color)
{
    uint16_t x_prev = 0, y_prev = 0;
    uint16_t x_curr = 0, y_curr = 0;
    uint8_t is_drawing = 0;
    uint8_t line_started = 0;
    
    while(1)
    {
        tp_dev.scan(0);  /* 扫描触摸屏 */
        
        if(tp_dev.sta & TP_PRES_DOWN)  /* 触摸按下 */
        {
            x_curr = tp_dev.x[0];
            y_curr = tp_dev.y[0];
            
            if(!is_drawing)  /* 刚开始按下 */
            {
                is_drawing = 1;
                line_started = 1;
                x_prev = x_curr;
                y_prev = y_curr;
                
                /* 记录起始点 */
                printf("Drawing started at: X=%d, Y=%d\r\n", x_curr, y_curr);
                LCD_DrawPoint(x_curr, y_curr, color);  /* 画起始点 */
            }
            else  /* 持续触摸，画线 */
            {
                /* 只有位置改变时才画线，避免重复画同一个点 */
                if((x_curr != x_prev) || (y_curr != y_prev))
                {
                    LCD_DrawLine(x_prev, y_prev, x_curr, y_curr, color);
                    x_prev = x_curr;
                    y_prev = y_curr;
                }
            }
        }
        else  /* 触摸松开 */
        {
            if(is_drawing)  /* 刚刚松开 */
            {
                is_drawing = 0;
                if(line_started)
                {
                    printf("Drawing ended at: X=%d, Y=%d\r\n", x_curr, y_curr);
                    line_started = 0;
                }
            }
        }
        
        HAL_Delay(5);  /* 短延时，提高画线流畅度 */
    }
}
