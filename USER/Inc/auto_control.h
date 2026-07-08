#ifndef __AUTO_CONTROL_H
#define __AUTO_CONTROL_H

#include "system_config.h"

// 冷却期时长：10分钟
#define MANUAL_LOCK_DURATION  600000

/**
 * @brief 检查手动关闭冷却期是否激活
 * @param last_manual_close 上次手动关闭的时间戳
 * @return 1=冷却期中, 0=冷却期已过
 */
uint8_t IsManualLockActive(uint32_t last_manual_close);

/**
 * @brief 根据温度计算风扇档位
 * @param temp 当前温度
 * @param cfg 系统配置
 * @return 风扇档位 0~4
 */
uint8_t GetFanLevelByTemperature(float temp, SystemConfig_t *cfg);

/**
 * @brief 统一自动控制（全自动模式下调用）
 * @param cfg 系统配置
 */
void AutoControl_All(SystemConfig_t *cfg);

// ========== 手动控制函数 ==========
/**
 * @brief 手动模式下风扇自动匹配档位
 * @param cfg 系统配置
 * @note 根据温度自动调节，最低1档，不会自动关闭
 */
void FanAutoLevel_Manual(SystemConfig_t *cfg);

/**
 * @brief 手动设置风扇档位（屏幕/遥控器调用）
 * @param level 档位 0~4，0=关闭
 */
void Manual_SetFanLevel(uint8_t level);

/**
 * @brief 手动关闭风扇（屏幕/遥控器调用）
 */
void Manual_SetFanOff(void);

/**
 * @brief 前端控制风扇开关
 * @param state 1=打开（自动匹配档位）, 0=关闭
 */
void Remote_SetFan(uint8_t state);

/**
 * @brief 手动设置卷帘（屏幕/遥控器/前端通用）
 * @param state 1=打开, 0=关闭
 */
void Manual_SetCurtain(uint8_t state);

/**
 * @brief 手动设置补光灯（屏幕/遥控器）
 * @param state 1=打开, 0=关闭
 */
void Manual_SetLight(uint8_t state);

/**
 * @brief 切换到全自动模式
 */
void Manual_SetSystemAuto(void);

/**
 * @brief 切换到手动模式
 */
void Manual_SetSystemManual(void);

/**
 * @brief 20:00自动恢复全自动模式
 */
void System_AutoRecovery(void);

#endif
