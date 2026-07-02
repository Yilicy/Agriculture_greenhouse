/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "time.h"
#include "queue.h"
#include "math.h"
#include "usart.h"
#include "tim.h"
#include "timers.h" 
#include "dht.h"
#include "lcd.h"
#include "lcd_draw.h"
#include "gpio.h"
#include "adc.h"
#include "touch.h"
#include <string.h> 
#include "sd.h"
#include "ss_rtc.h"
#include "sensor_data.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define STORAGE_LATENCY   1800000     // 30分钟 = 1800000ms
#define MUTEX_TIMEOUT     pdMS_TO_TICKS(50)     // 互斥锁信号量获取超时50ms
float adc_history=0;      // 上次ADC采样值，用于亮度调节历史比较
int flag = 1;
static uint32_t task_watchdog[3]={0};         // 添加任务状态监控
EnvData_t env_data;  /* 环境数据变量 */
extern const char *weekday_names[];
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
TaskHandle_t defaultTaskHandle = NULL;      // 默认任务（LED闪烁）
TaskHandle_t pwmDACTaskHandle = NULL;       // 自动调节屏幕亮度
TaskHandle_t screenTaskHandle = NULL;       // 屏幕显示
TaskHandle_t touchHandle = NULL;            // 触摸任务
TaskHandle_t watchdogTaskHandle = NULL;     // 看门狗监控任务句柄

SemaphoreHandle_t spi_semaphore = NULL;           // SPI完成信号量
extern SemaphoreHandle_t MutexHandle = NULL;      // 互斥信号量（保护共享资源）
TimerHandle_t TimerHandle = NULL;             // 定时器
QueueHandle_t Queueretouch = NULL;                // 队列（存储触摸屏幕次数）
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
void TouchTask(void *argument);           // 触摸任务（检测屏幕触摸切换页面）
void LightControlTask(void *argument);    // 自动控制光亮任务（根据环境光）
void ScreenTask(void *argument);          // 屏幕显示任务
void WatchdogTask(void *argument);        // 看门狗监控任务（监控各任务状态，重启系统）
void SensorData_Task(void *pvParameters); // 传感器数据存储任务
/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/**
 * @brief 定时器回调函数 - 30分钟手动模式自动恢复
 * @param xTimer 定时器句柄
 */
void timer_Callback(TimerHandle_t xTimer)
{
  flag =1;// 30存储一次数据
}
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

	// 测试RTC初始化
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "RTC Initialized\r\n");
	HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);

  HAL_TIM_Base_Start_IT(&htim2); //启动TIM2定时器（用于ADC采样触发）
  // 启动 PWM
  HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_2);
  __HAL_TIM_SetCompare(&htim12, TIM_CHANNEL_2, 125); // 设置亮度 0-255

  // LCD初始化
  lcd_Init();

  // 触摸初始化
  tp_dev.init();

  data_logger_init();  // 初始化数据记录模块
  
/* 初始化环境数据（示例，实际从API获取） */
env_data.temperature = 25.3;
env_data.humidity = 60.5;
env_data.light = 320;
/* 更新数据记录器（内部自动判断是否存储） */
    data_logger_update(&env_data);
    
    HAL_Delay(1000);  /* 每秒更新一次 */
    
    /* 测试：查询某一天数据 */
    static uint8_t test_query_flag = 0;
    if (!test_query_flag) {
        char buffer[2048];
        test_query_flag = 1;
        if (data_logger_get_data("2026-07-01", buffer, sizeof(buffer))) {
            printf("\r\n=== 查询结果 ===\r\n%s\r\n", buffer);
        } else {
            printf("无数据\r\n");
        }
    }
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  // 创建互斥信号量（保护共享资源）
  MutexHandle = xSemaphoreCreateMutex();  
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  TimerHandle = xTimerCreate("storage",pdMS_TO_TICKS(STORAGE_LATENCY),pdTRUE,(void *)0,timer_Callback);
  if(TimerHandle != NULL)
  {
    if(xTimerStart(TimerHandle,pdMS_TO_TICKS(10))==pdPASS)
      printf("Storage latency started (30 minutes)\r\n");
  }
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  // 创建触摸次数数据队列
  Queueretouch=xQueueCreate(5,sizeof(uint8_t));
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  // 触摸任务
  BaseType_t Retouch = xTaskCreate(TouchTask,"touch-TFT",256,(void*)NULL,5,&touchHandle);
  if(Retouch == pdFALSE)
    printf("Create touch-TFT task falied! \r\n");
  else
    printf("Create touch-TFT task success!\r\n");

  // 自动亮度控制任务
  BaseType_t RepwmDAC = xTaskCreate(LightControlTask,"PWM-DAC",256,NULL,4,&pwmDACTaskHandle);
  if(RepwmDAC == pdFALSE)
    printf("Create pwm task failed!\r\n");
  else
    printf("Create pwm task success!\r\n");

  // 创建传感器数据存储任务（优先级最低，不影响其他任务）
  BaseType_t ret = xTaskCreate(SensorData_Task, "SensorData", 512, NULL, 1, NULL );
  if (ret == pdFALSE) {
    printf("Create SensorData task failed!\r\n");
  } else {
    printf("Create SensorData task success!\r\n");
  }
  
  // 屏幕显示任务
  BaseType_t Rescreen = xTaskCreate(ScreenTask,"screen",1024,NULL,2,&screenTaskHandle);
  if(Rescreen == pdFALSE)
    printf("Create screen task falied!\r\n");
  else
    printf("Create screen task success!\r\n");
  
  // 看门狗监控任务
  BaseType_t Rewatchdog = xTaskCreate(WatchdogTask,"wtachdog",256,NULL,1,&watchdogTaskHandle);
  if(Rewatchdog == pdFALSE)
    printf("Create watchdog task failed!\r\n");
  else
    printf("Create watchdog task success!\r\n");
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
//   /* Infinite loop */
//   for(;;)
//   {
//     osDelay(1);
//   }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/**
 * @brief 触摸任务 - 检测屏幕触摸切换页面
 * @param argument 任务参数
 */
void TouchTask(void *argument)
{
  uint32_t last_scan = 0;   // 上次扫描时间戳
  uint16_t start_x = 0;     // 触摸起始坐标
  uint8_t touch_state = 0;  // 0:等待触摸, 1:触摸中, 2:已处理
  uint32_t touch_start_time = 0;  // 触摸开始时间戳
    
  for(;;)
  {
    vTaskDelayUntil(&last_scan, pdMS_TO_TICKS(20));
    task_watchdog[0]++;

    tp_dev.scan(0);
      
    // 检测触摸按下（上升沿）
    if ((tp_dev.sta & TP_PRES_DOWN) && touch_state == 0)
    {
      touch_state = 1;  // 进入触摸中状态
      start_x = tp_dev.x[0];
      touch_start_time = xTaskGetTickCount();
    }
      
    // 触摸中状态：检测滑动
    if (touch_state == 1 && (tp_dev.sta & TP_PRES_DOWN))
    {
      // int16_t delta_x = tp_dev.x[0] - start_x;
      uint32_t elapsed = xTaskGetTickCount() - touch_start_time;
    }
      
    // 检测触摸释放（下降沿）
    if (!(tp_dev.sta & TP_PRES_DOWN) && touch_state == 1)
    {
      touch_state = 0;  // 复位状态
    }
      
    // 超时保护：如果触摸卡住超过1秒，强制复位
    if (touch_state == 1 && (xTaskGetTickCount() - touch_start_time) > pdMS_TO_TICKS(1000))
    {
      printf("Touch timeout, reset state\r\n");
      touch_state = 0;
    }
  }
}

/**
 * @brief 自动亮度控制任务 - 根据环境光传感器调节屏幕亮度
 * @param argument 任务参数
 */
void LightControlTask(void *argument)
{
  uint32_t adc3x;        // ADC采样值（0-4095）
  uint8_t pwmdac_val;    // PWM比较值（0-255）
  float light_val;       // 光照电压值
  
  for(;;)
  {
    vTaskDelay(pdMS_TO_TICKS(100));  // 降低采样频率到100ms
    
    // 更新看门狗计数
    task_watchdog[1]++;
      
    // 读取ADC，添加超时保护
    adc3x = ADC3_result(20);
    light_val = (float)adc3x * (3.3f / 4096.0f);
    
    // 如果光照变化超过阈值，调整亮度
    if(fabsf(light_val - adc_history) >= 0.2f)
    {
      pwmdac_val = (uint8_t)(light_val * (256.0f / 3.3f));
      if(pwmdac_val > 255) pwmdac_val = 255;
      pwmdac_val = 255 - pwmdac_val;  // 反转
      
      // 使用互斥信号量保护PWM设置
      if(xSemaphoreTake(MutexHandle, MUTEX_TIMEOUT) == pdTRUE)
      {
        __HAL_TIM_SetCompare(&htim12, TIM_CHANNEL_2, pwmdac_val);
        adc_history = light_val;
        xSemaphoreGive(MutexHandle);
      }
    }
  }
}

/**
 * @brief 传感器数据存储任务
 * @param pvParameters 无
 * @note 每分钟检查一次，在分钟=00时执行存储
 */
void SensorData_Task(void *pvParameters)
{
    SS_RTC_Time_t time;
    uint8_t last_minute = 255;  // 记录上一分钟的分钟值
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    printf("SensorData_Task 启动\r\n");
    
    while (1)
    {
        // 获取当前时间
        SS_RTC_GetTime(&time);
        
        // // 检查分钟是否为00，且这一分钟还没存过
        // if (time.minutes == 0 && last_minute != 0)
        // {
        //     // 执行存储操作（小时整点存储）
        //     SensorData_DoStorage(&time);
        //     last_minute = 0;
            
        //     // 每天0点执行清理（小时=0且分钟=0时）
        //     if (time.hours == 0 && time.minutes == 0)
        //     {
        //         SensorData_CleanOldFiles();
        //     }
        // }
        // else if (time.minutes != 0)
        // {
        //     last_minute = time.minutes;  // 更新记录
        // }
        
        // 每10秒检查一次（平衡实时性和CPU占用）
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/**
 * @brief 背景切换任务 - 控制LCD显示时间页面或图片页面
 * @param argument 任务参数
 */
void ScreenTask(void *argument)
{
  static uint8_t Last_temp=0;   //上一次的读取的温度
  uint8_t humi=0, temp=0;   //温湿度
  static uint8_t first_draw=1;
  SS_RTC_Time_t rtc_time;
  static uint8_t sd_initialized = 0;  // 添加SD卡初始化标志

  LCD_Clear(BLACK);  // 清屏为黑色

  for(;;)
  {
    vTaskDelay(pdMS_TO_TICKS(200));

    // 更新看门狗计数
    task_watchdog[2]++;

    SS_RTC_GetTime(&rtc_time);  // 获取当前时间
      
    // 2. 显示时间和日期
    char time_str[16];
    sprintf(time_str, "%02d:%02d:%02d", rtc_time.hours, rtc_time.minutes, rtc_time.seconds);
    
    if(xSemaphoreTake(MutexHandle, pdMS_TO_TICKS(100)) == pdTRUE)
    {
      lcd_showchinese(10, 0, 24, weekday_names[rtc_time.weekday % 7], YELLOW,BLACK);  // 显示星期
      if(first_draw){
        lcd_show_string(15,40,64,time_str,YELLOW,BLACK);
      }
      update_time_display(15, 40, 64, time_str);
      xSemaphoreGive(MutexHandle);
    }

    // 首次绘制时显示固定UI元素
    if(first_draw)
    { 
      if(xSemaphoreTake(MutexHandle, pdMS_TO_TICKS(100)) == pdTRUE)
      {
        // 显示星期几的汉字（一周七天）
        LCD_DrawLine(0,130,320,130,GRAY);
        lcd_shownum(0,135,32,rtc_time.month,LIGHTGREEN,BLACK);
        lcd_showchinese(14,135,32,"月",LIGHTGREEN,BLACK);
        lcd_shownum(125,135,32,rtc_time.year,LIGHTGREEN,BLACK);
        lcd_showchinese(190,135,32,"年",LIGHTGREEN,BLACK);
        lcd_draw_hline(0,165,320,GRAY);
        lcd_draw_hline(0,166,320,GRAY);
        lcd_showchinese(1, 170, 32, "一", LIGHTBLUE,BLACK);
        lcd_showchinese(48, 170, 32, "二", LIGHTBLUE,BLACK);
        lcd_showchinese(95, 170, 32, "三", LIGHTBLUE,BLACK);
        lcd_showchinese(142, 170, 32, "四", LIGHTBLUE,BLACK);
        lcd_showchinese(189, 170, 32, "五", LIGHTBLUE,BLACK);
        lcd_showchinese(237, 170, 32, "六", LIGHTBLUE,BLACK);
        lcd_showchinese(284, 170, 32, "日", LIGHTBLUE,BLACK);
        Lender_display(&rtc_time);  // 显示日历
        // 首次读取温湿度并显示
        dht_read_data(&humi, &Last_temp);
        lcd_draw_hline(0,440,320,GRAY);
        lcd_draw_hline(0,441,320,GRAY);
        lcd_showchinese(0, 447, 32, "室内温度", LIGHTGRAY,BLACK);
        lcd_show_string(129, 447, 32, ":", LIGHTGRAY,BLACK);
        lcd_shownum(162, 447, 32, Last_temp, WHITE,BLACK);
        lcd_showchinese(195, 450, 32, "℃", WHITE,BLACK);
        xSemaphoreGive(MutexHandle);
      }
      first_draw = 0;  // 清除首次绘制标志
    } 
      
    // 温湿度更新（每10秒检查一次）
    static uint32_t last_dht_read = 0;
    if(xTaskGetTickCount() - last_dht_read >= pdMS_TO_TICKS(10000))
    {
      last_dht_read = xTaskGetTickCount();
      
      dht_read_data(&humi, &temp);
      if(temp != Last_temp && temp != 0)  // 避免读取出错
      {
        Last_temp = temp;
        if(xSemaphoreTake(MutexHandle, MUTEX_TIMEOUT) == pdTRUE)
        {
          lcd_shownum(162, 447, 32, temp, WHITE, BLACK);
          xSemaphoreGive(MutexHandle);
        }
      }
    }
    if(rtc_time.minutes == 0|| rtc_time.minutes == 30)
    {
      printf("30");
    }
  }
}

/**
 * @brief 看门狗监控任务 - 监控各任务是否卡死
 * @param argument 任务参数
 */
void WatchdogTask(void *argument)
{
  uint32_t last_values[2] = {0};
  uint32_t stuck_count = 0;
  
  printf("[Watchdog] 看门狗监控任务启动\n");
  
  for(;;)
  {
    vTaskDelay(pdMS_TO_TICKS(10000));  // 每10秒检查一次
    
    for(int i = 0; i < 2; i++)
    {
      if(task_watchdog[i] == last_values[i])
      {
        stuck_count++;
        printf("[Watchdog] Task %d stuck! (no change for 10s)\n", i);
        
        if(stuck_count >= 2) {
          printf("[Watchdog] System reset!\n");
          NVIC_SystemReset();
        }
      }
      else
      {
        stuck_count = 0;
      }
      last_values[i] = task_watchdog[i];
    }
  }
}
/* USER CODE END Application */
