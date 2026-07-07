// #include "auto_control.h"
// #include "sensor_data.h"
// #include "SG90.h"
// #include "Motor.h"
// #include "tim.h"
// #include <stdio.h>

// // 冷却期检查
// uint8_t IsManualLockActive(uint32_t last_manual_close)
// {
//     // 如果时间戳为 0，说明没有冷却期
//     if (last_manual_close == 0) {
//         return 0;
//     }
    
//     uint32_t now = HAL_GetTick();
//     uint32_t elapsed = now - last_manual_close;
    
//     if (elapsed < MANUAL_LOCK_DURATION) {  // 30分钟
//         return 1;  // 还在冷却期
//     }
    
//     return 0;  // 冷却期已过
// }

// // 统一的自动控制函数
// void AutoControl_All(SystemConfig_t *cfg)
// {
//     uint8_t need_save = 0;
    
//     // ========== 风扇 ==========
//     if (cfg->fan_mode == 0) {
//         if (!(cfg->fan_level == 0 && IsManualLockActive(cfg->fan_last_manual_close))) {
            
//             float temp = g_sensor_data.temperature;
//             uint8_t target_level = GetFanLevelByTemperature(temp, cfg);
//             uint8_t current_level = cfg->fan_level;

//             if (target_level > current_level) {
//                 cfg->fan_level = target_level;
//                 Fan_SetLevel(target_level);
//                 need_save = 1;
//                 printf("[AUTO] 风扇升档 -> %d档 (温度: %.1f°C)\r\n", target_level, temp);
//             } 
//             else if (target_level < current_level) {
//                 float down_threshold = 0;
//                 switch (current_level) {
//                     case 1: down_threshold = cfg->temp_max + 0.0f; break;
//                     case 2: down_threshold = cfg->temp_max + 2.5f; break;
//                     case 3: down_threshold = cfg->temp_max + 5.0f; break;
//                     case 4: down_threshold = cfg->temp_max + 7.5f; break;
//                 }
//                 if (temp < down_threshold) {
//                     cfg->fan_level = target_level;
//                     Fan_SetLevel(target_level);
//                     need_save = 1;
//                     printf("[AUTO] 风扇降档 -> %d档\r\n", target_level);
//                 }
//             }
//         }
//     }

//     // ========== 卷帘 ==========
//     if (cfg->curtain_mode == 0) {
//         if (!(cfg->curtain_state == 0 && IsManualLockActive(cfg->curtain_last_manual_close))) {
            
//             uint16_t light = g_sensor_data.light;
//             uint16_t open_threshold = (uint16_t)(cfg->light_max * 1.2f);
//             uint16_t close_threshold = (uint16_t)(cfg->light_max * 0.5f);
            
//             if (cfg->curtain_state == 0 && light > open_threshold) {
//                 cfg->curtain_state = 1;
//                 Curtain_On();
//                 need_save = 1;
//                 printf("[AUTO] 卷帘打开\r\n");
//             } else if (cfg->curtain_state == 1 && light < close_threshold) {
//                 cfg->curtain_state = 0;
//                 Curtain_Off();
//                 need_save = 1;
//                 printf("[AUTO] 卷帘关闭\r\n");
//             }
//         }
//     }

//     // ========== 补光灯 ==========
//     if (cfg->light_mode == 0) {
//         if (!(cfg->light_state == 0 && IsManualLockActive(cfg->light_last_manual_close))) {
            
//             uint16_t light = g_sensor_data.light;
//             uint16_t open_threshold = (uint16_t)(cfg->light_min * 0.5f);
//             uint16_t close_threshold = (uint16_t)(cfg->light_min * 0.8f);
            
//             if (cfg->light_state == 0 && light < open_threshold) {
//                 cfg->light_state = 1;
//                 need_save = 1;
//                 printf("[AUTO] 补光灯打开\r\n");
//             } else if (cfg->light_state == 1 && light > close_threshold) {
//                 cfg->light_state = 0;
//                 need_save = 1;
//                 printf("[AUTO] 补光灯关闭\r\n");
//             }
//         }
//     }

//     // 统一保存
//     if (need_save) {
//         SystemConfig_Save();
//         printf("[CFG] 配置已保存\r\n");
//     }
// }

// // 风扇档位计算
// uint8_t GetFanLevelByTemperature(float temp, SystemConfig_t *cfg)
// {
//     // 舒适区上限
//     float comfort_max = cfg->temp_max;
//     // 危险区上限 = 舒适区上限 + 10°C
//     float danger_temp = cfg->temp_max + 10.0f;
    
//     // 在舒适区或以下：风扇不转
//     if (temp <= comfort_max) return 0;
    
//     // 极端高温：全速
//     if (temp >= danger_temp) return 4;
    
//     // 线性映射：comfort_max ~ danger_temp → 1~4档
//     float range = danger_temp - comfort_max;        // 10°C
//     float ratio = (temp - comfort_max) / range;     // 0~1
//     uint8_t level = (uint8_t)(1 + ratio * 3);       // 1~4档
    
//     if (level > 4) level = 4;
//     return level;
// }

// // 风扇自动控制
// void FanAutoControl(SystemConfig_t *cfg)
// {
//     // 手动模式：跳过自动控制
//     if (cfg->fan_mode == 1) return;
    
//     printf("[Fan] check lock\r\n");

//     // 冷却期检查：用户手动关闭后30分钟内不自动打开
//     if (cfg->fan_level == 0 && IsManualLockActive(cfg->fan_last_manual_close)) {
//         printf("[AUTO] 风扇冷却期，系统不接管\n");
//         return;
//     }

//     float temp = g_sensor_data.temperature;
//     uint8_t target_level = GetFanLevelByTemperature(temp, cfg);
//     uint8_t current_level = cfg->fan_level;

//     // 迟滞逻辑：升档立即执行，降档需要降温
//     if (target_level > current_level) {
//         // 升档：立即执行
//         cfg->fan_level = target_level;
//         Fan_SetLevel(target_level);
    
//         printf("[Fan] before SystemConfig_Save\r\n");  // ← 加这行
//         SystemConfig_Save();
//         printf("[Fan] after SystemConfig_Save\r\n");   // ← 加这行
//         printf("[AUTO] 风扇升档 -> %d档 (温度: %.1f°C)\r\n", target_level, temp);
//     } 
//     else if (target_level < current_level) {
//         // 降档：需要温度低于当前档位的"降档阈值"(每个档位的降档阈值 = 该档位的升档温度 - 1.5°C)
//         float down_threshold = 0;
        
//         // 根据当前档位计算降档阈值
//         switch (current_level) {
//             case 1: down_threshold = cfg->temp_max + 0.0f; break;      // 1档降回0档：低于 temp_max
//             case 2: down_threshold = cfg->temp_max + 2.5f; break;      // 2档降回1档：低于 temp_max + 2.5°C
//             case 3: down_threshold = cfg->temp_max + 5.0f; break;      // 3档降回2档：低于 temp_max + 5°C
//             case 4: down_threshold = cfg->temp_max + 7.5f; break;      // 4档降回3档：低于 temp_max + 7.5°C
//             default: return;
//         }
        
//         // 温度低于降档阈值才执行降档
//         if (temp < down_threshold) {
//             cfg->fan_level = target_level;
//             Fan_SetLevel(target_level);
//             SystemConfig_Save();
//             printf("[AUTO] 风扇降档 -> %d档 (温度: %.1f°C, 阈值: <%.1f°C)\r\n", target_level, temp, down_threshold);
//         } else {
//             printf("[AUTO] 风扇保持 %d档 (温度: %.1f°C, 需降到 %.1f°C以下才降档)\r\n", current_level, temp, down_threshold);
//         }
//     }
// }

// // 卷帘自动控制
// void CurtainAutoControl(SystemConfig_t *cfg)
// {
//     // 手动模式：跳过
//     if (cfg->curtain_mode == 1) return;
    
//     // 冷却期检查
//     if (cfg->curtain_state == 0 && IsManualLockActive(cfg->curtain_last_manual_close)) {
//         printf("[AUTO] 卷帘冷却期，系统不接管\n");
//         return;
//     }

//     uint16_t light = g_sensor_data.light;
//     uint8_t new_state = cfg->curtain_state;
    
//     uint16_t open_threshold = (uint16_t)(cfg->light_max * 1.2f);    // 卷帘打开阈值 = 适宜光照上限 × 1.2
//     uint16_t close_threshold = (uint16_t)(cfg->light_max * 0.5f);   // 卷帘关闭阈值 = 适宜光照上限 × 0.5（加大迟滞区间）
    
//     // 当前关闭：光照超过上限×1.2 (打开)
//     if (cfg->curtain_state == 0) {
//         if (light > open_threshold) {
//             new_state = 1;
//             Curtain_On();
//             SystemConfig_Save();
//             printf("[AUTO] 卷帘打开 (光照: %d Lux > %d)\r\n", light, open_threshold);
//         }
//     } 
//     // 当前打开：光照低于上限×0.5 (关闭)
//     else {
//         if (light < close_threshold) {
//             new_state = 0;
//             Curtain_Off();
//             SystemConfig_Save();
//             printf("[AUTO] 卷帘关闭 (光照: %d Lux < %d)\r\n", light, close_threshold);
//         }
//     }
    
//     // 状态变化时保存
//     if (new_state != cfg->curtain_state) {
//         cfg->curtain_state = new_state;
//         SystemConfig_Save();
//     }
// }

// // 补光灯自动控制
// void LightAutoControl(SystemConfig_t *cfg)
// {
//     // 手动模式：跳过
//     if (cfg->light_mode == 1) {
//         return;
//     }

//     // 冷却期检查
//     if (cfg->light_state == 0 && IsManualLockActive(cfg->light_last_manual_close)) {
//         printf("[AUTO] 补光灯冷却期，系统不接管\n");
//         return;
//     }
    
//     uint16_t light = g_sensor_data.light;
//     uint8_t new_state = cfg->light_state;
    
//     uint16_t open_threshold = (uint16_t)(cfg->light_min * 0.5f); // 补光灯打开阈值 = 适宜光照下限 × 0.5
//     uint16_t close_threshold = (uint16_t)(cfg->light_min * 0.8f); // 补光灯关闭阈值 = 适宜光照下限 × 0.8
    
//     // 当前关闭：光照低于下限×0.5 (打开)
//     if (cfg->light_state == 0) {
//         if (light < open_threshold) {
//             new_state = 1;
//             // Light_On();
//         }
//     }
//     // 当前打开：光照高于下限×0.8 (关闭)
//     else {
//         if (light > close_threshold) {
//             new_state = 0;
//            // Light_Off();
//         }
//     }
    
//     // 状态变化时保存
//     if (new_state != cfg->light_state) {
//         cfg->light_state = new_state;
//         SystemConfig_Save();
//         printf("[AUTO] 补光灯 -> %s (光照: %d Lux, 阈值: 开<%d, 关>%d)\r\n",
//                new_state ? "开" : "关", light, open_threshold, close_threshold);
//     }
// }

// // 手动设置风扇档位（屏幕/遥控器调用）
// void Manual_SetFanLevel(uint8_t level)
// {
//     if (level > 4) level = 4;
//     g_sys_config.fan_mode = 1;      // 手动模式
//     g_sys_config.fan_level = level;
//     // 清除冷却期标记（用户主动操作，不需要冷却）
//     g_sys_config.fan_last_manual_close = 0;
//     Fan_SetLevel(level);
//     SystemConfig_Save();
//     // 更新系统模式
//     g_sys_config.system_mode = 1;
//     printf("[MANUAL] 风扇 -> %d档\n", level);
// }

// // 手动关闭风扇（屏幕/遥控器/前端调用）
// void Manual_SetFanOff(void)
// {
//     g_sys_config.fan_mode = 1;      // 手动模式
//     g_sys_config.fan_level = 0;
//     // 记录关闭时间，进入冷却期
//     g_sys_config.fan_last_manual_close = HAL_GetTick();
//     Fan_SetLevel(0);
//     SystemConfig_Save();
//     g_sys_config.system_mode = 1;
//     printf("[MANUAL] 风扇关闭 (冷却期30分钟)\n");
// }

// // 手动设置卷帘
// void Manual_SetCurtain(uint8_t state)
// {
//     g_sys_config.curtain_mode = 1;
//     g_sys_config.curtain_state = state;
//     if (state == 0) {
//         g_sys_config.curtain_last_manual_close = HAL_GetTick();
//         Curtain_Off();
//         printf("[MANUAL] 卷帘关闭 (冷却期30分钟)\n");
//     } else {
//         g_sys_config.curtain_last_manual_close = 0;  // 打开不需要冷却
//         Curtain_On();
//         printf("[MANUAL] 卷帘打开\n");
//     }
//     SystemConfig_Save();
//     g_sys_config.system_mode = 1;
// }

// // 手动设置补光灯
// void Manual_SetLight(uint8_t state)
// {
//     g_sys_config.light_mode = 1;
//     g_sys_config.light_state = state;
//     if (state == 0) {
//         g_sys_config.light_last_manual_close = HAL_GetTick();
//         // Light_Off();
//         printf("[MANUAL] 补光灯关闭 (冷却期30分钟)\n");
//     } else {
//         g_sys_config.light_last_manual_close = 0;
//        // Light_On();
//         printf("[MANUAL] 补光灯打开\n");
//     }
//     SystemConfig_Save();
//     g_sys_config.system_mode = 1;
// }

// // 用户主动切换回系统自动
// void Manual_SetSystemAuto(void)
// {
//     g_sys_config.fan_mode = 0;
//     g_sys_config.curtain_mode = 0;
//     g_sys_config.light_mode = 0;
//     g_sys_config.system_mode = 0;
//     // 清除所有冷却期标记
//     g_sys_config.fan_last_manual_close = 0;
//     g_sys_config.curtain_last_manual_close = 0;
//     g_sys_config.light_last_manual_close = 0;
//     SystemConfig_Save();
//     printf("[MANUAL] 用户切换到系统自动模式\n");
// }

// // 20:00 自动恢复
// void System_AutoRecovery(void)
// {
//     if (g_sys_config.system_mode == 0) {
//         return;  // 已经是全自动模式，不需要恢复
//     }
    
//     printf("[RECOVERY] 20:00 自动恢复系统自动模式\n");
//     g_sys_config.fan_mode = 0;
//     g_sys_config.curtain_mode = 0;
//     g_sys_config.light_mode = 0;
//     g_sys_config.system_mode = 0;
//     // 清除所有冷却期标记
//     g_sys_config.fan_last_manual_close = 0;
//     g_sys_config.curtain_last_manual_close = 0;
//     g_sys_config.light_last_manual_close = 0;
//     SystemConfig_Save();
// }
#include "auto_control.h"
#include "sensor_data.h"
#include "SG90.h"
#include "Motor.h"
#include "tim.h"
#include <stdio.h>

uint8_t IsManualLockActive(uint32_t last_manual_close)
{
    if (last_manual_close == 0) return 0;
    
    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - last_manual_close;
    
    if (elapsed < MANUAL_LOCK_DURATION) return 1;
    return 0;
}

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

void AutoControl_All(SystemConfig_t *cfg)
{
    // ========== 风扇自动控制 ==========
    if (cfg->fan_mode == 1) {
        if (!(cfg->fan_level == 0 && IsManualLockActive(cfg->fan_last_manual_close))) {
            
            float temp = g_sensor_data.temperature;
            uint8_t target_level = GetFanLevelByTemperature(temp, cfg);
            uint8_t current_level = cfg->fan_level;

            if (target_level > current_level) {
                cfg->fan_level = target_level;
                cfg->fan_state = 1;  // 风扇打开
                Fan_SetLevel(target_level);
                printf("[AUTO] 风扇升档 -> %d档 (温度: %.1f°C)\r\n", target_level, temp);
            } 
            else if (target_level < current_level) {
                float down_threshold = 0;
                switch (current_level) {
                    case 1: down_threshold = cfg->temp_max + 0.0f; break;
                    case 2: down_threshold = cfg->temp_max + 2.5f; break;
                    case 3: down_threshold = cfg->temp_max + 5.0f; break;
                    case 4: down_threshold = cfg->temp_max + 7.5f; break;
                }
                if (temp < down_threshold) {
                    cfg->fan_level = target_level;
                    if(target_level == 0)
                        cfg->fan_state = 0;
                    Fan_SetLevel(target_level);
                    printf("[AUTO] 风扇降档 -> %d档 (温度: %.1f°C)\r\n", target_level, temp);
                }
            }
        }
    }

    // ========== 卷帘自动控制 ==========
    if (cfg->curtain_mode == 1) {
        if (!(cfg->curtain_state == 0 && IsManualLockActive(cfg->curtain_last_manual_close))) {
            
            uint16_t light = g_sensor_data.light;
            uint16_t open_threshold = (uint16_t)(cfg->light_max * 1.2f);
            uint16_t close_threshold = (uint16_t)(cfg->light_max * 0.5f);
            
            if (cfg->curtain_state == 0 && light > open_threshold) {
                cfg->curtain_state = 1;
                Curtain_On();
                printf("[AUTO] 卷帘打开 (光照: %d Lux)\r\n", light);
            } else if (cfg->curtain_state == 1 && light < close_threshold) {
                cfg->curtain_state = 0;
                Curtain_Off();
                printf("[AUTO] 卷帘关闭 (光照: %d Lux)\r\n", light);
            }
        }
    }

    // ========== 补光灯自动控制 ==========
    if (cfg->light_mode == 1) {
        if (!(cfg->light_state == 0 && IsManualLockActive(cfg->light_last_manual_close))) {
            
            uint16_t light = g_sensor_data.light;
            uint16_t open_threshold = (uint16_t)(cfg->light_min * 0.5f);
            uint16_t close_threshold = (uint16_t)(cfg->light_min * 0.8f);
            
            if (cfg->light_state == 0 && light < open_threshold) {
                cfg->light_state = 1;
                printf("[AUTO] 补光灯打开 (光照: %d Lux)\r\n", light);
            } else if (cfg->light_state == 1 && light > close_threshold) {
                cfg->light_state = 0;
                printf("[AUTO] 补光灯关闭 (光照: %d Lux)\r\n", light);
            }
        }
    }
}

void Manual_SetFanLevel(uint8_t level)
{
    if (level > 4) level = 4;
    g_sys_config.fan_mode = 0;
    g_sys_config.fan_level = level;
    g_sys_config.fan_state = (level > 0) ? 1 : 0;  // 根据档位设置开关状态
    g_sys_config.fan_last_manual_close = 0;
    Fan_SetLevel(level);
    g_sys_config.system_mode = 0;
    SystemConfig_Save();
    printf("[MANUAL] 风扇 -> %d档\r\n", level);
}

void Manual_SetFanOff(void)
{
    g_sys_config.fan_mode = 0;
    g_sys_config.fan_level = 0;
    g_sys_config.fan_state = 0;  // 关闭
    g_sys_config.fan_last_manual_close = HAL_GetTick();
    Fan_SetLevel(0);
    g_sys_config.system_mode = 0;
    SystemConfig_Save();
    printf("[MANUAL] 风扇关闭 (冷却期30分钟)\r\n");
}

void Manual_SetCurtain(uint8_t state)
{
    g_sys_config.curtain_mode = 0;
    g_sys_config.curtain_state = state;
    if (state == 0) {
        g_sys_config.curtain_last_manual_close = HAL_GetTick();
        Curtain_Off();
        printf("[MANUAL] 卷帘关闭 (冷却期30分钟)\r\n");
    } else {
        g_sys_config.curtain_last_manual_close = 0;
        Curtain_On();
        printf("[MANUAL] 卷帘打开\r\n");
    }
    g_sys_config.system_mode = 0;
    SystemConfig_Save();
}

void Manual_SetLight(uint8_t state)
{
    g_sys_config.light_mode = 0;
    g_sys_config.light_state = state;
    if (state == 0) {
        g_sys_config.light_last_manual_close = HAL_GetTick();
        printf("[MANUAL] 补光灯关闭 (冷却期30分钟)\r\n");
    } else {
        g_sys_config.light_last_manual_close = 0;
        printf("[MANUAL] 补光灯打开\r\n");
    }
    g_sys_config.system_mode = 0;
    SystemConfig_Save();
}

void Manual_SetSystemAuto(void)
{
    g_sys_config.fan_mode = 1;
    g_sys_config.curtain_mode = 1;
    g_sys_config.light_mode = 1;
    g_sys_config.system_mode = 1;
    g_sys_config.fan_last_manual_close = 0;
    g_sys_config.curtain_last_manual_close = 0;
    g_sys_config.light_last_manual_close = 0;
    SystemConfig_Save();
    printf("[MANUAL] 切换到全自动模式\r\n");
}

void System_AutoRecovery(void)
{
    if (g_sys_config.system_mode == 1) return;
    
    printf("[RECOVERY] 20:00 自动恢复系统自动模式\r\n");
    g_sys_config.fan_mode = 1;
    g_sys_config.curtain_mode = 1;
    g_sys_config.light_mode = 1;
    g_sys_config.system_mode = 1;
    g_sys_config.fan_last_manual_close = 0;
    g_sys_config.curtain_last_manual_close = 0;
    g_sys_config.light_last_manual_close = 0;
    SystemConfig_Save();
}
