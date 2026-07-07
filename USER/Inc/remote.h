#ifndef __REMOTE_H
#define __REMOTE_H

#include "stm32f4xx.h"

#define RDATA HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8)   // 红外接收引脚

extern uint8_t g_remote_cnt;   // 按键按下的次数

uint8_t remote_scan(void);
char* jdge_remote_key(void);

#endif 
