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
#include "pic.h"
#include "gpio.h"
#include "adc.h"
#include "touch.h"
#include <string.h> 
#include "sd.h"
#include "ss_rtc.h"
#include "SG90.h"
#include "BH1750.h"
#include "Motor.h"
#include "esp8266.h"
#include "sensor_data.h"
#include "auto_control.h"
#include "system_config.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define STORAGE_LATENCY   1800000     // 30分钟 = 1800000ms
#define MUTEX_TIMEOUT     pdMS_TO_TICKS(50)     // 互斥锁信号量获取超时50ms

float adc_history=0;     // 上次ADC采样值，用于亮度调节历史比较
static uint32_t task_watchdog[7]={0};     // 添加任务状态监控
extern const char *weekday_names[];
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
TaskHandle_t defaultTaskHandle = NULL;      // 默认任务（LED闪烁）
TaskHandle_t pwmDACTaskHandle = NULL;       // 自动调节屏幕亮度
TaskHandle_t screenTaskHandle = NULL;       // 屏幕显示
TaskHandle_t touchHandle = NULL;            // 触摸
TaskHandle_t SensorHandle = NULL;           // 数据采集
TaskHandle_t autocontrolHandle = NULL;      // 窗帘控制
TaskHandle_t cloudsyncHandle = NULL;        // 上传前端
TaskHandle_t espTaskHandle = NULL;
TaskHandle_t watchdogTaskHandle = NULL;     // 看门狗监控任务句柄

extern SemaphoreHandle_t MutexHandle = NULL;      // 互斥信号量（保护共享资源）
QueueHandle_t Queueretouch = NULL;                // 队列（存储触摸屏幕次数）
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
void TouchTask(void *argument);             // 触摸任务（检测屏幕触摸切换页面）
void LightControlTask(void *argument);      // 自动控制光亮任务（根据环境光）
void ScreenTask(void *argument);            // 屏幕显示任务
void SensorTask(void *argument);            // 采集数据任务
void AutoControlTask(void *argument);       // 设备控制任务（风扇、舵机、led灯）
void CloudsyncTask(void *argumnet);         // 上传前端任务
void ESP8266_Task(void *argument);
void WatchdogTask(void *argument);          // 看门狗监控任务（监控各任务状态，重启系统）
/* USER CODE END Variables */
// osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

// void StartDefaultTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) 
{
  /* USER CODE BEGIN Init */

// /* 更新数据记录器（内部自动判断是否存储） */
//     data_logger_update(&env_data);
    
//     HAL_Delay(1000);  /* 每秒更新一次 */
    
//     /* 测试：查询某一天数据 */
//     static uint8_t test_query_flag = 0;
//     if (!test_query_flag) {
//         char buffer[2048];
//         test_query_flag = 1;
//         if (data_logger_get_data("2026-07-01", buffer, sizeof(buffer))) {
//             printf("\r\n=== 查询结果 ===\r\n%s\r\n", buffer);
//         } else {
//             printf("无数据\r\n");
//         }
//     }
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  // 创建互斥信号量（保护共享资源）
  MutexHandle = xSemaphoreCreateMutex();
  MutexpHandle = xSemaphoreCreateMutex(); 
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */

  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  // 创建触摸次数数据队列
  Queueretouch=xQueueCreate(5,sizeof(uint8_t));
  // 创建ESP8266操作队列（最多缓存10个消息）
  espQueueHandle = xQueueCreate(10, sizeof(EspMessage_t));
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  SystemConfig_Init();

  // 看门狗监控（优先级6 - 保证能监控所有任务）
  BaseType_t Rewatchdog = xTaskCreate(WatchdogTask,"wtachdog",256,NULL,1,&watchdogTaskHandle);
  if(Rewatchdog == pdFALSE)
    printf("Create watchdog task failed!\r\n");

  // 触摸任务（优先级5）
  BaseType_t Retouch = xTaskCreate(TouchTask,"touch-TFT",256,(void*)NULL,5,&touchHandle);
  if(Retouch == pdFALSE)
    printf("Create touch-TFT task falied! \r\n");
  
  // 屏幕显示（优先级4）
  BaseType_t Rescreen = xTaskCreate(ScreenTask,"screen",1024,NULL,4,&screenTaskHandle);
  if(Rescreen == pdFALSE)
    printf("Create screen task falied!\r\n");
  
  // 传感器采集（优先级3）
  BaseType_t Resensor = xTaskCreate(SensorTask,"Sensor",256,NULL,3,&SensorHandle);
  if(Resensor == pdFALSE)
    printf("Create Sensor task failed!\r\n");

  // 自动亮度控制（优先级3）
  BaseType_t RepwmDAC = xTaskCreate(LightControlTask,"PWM-DAC",256,NULL,3,&pwmDACTaskHandle);
  if(RepwmDAC == pdFALSE)
    printf("Create pwm task failed!\r\n");

  // 设备控制（优先级2）
  BaseType_t Reauto = xTaskCreate(AutoControlTask,"Auto",256,NULL,2,&autocontrolHandle);
  if(Reauto == pdFALSE)
    printf("Create Autocontrol task failed!\r\n");

  // 创建控制服务器任务
  BaseType_t Resesp = xTaskCreate(ESP8266_Task, "esp8266", 512, NULL, 3, &espTaskHandle);
  if(Resesp == pdFALSE)
    printf("Create ESP8266 task failed!\r\n");

  // 云端上传（优先级1）
  BaseType_t Recloud = xTaskCreate(CloudsyncTask,"cloud",1024,NULL,1,&cloudsyncHandle);
  if(Recloud == pdFALSE)
    printf("Creat Cloud task faied!\r\n");
  lcd_showpicture(0, 0, 320, 480, gImage_back);

  // lcd_show_string(10,10,24,"2026-7-5",DARKBLUE);
  // lcd_show_string(140,10,24,"14:10",0x0140);
  // lcd_showchinese(25,70,24,"温度", 0x0180);
  // lcd_show_string(30,110,32,"26.5",DARKBLUE);
  // lcd_show_string(100,200,32,"890",MID_GREEN);
  // lcd_show_string(100,200,32,"890",DARK_GREEN);
  // lcd_show_string(100,200,32,"890",DIM_GREEN);
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */

/* USER CODE END StartDefaultTask */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/**
 * @brief 触摸任务 - 检测屏幕触摸切换页面
 * @param argument 任务参数
 */
void TouchTask(void *argument)
{
  uint32_t last_scan = 0;   // 上次扫描时间戳
  uint8_t touch_state = 0;  // 0:等待触摸, 1:触摸中, 2:已处理
  uint32_t touch_start_time = 0;  // 触摸开始时间戳
    
  for(;;)
  {
    vTaskDelayUntil(&last_scan, pdMS_TO_TICKS(50));
    task_watchdog[0]++;

    tp_dev.scan(0);
      
    // 检测触摸按下（上升沿）
    if ((tp_dev.sta & TP_PRES_DOWN) && touch_state == 0)
    {
      touch_state = 1;  // 进入触摸中状态
      // start_x = tp_dev.x[0];
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

/*
 * @brief 设备控制任务
 * @param argument 无
 * @note 系统自动控制设备运行状态
 */
void AutoControlTask(void *argument)
{
  SS_RTC_Time_t time;
  static uint8_t first_run = 1;
  
  // SD卡初始化
  if (DataLogger_Init() != 1) {
    printf("[CLOUD] SD卡挂载失败\r\n");
    SystemConfig_SetDefaults();
  } else {
    printf("[CLOUD] SD卡挂载成功\n");
  }
    // // 等 ScreenTask 初始化完成
    // vTaskDelay(pdMS_TO_TICKS(3000));
    // SystemConfig_Init();

  while (1)
  {
    task_watchdog[2]++;
    
    // 首次运行：恢复设备状态
    if (first_run) {
      Fan_SetLevel(g_sys_config.fan_level);
      if (g_sys_config.curtain_state) {
        Curtain_On();
      } else {
        Curtain_Off();
      }
      printf("[AUTO] 设备状态已恢复\r\n");
      first_run = 0;
    }
    
    SS_RTC_GetTime(&time);
    if (time.hours == 20 && time.minutes == 0 && time.seconds == 0) {
      System_AutoRecovery();
    }
    AutoControl_All(&g_sys_config);
    
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

/*
 * @brief 数据采集任务
 * @param argument 无
 * @note 1s采集一次
 */
void SensorTask(void *argument)
{
  TickType_t xLastWakeTime = xTaskGetTickCount();

  printf("[Sensor] 采集任务启动, 1s采集一次\r\n");

  while (1)
  {
    // 更新看门狗计数
    task_watchdog[3]++;

    // 采集传感器
    Sensor_Update();         // 读取传感器
    printf("[Sensor] T:%.1f H:%d L:%.1f\r\n", 
           g_sensor_data.temperature,
           g_sensor_data.humidity,
           g_sensor_data.light);

    // 每1秒采集一次
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
  }
}

/*
 * @brief 上传前端任务
 * @param argument 无
 * @note 每10s上传一次数据
 */
void CloudsyncTask(void *argument)
{
  // 等待高优先级任务完成初始化
  vTaskDelay(pdMS_TO_TICKS(3000));
  
  TickType_t xLastWakeTime = xTaskGetTickCount();

  printf("[CLOUD] 云同步任务启动, 2s上报一次\r\n");

  while (1)
  {
    task_watchdog[4]++;
    
    // 存储数据到SD卡
    DataLogger_StoreCurrent();
    
    EspMessage_t msg;
    msg.type = MSG_UPLOAD_DATA;
    xQueueSend(espQueueHandle, &msg, pdMS_TO_TICKS(10));

    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2000));
  }
}

/**
 * @brief ESP8266处理任务
 */
void ESP8266_Task(void *argument)
{
  EspMessage_t msg;
  
  // 调度器已启动，在这里创建
  dma_tx_done = xSemaphoreCreateBinary();
  espQueueHandle = xQueueCreate(10, sizeof(EspMessage_t));

  printf("[ESP] UDP任务启动\r\n");
  
  while(1)
  {
    task_watchdog[6]++;
    
    // 1. 先检查指令（不阻塞，有就处理）
    ESP8266_ProcessRequest();
    
    // 2. 再处理上传（有消息就发）
    if (xQueueReceive(espQueueHandle, &msg, 0) == pdTRUE)
    {
      if (msg.type == MSG_UPLOAD_DATA) 
      {
        upload_sensor_data();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

/**
 * @brief 屏幕显示任务
 * @param argument 任务参数
 */
void ScreenTask(void *argument)
{
  SS_RTC_Time_t rtc_time;
  uint8_t fan_laststate=5;
  uint8_t curtain_laststate=2;
  uint8_t light_laststate=2;
  uint16_t temp_history=0;    // 上次温度采样值，用于温度调节历史比较
  uint16_t humi_history=0;    // 上次湿度采样值，用于湿度调节历史比较
  uint16_t light_history=0;   // 上次光照采样值，用于光照调节历史比较
  char humi_str[8];    // 湿度字符串
  char light_str[16];  // 光照字符串
  char temp_str[16];   // 温度字符串

  // 显示
  lcd_showchinese(105, 116, 24, "℃", DARKBLUE,WHITE);
  lcd_show_string(248,116,24,"%RH",DARKBLUE,WHITE);
  lcd_show_string(182,210,24,"lux",DARKBLUE,WHITE);
  lcd_showchinese(35,78,24,"温度",BLACK,WHITE);
  lcd_showchinese(185, 78, 24, "湿度", BLACK,WHITE);
  lcd_showchinese(120,168,24,"光照",BLACK,WHITE);
  lcd_showchinese(40,330,24,"风扇",0x0140,WHITE);
  lcd_showchinese(135,330,24,"卷帘",0x0140,WHITE);
  lcd_showchinese(222,330,24,"补光灯",0x0140,WHITE);

  for(;;)
  {
    vTaskDelay(pdMS_TO_TICKS(200));

    // 更新看门狗计数
    task_watchdog[5]++;

    SS_RTC_GetTime(&rtc_time);  // 获取当前时间
      
    // 显示时间和日期
    char time_str[16],time_str1[16],data_str[16];
    sprintf(data_str, "%02d-%2d-%2d", rtc_time.year, rtc_time.month, rtc_time.day);
    sprintf(time_str1, "%02d:%02d", rtc_time.hours, rtc_time.minutes);
    sprintf(time_str, "%02d:%02d:%0d", rtc_time.hours, rtc_time.minutes, rtc_time.seconds);
    sprintf(temp_str, "%.1f", g_sensor_data.temperature);
    sprintf(humi_str, "%.lf",(float) g_sensor_data.humidity);
    sprintf(light_str,"%.1f",g_sensor_data.light);
    
    // 加互斥锁保护LCD操作
    if(xSemaphoreTake(MutexHandle, pdMS_TO_TICKS(50)) == pdTRUE)
    {
      lcd_show_string(10,10,24,data_str,0x0140,WHITE);
      update_time(135, 10, 24, time_str1);
      update_time_display(120,450,24,time_str);
      if(temp_history!=g_sensor_data.temperature){
        lcd_show_string(30, 110, 32, temp_str, DARKBLUE,WHITE);
        temp_history=g_sensor_data.temperature;
      }
      if(humi_history!=g_sensor_data.humidity){
        lcd_show_string(200, 110, 32, humi_str, DARKBLUE,WHITE);
        humi_history=g_sensor_data.humidity;
      }
      if(light_history!=g_sensor_data.light){
        lcd_show_string(110, 205, 32, light_str, DARKBLUE,WHITE);
        light_history=g_sensor_data.light;
      }
      xSemaphoreGive(MutexHandle);
    }

    if((fan_laststate!= g_sys_config.fan_state) || (curtain_laststate!= g_sys_config.curtain_state) || (light_laststate !=g_sys_config.light_state))
    {
      if(fan_laststate!=g_sys_config.fan_state)
      {
        if(g_sys_config.fan_state)
        {
          lcd_draw_rectangle(38,390,50,30,RED);
          LCD_Fill(38,390,88,420,RED);
          lcd_showchinese(40,393,24,"关闭",BRRED,RED);
        }
        else{
          lcd_draw_rectangle(38,390,50,30,GREEN);
          LCD_Fill(38,390,88,420,GREEN);
          lcd_showchinese(40,393,24,"打开",0x0180,GREEN);
        }
        fan_laststate=g_sys_config.fan_state;
      }
      if(curtain_laststate!= g_sys_config.curtain_state)
      {
        if(g_sys_config.curtain_state)
        {
          lcd_draw_rectangle(133,390,50,30,RED);
          LCD_Fill(130,390,180,420,RED);
          lcd_showchinese(135,393,24,"关闭",BRRED,RED);
        }
        else{
          lcd_draw_rectangle(132,390,50,30,GREEN);
          LCD_Fill(132,390,182,420,GREEN);
          lcd_showchinese(135,393,24,"打开",0x0180,GREEN);
        }
        curtain_laststate=g_sys_config.curtain_state;
      }
      if(light_laststate != g_sys_config.light_state)
      {
        if(g_sys_config.light_state)
        {
          lcd_draw_rectangle(228,390,50,30,RED);
          LCD_Fill(228,390,278,420,RED);
          lcd_showchinese(230,393,24,"关闭",BRRED,RED);
        }
        else{
          lcd_draw_rectangle(228,390,50,30,GREEN);
          LCD_Fill(228,390,278,420,GREEN);
          lcd_showchinese(230,393,24,"打开",0x0180,GREEN);
        }
        light_laststate = g_sys_config.light_state;
      }
    }
  }
}

/**
 * @brief 看门狗监控任务 - 监控各任务是否卡死
 * @param argument 任务参数
 */
void WatchdogTask(void *argument)
{
  uint32_t last_values[7] = {0};
  uint32_t stuck_count[7] = {0};
  
  // 初始化所有last_values为当前值，避免初始误判
  vTaskDelay(pdMS_TO_TICKS(20000));
  for(int i = 0; i < 7; i++) {
    last_values[i] = task_watchdog[i];  // 初始化基准值
  }
  
  for(;;)
  {
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    int total_stuck = 0;
    
    for(int i = 0; i < 7; i++)
    {
      if(task_watchdog[i] == last_values[i])
      {
        stuck_count[i]++;
        if(stuck_count[i] >= 6) {  // 放宽到30秒
          total_stuck++;
          printf("[Watchdog] Task %d stuck! (%d/6)\n", i, stuck_count[i]);
        }
      }
      else
      {
        stuck_count[i] = 0;  // 只要更新一次就清零
      }
      last_values[i] = task_watchdog[i];
    }
    
    if(total_stuck >= 2) {
      printf("[Watchdog] Multiple tasks stuck! System reset!\n");
      HAL_Delay(100);
      NVIC_SystemReset();
    }
  }
}
/* USER CODE END Application */
