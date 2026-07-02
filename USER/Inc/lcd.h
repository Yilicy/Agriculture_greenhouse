#ifndef __LCD_h_
#define __LCD_h_

#include "main.h"
#include "stm32f4xx.h"

/*LCD重要参数集*/
typedef struct {
    uint16_t width;       //LCD宽度
    uint16_t height;      //LCD高度
    uint16_t id;          //LCD ID
    uint16_t wramcmd;     //开始写gram指令
    uint16_t setxcmd;     //设置x坐标指令
    uint16_t setycmd;    //设置y坐标指令
}_lcd_dev_;
extern _lcd_dev_ lcddev;

/* LCD地址结构体 */ 
typedef struct 
{ 
    volatile uint16_t LCD_REG; 
    volatile uint16_t LCD_RAM; 
} LCD_TypeDef; 

#define FSMC_NEx    4
#define FSMC_Ay     6

/* LCD地址定义 */
#define FSMC_NE4    (0X60000000+(0X04000000*(FSMC_NEx-1)))
#define FSMC_A6     (1<<FSMC_Ay)*2
#define LCD_BASE    (uint32_t)(FSMC_NE4 | (FSMC_A6 -2)) 
#define LCD         ((LCD_TypeDef *) LCD_BASE) 

/*控制引脚宏*/
#define LCD_CS(x)   ((x)?HAL_GPIO_WritePin(LCDCS_GPIO_Port,LCDCS_Pin,GPIO_PIN_SET):HAL_GPIO_WritePin(LCDCS_GPIO_Port,LCDCS_Pin,GPIO_PIN_RESET))
#define LCD_DC(x)   ((x)?HAL_GPIO_WritePin(LCDRS_GPIO_Port,LCDRS_Pin,GPIO_PIN_SET):HAL_GPIO_WritePin(LCDDC_GPIO_Port,LCDDC_Pin,GPIO_PIN_RESET))
#define LCD_RD(x)   ((x)?HAL_GPIO_WritePin(LCDRD_GPIO_Port,LCDRD_Pin,GPIO_PIN_SET):HAL_GPIO_WritePin(LCDRD_GPIO_Port,LCDRD_Pin,GPIO_PIN_RESET))
#define LCD_WR(x)   ((x)?HAL_GPIO_WritePin(LCDWR_GPIO_Port,LCDWR_Pin,GPIO_PIN_SET):HAL_GPIO_WritePin(LCDWR_GPIO_Port,LCDWR_Pin,GPIO_PIN_RESET))
#define LCD_BLK(x)   ((x)?HAL_GPIO_WritePin(LCDBLK_GPIO_Port,LCDBLK_Pin,GPIO_PIN_SET):HAL_GPIO_WritePin(LCDBLK_GPIO_Port,LCDBLK_Pin,GPIO_PIN_RESET))

void lcd_wr_data(uint16_t data);                    /* 写数据 */
void lcd_wr_regno(volatile uint16_t regno);         /* 写寄存器地址 */
void lcd_wr_reg(uint16_t regno, uint16_t data);     /* 写寄存器(地址+数据) */
uint16_t lcd_read_data(void);                          /* 读数据 */
void LCD_WriteRAM_Prepare(void);

void lcd_Init(void);                  /* LCD初始化 */
void lcd_scan_dir(uint8_t dir);       /* 设置LCD的自动扫描方向(对RGB屏无效) */
void lcd_display_dir(void);           /* 设置屏幕显示方向(默认竖屏) */ 

#endif
