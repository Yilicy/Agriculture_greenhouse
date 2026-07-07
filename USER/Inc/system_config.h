#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"

// ===================== 系统配置结构体 =====================
typedef struct {
    // 用户配置
    float temp_min;              // 适宜温度下限 (°C)
    float temp_max;              // 适宜温度上限 (°C)
    uint16_t light_min;          // 适宜光照下限 (Lux)
    uint16_t light_max;          // 适宜光照上限 (Lux)
    
    // 设备状态（系统自动控制）
    uint8_t fan_mode;            // 0=自动, 1=手动
    uint8_t fan_level;           // 当前风扇档位 0~4
    uint8_t fan_state;           // 0 = 关闭, 1 = 打开
    uint32_t fan_last_manual_close;  // 上次手动关闭时间戳
    
    uint8_t curtain_mode;        // 0=自动, 1=手动
    uint8_t curtain_state;       // 0=关闭, 1=打开
    uint32_t curtain_last_manual_close;
    
    uint8_t light_mode;          // 0=自动, 1=手动
    uint8_t light_state;         // 补光灯 0=关闭, 1=打开
    uint32_t light_last_manual_close;
    
    uint8_t system_mode;         // 0=全自动, 1=混合模式
} SystemConfig_t;

// 全局变量声明
extern SystemConfig_t g_sys_config;
extern uint8_t g_config_initialized;
extern SemaphoreHandle_t MutexpHandle;

// 函数声明
void SystemConfig_Init(void);
void SystemConfig_Load(void);
void SystemConfig_Save(void);
void SystemConfig_Print(void);
void SystemConfig_SetDefaults(void);

#endif
