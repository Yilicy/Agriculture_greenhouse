#include "remote.h"
#include "tim.h"

uint8_t g_remote_sta=0;   // 远程控制状态：0=关闭，1=开启
uint32_t g_remote_data=0; // 红外接收到的数据
uint8_t g_remote_cnt=0;   // 按键按下的次数

/** 
 * @brief       定时器输入捕获中断回调函数 
 * @param       htim:定时器句柄 
 * @retval      无 
 */ 
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) 
{ 
  if (htim->Instance == TIM1) 
  { 
    uint16_t dval;   // 下降沿时计数器的值
      
    if (RDATA)  // 上升沿捕获
    { 
      __HAL_TIM_SET_CAPTUREPOLARITY(&htim1,TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);   // 设置为下降沿捕获 
      __HAL_TIM_SET_COUNTER(&htim1, 0);    // 清空定时器计数器值
      g_remote_sta |= 0X10;         // 标记上升沿已经被捕获
    } 
    else   // 下降沿捕获 
    {   
        dval = HAL_TIM_ReadCapturedValue(&htim1, TIM_CHANNEL_1);    // 读取CCR1也可以清CC1IF标志位
        __HAL_TIM_SET_CAPTUREPOLARITY(&htim1, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING); // 配置TIM1通道1上升沿捕获 

        if (g_remote_sta & 0X10)    // 完成一次高电平捕获
        { 
          if (g_remote_sta & 0X80)       // 接收到了引导码
          { 
            if (dval > 300 && dval < 800)  // 560为标准值,560us
            { 
              g_remote_data >>= 1;              // 右移一位
              g_remote_data &= ~0x80000000;     // 接收到0
            } 
            else if (dval > 1400 && dval < 1800)   /* 1680为标准值,1680us */ 
            { 
              g_remote_data >>= 1;            // 右移一位
              g_remote_data |= 0x80000000;    // 接收到1
            } 
            else if (dval > 2000 && dval < 3000) 
            {   /* 得到按键键值增加的信息 2250为标准值2.25ms */ 
              g_remote_cnt++;          // 按键次数增加1次
              g_remote_sta &= 0XF0;    // 清空计时器
            }
          } 
          else if (dval > 4200 && dval < 4700)    // 4500为标准值4.5ms
          { 
            g_remote_sta |= 1 << 7;     // 标记成功接收到了引导码 
            g_remote_cnt = 0;           // 清除按键次数计数器 
          } 
        } 
      g_remote_sta &= ~(1 << 4); 
    } 
  } 
} 

/** 
 * @brief       定时器溢出中断服务函数 
 * @param       htim:定时器句柄 
 * @retval      无 
 */ 
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) 
{ 
  if (htim->Instance == TIM1) 
  { 
    if (g_remote_sta & 0x80)        /* 上次有数据被接收到了 */ 
    { 
      g_remote_sta &= ~0X10;      /* 取消上升沿已经被捕获标记 */ 

      if ((g_remote_sta & 0X0F) == 0X00) 
      { 
        g_remote_sta |= 1 << 6;   /* 标记已经完成一次按键的键值信息采集 */ 
      } 
        
      if ((g_remote_sta & 0X0F) < 14) 
      { 
        g_remote_sta++; 
      } 
      else 
      { 
        g_remote_sta &= ~(1 << 7);   /* 清空引导标识 */ 
        g_remote_sta &= 0XF0;         /* 清空计数器 */ 
      } 
    } 
  } 
} 

/**
 * @brief       处理红外按键(类似按键扫描)
 * @param       无
 * @retval      0   , 没有任何按键按下
 *              其他, 按下的按键键值
 */
uint8_t remote_scan(void)
{
    uint8_t sta = 0;
    uint8_t t1, t2;

    if (g_remote_sta & (1 << 6))                        /* 得到一个按键的所有信息了 */
    {
        t1 = g_remote_data;                             /* 得到地址码 */
        t2 = (g_remote_data >> 8) & 0xff;               /* 得到地址反码 */

        if ((t1 == (uint8_t)~t2) && t1 == 0)    /* 检验遥控识别码(ID)及地址 */
        {
            t1 = (g_remote_data >> 16) & 0xff;
            t2 = (g_remote_data >> 24) & 0xff;

            if (t1 == (uint8_t)~t2)
            {
                sta = t1;                               /* 键值正确 */
            }
        }

        if ((sta == 0) || ((g_remote_sta & 0x80) == 0)) /* 按键数据错误/遥控已经没有按下了 */
        {
            g_remote_sta &= ~(1 << 6);                  /* 清除接收到有效按键标识 */
            g_remote_cnt = 0;                           /* 清除按键次数计数器 */
        }
    }

    return sta;
}

/** 
 * @brief       判断红外按键值 
 * @param       key: 按键值 
 * @retval      1   , 按键被按下 
 *              0   , 按键未被按下 
 */ 
char* jdge_remote_key(void) 
{ 
  uint8_t key;
  key = remote_scan();
  char *str = "FF";

  if (key)
  {
    switch (key)
    {
      case 0:
          str = "ERROR";
          break;

      case 69:
          str = "POWER";
          break;

      case 70:
          str = "UP";
          break;

      case 64:
          str = "PLAY";
          break;

      case 71:
          str = "ALIENTEK";
          break;

      case 67:
          str = "RIGHT";
          break;

      case 68:
          str = "LEFT";
          break;

      case 7:
          str = "VOL-";
          break;

      case 21:
          str = "DOWN";
          break;

      case 9:
          str = "VOL+";
          break;

      case 22:
          str = "1";
          break;

      case 25:
          str = "2";
          break;

      case 13:
          str = "3";
          break;

      case 12:
          str = "4";
          break;

      case 24:
          str = "5";
          break;

      case 94:
          str = "6";
          break;

      case 8:
          str = "7";
          break;

      case 28:
          str = "8";
          break;

      case 90:
          str = "9";
          break;

      case 66:
          str = "0";
          break;

      case 74:
          str = "DELETE";
          break;
    }
  }
  return str;
}
