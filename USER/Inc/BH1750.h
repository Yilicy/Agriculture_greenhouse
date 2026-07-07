// #ifndef BH1750_H_
// #define BH1750_H_

// #include "main.h"

// #define IIC_ADDRESS 0x23 // BH1750 I2C地址
// #define IIC_SCL(X)      HAL_GPIO_WritePin(I2C_SCL_GPIO_Port, I2C_SCL_Pin, (X) ? GPIO_PIN_SET : GPIO_PIN_RESET)     // SCL引脚
// #define IIC_SDA(X)      HAL_GPIO_WritePin(I2C_SDA_GPIO_Port, I2C_SDA_Pin, (X) ? GPIO_PIN_SET : GPIO_PIN_RESET)     // SDA引脚
// #define READ_SDA        HAL_GPIO_ReadPin(I2C_SDA_GPIO_Port, I2C_SDA_Pin)     // 读取SDA引脚状态

// #define BHAddWrite     0x46      // 从机地址+最后写方向位
// #define BHAddRead      0x47      // 从机地址+最后读方向位
// #define BHPowDown      0x00      // 关闭模块
// #define BHPowOn        0x01      // 打开模块等待测量指令
// #define BHReset        0x07      // 重置数据寄存器值在PowerOn模式下有效
// #define BHModeH1       0x10      // 高分辨率 单位1lx 测量时间120ms
// #define BHModeH2       0x11      // 高分辨率模式2 单位0.5lx 测量时间120ms
// #define BHModeL        0x13      // 低分辨率 单位4lx 测量时间16ms
// #define BHSigModeH     0x20      // 一次高分辨率 测量 测量后模块转到 PowerDown模式
// #define BHSigModeH2    0x21      // 一次高分辨率模式2 测量 测量后模块转到 PowerDown模式
// #define BHSigModeL     0x23      // 一次低分辨率 测量 测量后模块转到 PowerDown模式

// // 函数声明
// void BH1750_Init(void);
// void BH1750_IIC_Start(void);
// void BH1750_IIC_Stop(void);
// void BH1750_SendByte(uint8_t data);
// uint8_t BH1750_ReadByte(unsigned char ack);
// uint8_t BH1750_WaitAck(void);
// void BH1750_Ack(void);
// void BH1750_NAck(void);
 
// float BH1750_ReadLight(void);
// void bh_data_send(uint8_t command);

// void i2c_scan(void);

// #endif /* BH1750_H_ */

#ifndef __BH1750_H
#define __BH1750_H

#include "stm32f4xx.h"

// BH1750 设备地址（7位地址 0x23，左移一位得到 8位写/读地址）
#define BHAddWrite  0x46   // 0x23 << 1
#define BHAddRead   0x47   // (0x23 << 1) | 0x01

// 常用指令
#define BHPowOn     0x01   // 上电
#define BHReset     0x07   // 复位
#define BHModeH1    0x10   // 高分辨率模式 1 (1lx, 120ms)
#define BHModeH2    0x11   // 高分辨率模式 2 (0.5lx, 120ms)
#define BHModeL     0x13   // 低分辨率模式 (4lx, 16ms)

void BH1750_Init(void);
void bh_data_send(uint8_t cmd);
float BH1750_ReadLight(void);
void i2c_scan(void);

#endif
