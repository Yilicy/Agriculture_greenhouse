#include "lcd.h"
#include "lcd_ex.h"

_lcd_dev_ lcddev;

/**
 * @brief  写数据到LCD
 * @param  data: 要写入的数据
 * @retval 无
 */
void lcd_wr_data(uint16_t data)
{
    data=data; /* 使用-O2优化的时候,必须插入的延时 */ 
    LCD->LCD_RAM=data;
}

/**
 * @brief  写寄存器地址到LCD
 * @param  regno: 寄存器地址
 * @retval 无
 */
void lcd_wr_regno(uint16_t regno)
{
    regno=regno;
    LCD->LCD_REG = regno;
}

/**
 * @brief  写寄存器（地址+数据）
 * @param  regno: 寄存器地址
 * @param  data: 要写入的数据
 * @retval 无
 */
void lcd_wr_reg(uint16_t regno, uint16_t data)
{
    LCD->LCD_REG = regno;
    LCD->LCD_RAM = data;
}

/**
 * @brief  从LCD读取数据
 * @param  无
 * @retval 读取到的数据
 */
uint16_t lcd_read_data(void)
{
    volatile uint16_t data;
    delay_us(2);  // 必要的延时，等待数据稳定
    data = LCD->LCD_RAM;
    return data;
}

/**
 * @brief  准备写入GRAM
 * @param  无
 * @retval 无
 */
void LCD_WriteRAM_Prepare(void)
{
    LCD->LCD_REG=lcddev.wramcmd;
}

/**
 * @brief  LCD初始化
 * @param  无
 * @retval 无
 */
void lcd_Init(void)
{
    HAL_Delay(100); //延时确保fsmc成功建立

    // 执行LCD初始化序列
    lcd_reginit();

    // 设置LCD参数
    lcd_display_dir();
    
    // 打开背光
    LCD_BLK(1);

    printf("lcd init success\r\n");
}

/**
 * @brief  设置屏幕显示方向（默认竖屏）
 * @param  无
 * @retval 无
 */
void lcd_display_dir(void)
{
    lcddev.width = 320;
    lcddev.height = 480;
    lcddev.setxcmd = 0x2A;  // 设置X坐标指令
    lcddev.setycmd = 0x2B;  // 设置Y坐标指令
    lcddev.wramcmd = 0x2C;  // 开始写GRAM指令

    lcd_scan_dir(0);
}

/**
 * @brief       设置LCD的自动扫描方向(对RGB屏无效)
 * @param       dir:0~7,代表8个方向
 * @retval      无
 */
void lcd_scan_dir(uint8_t dir)
{
    uint16_t regval = 0x00;  
    uint16_t dirreg = 0x36;

    lcd_wr_reg(dirreg,regval);

    lcd_wr_regno(lcddev.setxcmd);
    lcd_wr_data(0);
    lcd_wr_data(0);
    lcd_wr_data((lcddev.width-1)>>8);
    lcd_wr_data((lcddev.width-1)&0xFF);
    lcd_wr_regno(lcddev.setycmd);
    lcd_wr_data(0);
    lcd_wr_data(0);
    lcd_wr_data((lcddev.height - 1) >> 8);
    lcd_wr_data((lcddev.height - 1) & 0xFF);
}
