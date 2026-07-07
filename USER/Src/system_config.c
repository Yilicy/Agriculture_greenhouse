#include "system_config.h"
#include "fatfs.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>

// 全局变量
SystemConfig_t g_sys_config;
uint8_t g_config_initialized = 0;
SemaphoreHandle_t MutexpHandle = NULL;

#define CONFIG_FILE "0:/config.txt"

//  默认配置
void SystemConfig_SetDefaults(void)
{
    // 用户配置
    g_sys_config.temp_min = 22.0f;      // 适宜温度下限
    g_sys_config.temp_max = 28.0f;      // 适宜温度上限
    g_sys_config.light_min = 500;       // 适宜光照下限 (Lux)
    g_sys_config.light_max = 8000;      // 适宜光照上限 (Lux)
    
    // 风扇
    g_sys_config.fan_level = 0;         // 风扇关闭
    g_sys_config.fan_mode = 1;          // 自动模式
    g_sys_config.fan_state = 0;         // 关闭
    g_sys_config.fan_last_manual_close = 0;
    
    // 卷帘
    g_sys_config.curtain_state = 0;     // 卷帘关闭
    g_sys_config.curtain_mode = 1;      // 自动模式
    g_sys_config.curtain_last_manual_close = 0;
    
    // 补光灯
    g_sys_config.light_state = 0;       // 补光灯关闭
    g_sys_config.light_mode = 1;        // 自动模式
    g_sys_config.light_last_manual_close = 0;
    
    // 系统
    g_sys_config.system_mode = 1;       // 自动模式
}

// 读取配置
void SystemConfig_Load(void)
{
    FIL file;
    FRESULT fr;
    char line[64];
    float fval;
    int ival;
    
    // 先设置默认值
    SystemConfig_SetDefaults();
    
    fr = f_open(&file, CONFIG_FILE, FA_READ);
    if (fr != FR_OK) {
        g_config_initialized = 0;
        return;
    }
    
    while (f_gets(line, sizeof(line), &file) != NULL) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '#') {
            continue;
        }
        
        if (sscanf(line, " temp_min = %f", &fval) == 1) {
            g_sys_config.temp_min = fval;
        }
        else if (sscanf(line, " temp_max = %f", &fval) == 1) {
            g_sys_config.temp_max = fval;
        }
        else if (sscanf(line, " light_min = %d", &ival) == 1) {
            g_sys_config.light_min = (uint16_t)ival;
        }
        else if (sscanf(line, " light_max = %d", &ival) == 1) {
            g_sys_config.light_max = (uint16_t)ival;
        }
        else if (sscanf(line, " fan_mode = %d", &ival) == 1) {
            g_sys_config.fan_mode = (uint8_t)ival;
        }
        else if (sscanf(line, " curtain_mode = %d", &ival) == 1) {
            g_sys_config.curtain_mode = (uint8_t)ival;
        }
        else if (sscanf(line, " light_mode = %d", &ival) == 1) {
            g_sys_config.light_mode = (uint8_t)ival;
        }
        else if (sscanf(line, " system_mode = %d", &ival) == 1) {
            g_sys_config.system_mode = (uint8_t)ival;
        }
    }
    
    f_close(&file);
    
    // 上电时重置冷却期和设备状态
    g_sys_config.fan_last_manual_close = 0;
    g_sys_config.curtain_last_manual_close = 0;
    g_sys_config.light_last_manual_close = 0;
    g_sys_config.fan_level = 0;
    g_sys_config.fan_state = 0;
    g_sys_config.curtain_state = 0;
    g_sys_config.light_state = 0;
    
    g_config_initialized = 1;
}

void SystemConfig_Save(void)
{
    FIL file;
    FRESULT fr;
    
    fr = f_open(&file, CONFIG_FILE, FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) {
        return;
    }
    
    f_printf(&file, "# ========== 大棚环境配置 ==========\r\n");
    f_printf(&file, "temp_min = %.1f\r\n", g_sys_config.temp_min);
    f_printf(&file, "temp_max = %.1f\r\n", g_sys_config.temp_max);
    f_printf(&file, "light_min = %d\r\n", g_sys_config.light_min);
    f_printf(&file, "light_max = %d\r\n", g_sys_config.light_max);
    f_printf(&file, "fan_mode = %d\r\n", g_sys_config.fan_mode);
    f_printf(&file, "curtain_mode = %d\r\n", g_sys_config.curtain_mode);
    f_printf(&file, "light_mode = %d\r\n", g_sys_config.light_mode);
    f_printf(&file, "system_mode = %d\r\n", g_sys_config.system_mode);
    
    f_close(&file);
}

void SystemConfig_Init(void)
{
    printf("[CFG] 系统配置初始化...\r\n");
    
    SystemConfig_Load();

    if (!g_config_initialized) {
        printf("[CFG] 首次开机，创建默认配置\r\n");
        SystemConfig_Save();
    }
    else {
        printf("[CFG] 配置加载成功\r\n");
    }
    
    SystemConfig_Print();
}

void SystemConfig_Print(void)
{
    printf("========== 系统配置 ==========\r\n");
    printf("温度: %.1f°C ~ %.1f°C\r\n", g_sys_config.temp_min, g_sys_config.temp_max);
    printf("光照: %d ~ %d Lux\r\n", g_sys_config.light_min, g_sys_config.light_max);
    printf("风扇: mode=%d(%s), state=%d(%s), level=%d\r\n", 
           g_sys_config.fan_mode, 
           g_sys_config.fan_mode == 1 ? "自动" : "手动",
           g_sys_config.fan_state,
           g_sys_config.fan_state == 1 ? "开" : "关",
           g_sys_config.fan_level);
    printf("卷帘: mode=%d(%s), state=%d(%s)\r\n", 
           g_sys_config.curtain_mode,
           g_sys_config.curtain_mode == 1 ? "自动" : "手动",
           g_sys_config.curtain_state,
           g_sys_config.curtain_state == 1 ? "开" : "关");
    printf("补光灯: mode=%d(%s), state=%d(%s)\r\n", 
           g_sys_config.light_mode,
           g_sys_config.light_mode == 1 ? "自动" : "手动",
           g_sys_config.light_state,
           g_sys_config.light_state == 1 ? "开" : "关");
    printf("系统模式: %s\r\n", g_sys_config.system_mode == 1 ? "全自动" : "混合");
    printf("==============================\r\n");
}
