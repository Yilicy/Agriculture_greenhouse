// #ifndef AUTO_CONTROL_H
// #define AUTO_CONTROL_H

// #include "system_config.h"

// // 冷却期配置
// #define MANUAL_LOCK_DURATION  1800000  // 30分钟（毫秒）

// // 函数声明
// uint8_t GetFanLevelByTemperature(float temp, SystemConfig_t *cfg);
// uint8_t IsManualLockActive(uint32_t last_manual_close);

// void FanAutoControl(SystemConfig_t *cfg);
// void CurtainAutoControl(SystemConfig_t *cfg);
// void LightAutoControl(SystemConfig_t *cfg);
// void AutoControl_All(SystemConfig_t *cfg);

// // 手动控制接口
// void Manual_SetFanLevel(uint8_t level);
// void Manual_SetFanOff(void);
// void Manual_SetCurtain(uint8_t state);
// void Manual_SetLight(uint8_t state);
// void Manual_SetSystemAuto(void);

// // 20:00 自动恢复
// void System_AutoRecovery(void);

// #endif
#ifndef __AUTO_CONTROL_H
#define __AUTO_CONTROL_H

#include "system_config.h"

#define MANUAL_LOCK_DURATION  1800000  // 30分钟冷却期

uint8_t IsManualLockActive(uint32_t last_manual_close);
uint8_t GetFanLevelByTemperature(float temp, SystemConfig_t *cfg);
void AutoControl_All(SystemConfig_t *cfg);

// 手动控制函数
void Manual_SetFanLevel(uint8_t level);
void Manual_SetFanOff(void);
void Manual_SetCurtain(uint8_t state);
void Manual_SetLight(uint8_t state);
void Manual_SetSystemAuto(void);
void System_AutoRecovery(void);

#endif
