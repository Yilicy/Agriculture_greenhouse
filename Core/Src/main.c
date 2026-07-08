/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "rtc.h"
#include "sdio.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "fsmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include "lcd_draw.h"
#include "touch.h"
#include "esp8266.h"
#include "remote.h"
#include "sensor_data.h"
#include <string.h>
#include "cloud_sync.h"
#include "esp8266.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define WIFI_SSID         "hahaha"
#define WIFI_PASSWORD     "dong00407"
#define SERVER_IP         "10.17.102.190"
#define SERVER_PORT       5000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// printf重定义
int fputc(int ch, FILE *f)
{
	HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 10);//0xffff
	return ch;
}

/*微秒延时函数*/
void delay_us(uint32_t us)
{
  __HAL_TIM_SET_COUNTER(&htim3, 0);  // 重置计数器
  HAL_TIM_Base_Start(&htim3);  // 启动定时器
  while (__HAL_TIM_GET_COUNTER(&htim3) < us);  // 等待计数完成
  HAL_TIM_Base_Stop(&htim3);  // 停止定时器
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM12_Init();
  MX_ADC3_Init();
  MX_FSMC_Init();
  MX_SDIO_SD_Init();
  MX_FATFS_Init();
  MX_RTC_Init();
  MX_USART3_UART_Init();
  MX_TIM1_Init();
  MX_I2C1_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  // 启动 PWM
  HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_2);
  __HAL_TIM_SetCompare(&htim12, TIM_CHANNEL_2, 125); // 设置亮度 0-255

  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1); // 启动定时器4的PWM输出，通道1用于控制直流电机
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1); // 启动定时器2的PWM输出，通道1用于控制SG90舵机
  __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, 150); // 初始化舵机角度为90度

  // LCD初始化
  lcd_Init();

  // 触摸初始化
  tp_dev.init();

  // 光照传感器初始化
  BH1750_Init();

  ESP8266_SendCmd("AT+RST\r\n", "OK", 3000);
  HAL_Delay(1500);
  ESP8266_Init(WIFI_SSID, WIFI_PASSWORD);

  // 启动服务器
  ESP8266_SendCmd("AT+CIPSERVER=1,80\r\n", "OK", 2000);
  printf("服务器已启动, IP: 10.17.102.247\r\n");

    // int count = 0;
    // while (1)
    // {
    //     // ===== 1. 检查是否有队友指令 =====
    // if (USART3_RX_STA & 0x8000)
    // {
    //   char *p = strstr(USART3_RX_BUF, "+IPD,");
    //     if (p && strstr(p, "GET /control"))
    // {
    //     printf("\r\n===== 收到指令 =====\r\n");
        
    //     // 1. 解析 link_id
    //     int link_id = 0;
    //     sscanf(p, "+IPD,%d,", &link_id);
        
    //     // 2. 构造响应体 (Body)
    //     char body[] = "{\"status\":\"ok\"}";
        
    //     // 3. 构造完整的 HTTP 响应头 + 响应体
    //     // 注意：这里必须包含 Content-Length，且换行符必须是 \r\n
    //     char http_response[256];
    //     int body_len = strlen(body);
        
    //     sprintf(http_response, 
    //         "HTTP/1.1 200 OK\r\n"
    //         "Content-Type: application/json\r\n"
    //         "Content-Length: %d\r\n" 
    //         "Connection: close\r\n"
    //         "\r\n"             // 空行，分隔头部和主体
    //         "%s",              // 放入 Body
    //         body_len, body
    //     );

    //     // 4. 计算准确的发送长度 (不包含字符串结尾的 \0)
    //     int total_len = strlen(http_response);

    //     // 5. 发送 AT 指令告知长度
    //     char cmd[64];
    //     sprintf(cmd, "AT+CIPSEND=%d,%d\r\n", link_id, total_len);
    //     HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 1000);
        
    //     // 【关键修改】增加延时，等待模块准备就绪 (建议 200ms 以上)
    //     HAL_Delay(300); 
        
    //     // 6. 发送实际的 HTTP 数据
    //     HAL_UART_Transmit(&huart3, (uint8_t*)http_response, total_len, 1000);
        
    //     // 7. 等待数据发送完毕，再关闭连接 (防止数据被截断)
    //     HAL_Delay(200); 
        
    //     // 8. 关闭连接
    //     sprintf(cmd, "AT+CIPCLOSE=%d\r\n", link_id);
    //     HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 1000);
        
    //     printf("已回复 200 OK (Len:%d)\r\n", total_len);
    //     printf("==================\r\n\r\n");
    // }
    //     USART3_RX_STA = 0;
    //     memset(USART3_RX_BUF, 0, RX_BUFFER_SIZE);
    // }
      
    //   // ===== 2. 上传数据（用ID 4）=====
    //   ESP8266_SendCmd("AT+CIPCLOSE=4\r\n", NULL, 500);
    //   HAL_Delay(200);
      
    //   char cmd[128];
    //   sprintf(cmd, "AT+CIPSTART=4,\"TCP\",\"%s\",%d\r\n", SERVER_IP, SERVER_PORT);
    //   if (ESP8266_SendCmd(cmd, "OK", 5000) == ESP_OK)
    //   {
    //       char json[64];
    //       sprintf(json, "{\"temp\":30.3,\"humid\":64,\"light\":18.4}");
          
    //       char http_req[512];
    //       sprintf(http_req,
    //           "POST /api/hardware/init HTTP/1.1\r\n"
    //           "Host: %s:%d\r\n"
    //           "Content-Type: application/json\r\n"
    //           "Content-Length: %d\r\n"
    //           "Connection: close\r\n"
    //           "\r\n"
    //           "%s",
    //           SERVER_IP, SERVER_PORT, (int)strlen(json), json);
          
    //       int len = strlen(http_req);
    //       sprintf(cmd, "AT+CIPSEND=4,%d\r\n", len);
          
    //       USART3_RX_STA = 0;
    //       memset(USART3_RX_BUF, 0, RX_BUFFER_SIZE);
    //       HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 1000);
          
    //       uint32_t t = HAL_GetTick();
    //       while (HAL_GetTick() - t < 2000)
    //       {
    //           if (USART3_RX_STA & 0x8000 && strstr(USART3_RX_BUF, ">")) break;
    //           HAL_Delay(10);
    //       }
          
    //       HAL_UART_Transmit(&huart3, (uint8_t*)http_req, len, 3000);
    //       HAL_Delay(200);
          
    //       printf("上传%d: %s\r\n", count, json);
          
    //       ESP8266_SendCmd("AT+CIPCLOSE=4\r\n", NULL, 500);
    //   }
      
    //   count++;
    //   HAL_Delay(2000);
    // }
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
