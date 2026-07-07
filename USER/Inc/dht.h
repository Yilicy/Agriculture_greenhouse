#ifndef __DHT_H_
#define __DHT_H_

#include "stm32f4xx.h"
#include "main.h"

//GPIO置位
#define DHT_DQ(x) do{(x)?HAL_GPIO_WritePin(DHT_GPIO_Port,DHT_Pin,GPIO_PIN_SET):HAL_GPIO_WritePin(DHT_GPIO_Port,DHT_Pin,GPIO_PIN_RESET);}while(0)

void DHT_MODE_Input(void);   //DQ输入模式
void DHT_MODE_Output(void);  //DQ输出模式

void DHT_Start(void);        //主机（起始信号）
uint8_t DHT_Response(void);  //从机（响应信号）
int8_t DHT11_Wait(uint8_t mode);
void dht_read_data(uint8_t *humi,float *temp); //读取数据（湿度+温度）

#endif
