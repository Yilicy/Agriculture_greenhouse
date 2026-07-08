#include "dht.h"
#include "main.h"
#include "stm32f4xx_hal_gpio.h"
#include "usart.h"

//DQ输入模式
void DHT_MODE_Input(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOG_CLK_ENABLE();

  GPIO_InitStruct.Pin = DHT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(DHT_GPIO_Port, &GPIO_InitStruct);
} 

//DQ输出模式
void DHT_MODE_Output(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOG_CLK_ENABLE();

  GPIO_InitStruct.Pin = DHT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(DHT_GPIO_Port, &GPIO_InitStruct);
}

//等待总线数据变化
int8_t DHT11_Wait(uint8_t mode)
{
  uint16_t timeout = 0xffff;    //超时时间
  while(HAL_GPIO_ReadPin(DHT_GPIO_Port,DHT_Pin) == mode && timeout--){    
    delay_us(1);
  }
  if(timeout == 0){
    return -1;
  }
  else{
    return 0;
  }
}

//主机（起始信号）
void DHT_Start(void)
{
  DHT_MODE_Output();
  DHT_DQ(0);
  HAL_Delay(20); //主机拉低保持（18~30ms)
  
  //释放总线前先输出高电平(保持10~20us)
  DHT_DQ(1);
  delay_us(30);    // 确保高电平稳定
}     

//从机（响应信号）
//返回1：未检测到DHT11的存在
//返回0：存在
uint8_t DHT_Response(void)
{
  uint8_t retry=0;

  //从机响应信号，等待DHT11拉低
  while( HAL_GPIO_ReadPin(DHT_GPIO_Port, DHT_Pin)==1 && retry<100 ){
    delay_us(1);
    retry++;
  }
  if(retry>=100) return 1; //超时

  //从机确认应答，输出数据（低电平保持81~85us）
  retry=0;
  while (HAL_GPIO_ReadPin(DHT_GPIO_Port, DHT_Pin)==0 &&retry<100)
  {
    delay_us(1);
    retry++;
  }
  if(retry>=100) return 2;

  //从机拉高（高电平保持85~88us）
  retry=0;
  while(HAL_GPIO_ReadPin(DHT_GPIO_Port, DHT_Pin)==1&&retry<100)
  {
    delay_us(1);
    retry++;
  }
  if(retry>=100) return 3;

  return 0;
}

//读取数据（湿度+温度）
void dht_read_data(uint8_t *humi,float *temp)
{ 
  uint8_t buf[5];
  static uint8_t last_humi=0;
  static uint8_t last_temp=0;

  DHT_Start();          //发出起始信号
  DHT_MODE_Input();     //IO方向：输入

  if(DHT_Response()!=0){
    printf("error2 %d\r\n",DHT_Response());
  }

  for(int i = 0;i < 5;i++)       //接收温湿度数据
  {
    uint8_t rx_data = 0;       //定义一个变量，用于接收DHT11发送的数据
    for(int j = 0;j < 8;j++)
    {
      if(DHT11_Wait(0) != 0){    //等待数据’0’，‘1’低电平时间结束
        printf("error3 \r\n");  
      }

      delay_us(40);   //延时40us后，读取总线数据
      rx_data <<= 1;     //读取的数据高位在前
      if(HAL_GPIO_ReadPin(DHT_GPIO_Port,DHT_Pin)== 1)
      {
        rx_data |= 0x01;
        if(DHT11_Wait(1) != 0){     //等待数据‘1’剩余高电平时间结束
          printf("error4 \r\n");
        }                       
      }
    }
    buf[i] = rx_data;      //保存数据到缓冲区
  }
  uint8_t sum = buf[0] + buf[1] + buf[2] + buf[3];    //校验数据

  if(sum == buf[4]){
    last_humi = buf[0];
    last_temp = (float)buf[2] + (float)buf[3] / 10.0f;
  }
  *humi = last_humi;
  *temp = last_temp;
}
