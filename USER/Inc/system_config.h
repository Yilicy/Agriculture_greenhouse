#ifndef __SYSTEM_CONFIG_H
#define __SYSTEM_CONFIG_H

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "timers.h"

typedef struct {
    // 用户设置（需要保存）
    float temp_min;
    float temp_max;
    uint16_t light_min;
    uint16_t light_max;
    uint8_t fan_mode;           // 1=自动, 0=手动
    uint8_t curtain_mode;       // 1=自动, 0=手动
    uint8_t light_mode;         // 1=自动, 0=手动
    uint32_t fan_last_manual_close;
    uint32_t curtain_last_manual_close;
    uint32_t light_last_manual_close;
    uint8_t system_mode;        // 1=全自动, 0=手动
    
    // 设备状态（需要保存）
    uint8_t fan_level;          // 风扇档位 0~4
    uint8_t fan_state;          // 风扇开关 0=关, 1=开
    uint8_t fan_auto_level;     // 手动模式下风扇自动匹配：1=自动, 0=屏幕设定
    uint8_t curtain_state;      // 卷帘 0=关, 1=开
    uint8_t light_state;        // 补光灯 0=关, 1=开
} SystemConfig_t;

extern SystemConfig_t g_sys_config;
extern uint8_t g_config_initialized;

void SystemConfig_SetDefaults(void);
void SystemConfig_Load(void);
void SystemConfig_Save(void);
void SystemConfig_Init(void);
void SystemConfig_Print(void);
void SystemConfig_MarkDirty(void);
void SystemConfig_StartAutoSaveTimer(void);
void SystemConfig_CheckSave(void);

#endif
