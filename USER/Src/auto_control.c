#include "auto_control.h"
#include "sensor_data.h"
#include "SG90.h"
#include "Motor.h"
#include "tim.h"
#include <stdio.h>
#include "esp8266.h"

/**
 * @brief 检查手动关闭冷却期是否激活
 * @param last_manual_close 上次手动关闭的时间戳
 * @return 1=冷却期中, 0=冷却期已过或时间戳为0
 */
uint8_t IsManualLockActive(uint32_t last_manual_close)
{
    if (last_manual_close == 0) return 0;
    
    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - last_manual_close;
    
    if (elapsed < MANUAL_LOCK_DURATION) return 1;
    return 0;
}

/**
 * @brief 根据温度计算风扇档位
 * @param temp 当前温度
 * @param cfg 系统配置
 * @return 风扇档位 0~4
 * @note 舒适区(≤temp_max): 0档
 *       极端高温(≥temp_max+10): 4档
 *       中间线性映射到1~3档
 */
uint8_t GetFanLevelByTemperature(float temp, SystemConfig_t *cfg)
{
    float comfort_max = cfg->temp_max;
    float danger_temp = cfg->temp_max + 10.0f;
    
    if (temp <= comfort_max) return 0;
    if (temp >= danger_temp) return 4;
    
    float range = danger_temp - comfort_max;
    float ratio = (temp - comfort_max) / range;
    uint8_t level = (uint8_t)(1 + ratio * 3);
    
    if (level > 4) level = 4;
    return level;
}

/**
 * @brief 手动模式下风扇自动匹配档位
 * @param cfg 系统配置
 * @note 根据温度自动调节，最低1档，不会自动关闭
 */
void FanAutoLevel_Manual(SystemConfig_t *cfg)
{
    if (!cfg->fan_auto_level) return;                       // 不是自动匹配模式，跳过
    if (cfg->fan_state == 0) return;                        // 风扇已关闭，跳过
    if (IsManualLockActive(cfg->fan_last_manual_close)) return;  // 冷却期，跳过
    
    float temp = g_sensor_data.temperature;
    uint8_t target_level = GetFanLevelByTemperature(temp, cfg);
    if (target_level == 0) target_level = 1;                // 手动模式下最低1档
    
    uint8_t current_level = cfg->fan_level;
    
    if (target_level != current_level) {
        cfg->fan_level = target_level;
        Fan_SetLevel(target_level);
        SystemConfig_MarkDirty();                            // 标记需要保存
        printf("[AUTO] 风扇自动调节 -> %d档 (温度: %.1f°C)\r\n", target_level, temp);
    }
}

/**
 * @brief 统一自动控制（全自动模式下调用）
 * @param cfg 系统配置
 * @note 风扇/卷帘/补光灯全部自动调节，状态变化时标记需要保存
 */
void AutoControl_All(SystemConfig_t *cfg)
{
    // ========== 风扇自动控制（全自动模式） ==========
    if (cfg->fan_mode == 1 && !IsManualLockActive(cfg->fan_last_manual_close)) {
        
        float temp = g_sensor_data.temperature;
        uint8_t target_level = GetFanLevelByTemperature(temp, cfg);
        uint8_t current_level = cfg->fan_level;

        // 升档：立即执行
        if (target_level > current_level) {
            cfg->fan_level = target_level;
            cfg->fan_state = (target_level > 0) ? 1 : 0;
            Fan_SetLevel(target_level);
            SystemConfig_MarkDirty();
            printf("[AUTO] 风扇升档 -> %d档 (温度: %.1f°C)\r\n", target_level, temp);
            // 立即通知
            EspMessage_t msg;
            msg.type = MSG_UPLOAD_STATUS;
            xQueueSend(espQueueHandle, &msg, 0);
        } 
        // 降档：需要温度低于降档阈值（迟滞控制）
        else if (target_level < current_level) {
            float down_threshold = 0;
            switch (current_level) {
                case 1: down_threshold = cfg->temp_max + 0.0f; break;
                case 2: down_threshold = cfg->temp_max + 2.5f; break;
                case 3: down_threshold = cfg->temp_max + 5.0f; break;
                case 4: down_threshold = cfg->temp_max + 7.5f; break;
                default: break;
            }
            if (temp < down_threshold) {
                cfg->fan_level = target_level;
                cfg->fan_state = (target_level > 0) ? 1 : 0;
                Fan_SetLevel(target_level);
                SystemConfig_MarkDirty();
                printf("[AUTO] 风扇降档 -> %d档 (温度: %.1f°C)\r\n", target_level, temp);
                // 立即通知
                EspMessage_t msg;
                msg.type = MSG_UPLOAD_STATUS;
                xQueueSend(espQueueHandle, &msg, 0);
            }
        }
    }

    // ========== 卷帘自动控制 ==========
    if (cfg->curtain_mode == 1 && !IsManualLockActive(cfg->curtain_last_manual_close)) {
        
        uint16_t light = g_sensor_data.light;
        uint16_t open_threshold = (uint16_t)(cfg->light_max * 1.2f);
        uint16_t close_threshold = (uint16_t)(cfg->light_max * 0.5f);
        
        // 当前关闭：光照超过阈值就打开
        if (cfg->curtain_state == 0 && light > open_threshold) {
            cfg->curtain_state = 1;
            Curtain_On();
            SystemConfig_MarkDirty();
            printf("[AUTO] 卷帘打开 (光照: %d Lux)\r\n", light);
        } 
        // 当前打开：光照低于阈值就关闭
        else if (cfg->curtain_state == 1 && light < close_threshold) {
            cfg->curtain_state = 0;
            Curtain_Off();
            SystemConfig_MarkDirty();
            printf("[AUTO] 卷帘关闭 (光照: %d Lux)\r\n", light);
        }
    }

    // ========== 补光灯自动控制 ==========
    if (cfg->light_mode == 1 && !IsManualLockActive(cfg->light_last_manual_close)) {
        
        uint16_t light = g_sensor_data.light;
        uint16_t open_threshold = (uint16_t)(cfg->light_min * 0.5f);
        uint16_t close_threshold = (uint16_t)(cfg->light_min * 0.8f);
        
        // 当前关闭：光照过低就打开
        if (cfg->light_state == 0 && light < open_threshold) {
            cfg->light_state = 1;
            Light_On();
            SystemConfig_MarkDirty();
            printf("[AUTO] 补光灯打开 (光照: %d Lux)\r\n", light);
        } 
        // 当前打开：光照足够就关闭
        else if (cfg->light_state == 1 && light > close_threshold) {
            cfg->light_state = 0;
            Light_Off();
            SystemConfig_MarkDirty();
            printf("[AUTO] 补光灯关闭 (光照: %d Lux)\r\n", light);
        }
    }
}

// ========== 手动控制函数 ==========

/**
 * @brief 手动设置风扇档位（屏幕/遥控器调用）
 * @param level 档位 0~4，0=关闭
 * @note 关闭时进入10分钟冷却期，状态变化标记保存
 */
void Manual_SetFanLevel(uint8_t level)
{
    if (level > 4) level = 4;
    
    g_sys_config.fan_mode = 0;                              // 手动模式
    g_sys_config.fan_level = level;
    g_sys_config.fan_state = (level > 0) ? 1 : 0;
    g_sys_config.fan_auto_level = 0;                        // 屏幕手动调档，停止自动匹配
    g_sys_config.system_mode = 0;                           // 系统进入手动模式
    
    if (level == 0) {
        // 关闭风扇，记录冷却期
        g_sys_config.fan_last_manual_close = HAL_GetTick();
        printf("[MANUAL] 风扇关闭 (冷却期10分钟)\r\n");
    } else {
        // 打开风扇，清除冷却期
        g_sys_config.fan_last_manual_close = 0;
        printf("[MANUAL] 风扇 -> %d档\r\n", level);
    }
    
    Fan_SetLevel(level);
    // SystemConfig_MarkDirty();                                // 标记需要保存
}

/**
 * @brief 手动关闭风扇（屏幕/遥控器调用）
 * @note 进入10分钟冷却期
 */
void Manual_SetFanOff(void)
{
    Manual_SetFanLevel(0);
}

/**
 * @brief 前端控制风扇开关
 * @param state 1=打开（自动匹配档位）, 0=关闭
 * @note 打开时根据温度自动匹配档位，最低1档
 *       关闭时进入10分钟冷却期
 */
void Remote_SetFan(uint8_t state)
{
    g_sys_config.fan_mode = 0;                              // 手动模式
    g_sys_config.system_mode = 0;                           // 系统进入手动模式
    
    if (state == 1) {
        // 打开风扇：根据温度自动匹配档位，最低1档，开启自动匹配标志
        float temp = g_sensor_data.temperature;
        uint8_t level = GetFanLevelByTemperature(temp, &g_sys_config);
        if (level == 0) level = 1;                          // 最低保持1档
        
        g_sys_config.fan_level = level;
        g_sys_config.fan_state = 1;
        g_sys_config.fan_auto_level = 1;                    // 开启自动匹配
        g_sys_config.fan_last_manual_close = 0;             // 清除冷却期
        Fan_SetLevel(level);
        printf("[REMOTE] 风扇打开 -> %d档 (温度: %.1f°C)\r\n", level, temp);
    } else {
        // 关闭风扇：进入冷却期
        g_sys_config.fan_level = 0;
        g_sys_config.fan_state = 0;
        g_sys_config.fan_auto_level = 0;                    // 关闭自动匹配
        g_sys_config.fan_last_manual_close = HAL_GetTick();
        Fan_SetLevel(0);
        printf("[REMOTE] 风扇关闭 (冷却期10分钟)\r\n");
    }
    
    // SystemConfig_MarkDirty();                                // 标记需要保存
}

/**
 * @brief 手动设置卷帘（屏幕/遥控器/前端通用）
 * @param state 1=打开, 0=关闭
 * @note 关闭时进入10分钟冷却期
 */
void Manual_SetCurtain(uint8_t state)
{
    g_sys_config.curtain_mode = 0;                          // 手动模式
    g_sys_config.curtain_state = state;
    g_sys_config.system_mode = 0;                           // 系统进入手动模式
    
    if (state == 0) {
        g_sys_config.curtain_last_manual_close = HAL_GetTick();
        Curtain_Off();
        printf("[MANUAL] 卷帘关闭 (冷却期10分钟)\r\n");
    } else {
        g_sys_config.curtain_last_manual_close = 0;         // 清除冷却期
        Curtain_On();
        printf("[MANUAL] 卷帘打开\r\n");
    }
    
    // SystemConfig_MarkDirty();                                // 标记需要保存
}

/**
 * @brief 手动设置补光灯（屏幕/遥控器）
 * @param state 1=打开, 0=关闭
 * @note 关闭时进入10分钟冷却期
 */
void Manual_SetLight(uint8_t state)
{
    g_sys_config.light_mode = 0;                            // 手动模式
    g_sys_config.light_state = state;
    g_sys_config.system_mode = 0;                           // 系统进入手动模式
    
    if (state == 0) {
        g_sys_config.light_last_manual_close = HAL_GetTick();
        Light_Off();
        printf("[MANUAL] 补光灯关闭 (冷却期10分钟)\r\n");
    } else {
        g_sys_config.light_last_manual_close = 0;           // 清除冷却期
        Light_On();
        printf("[MANUAL] 补光灯打开\r\n");
    }
    
    // SystemConfig_MarkDirty();                                // 标记需要保存
}

/**
 * @brief 切换到全自动模式
 * @note 立即保存到SD卡，清除所有冷却期，设备交由自动控制
 */
void Manual_SetSystemAuto(void)
{
    g_sys_config.fan_mode = 1;                              // 风扇自动
    g_sys_config.curtain_mode = 1;                          // 卷帘自动
    g_sys_config.light_mode = 1;                            // 补光灯自动
    g_sys_config.system_mode = 1;                           // 全自动模式
    g_sys_config.fan_auto_level = 0;                        // 全自动模式不需要此标志
    
    // 清除所有冷却期标记
    g_sys_config.fan_last_manual_close = 0;
    g_sys_config.curtain_last_manual_close = 0;
    g_sys_config.light_last_manual_close = 0;
    
    // SystemConfig_MarkDirty();       // 不立即保存，等定时器
    printf("[MANUAL] 切换到全自动模式\r\n");
}

/**
 * @brief 切换到手动模式
 * @note 立即保存到SD卡，保持当前设备状态不变
 */
void Manual_SetSystemManual(void)
{
    g_sys_config.fan_mode = 0;                              // 风扇手动
    g_sys_config.curtain_mode = 0;                          // 卷帘手动
    g_sys_config.light_mode = 0;                            // 补光灯手动
    g_sys_config.system_mode = 0;                           // 手动模式
    g_sys_config.fan_auto_level = 0;                        // 默认不自动匹配
    
    // SystemConfig_MarkDirty();       // 不立即保存，等定时器
    printf("[MANUAL] 切换到手动模式\r\n");
}

/**
 * @brief 20:00自动恢复全自动模式
 * @note 如果已经是全自动模式则跳过，立即保存到SD卡
 */
void System_AutoRecovery(void)
{
    if (g_sys_config.system_mode == 1) return;              // 已是全自动，不需要恢复
    
    printf("[RECOVERY] 20:00 自动恢复全自动模式\r\n");
    
    g_sys_config.fan_mode = 1;
    g_sys_config.curtain_mode = 1;
    g_sys_config.light_mode = 1;
    g_sys_config.system_mode = 1;
    g_sys_config.fan_auto_level = 0;
    
    // 清除所有冷却期标记
    g_sys_config.fan_last_manual_close = 0;
    g_sys_config.curtain_last_manual_close = 0;
    g_sys_config.light_last_manual_close = 0;
    
    // SystemConfig_Save();                                     // 立即保存
}
