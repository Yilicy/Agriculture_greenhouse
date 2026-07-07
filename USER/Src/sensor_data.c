#include "sensor_data.h"
#include "ss_rtc.h"
#include "sd.h"
#include "dht.h"
#include "BH1750.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define DATA_DIR "0:/DATA/" 
#define MAX_DAYS 7
#define MAX_DATA_PER_DAY 12  // 24小时 / 2小时 = 12次

// 全局变量定义
SensorData_t g_sensor_data = {0, 0, 0, 0};
uint8_t g_sensor_valid = 0;

void Sensor_Update(void)
{
    float t, l;
    uint8_t h;
    
    // 读取传感器
    dht_read_data(&h, &t);
    l = BH1750_ReadLight();
    
    // 存入全局变量
    g_sensor_data.temperature = t;
    g_sensor_data.humidity = (uint8_t)h;
    g_sensor_data.light = l;
    g_sensor_data.timestamp = HAL_GetTick();
    g_sensor_valid = 1;
}

static int is_even_hour(SS_RTC_Time_t *time)
{
    // 只在分钟=0 且 小时为偶数时存储（0,2,4,6,8,10,12,14,16,18,20,22）
    return (time->minutes == 0 && time->hours % 2 == 0);
}

static void build_file_path(char *path, const char *date)
{
    sprintf(path, "%s%s.CSV", DATA_DIR, date);
}

uint8_t DataLogger_Init(void)
{
    FRESULT fr;

    // printf("[LOGGER] SDPath = %s\r\n", SDPath);
    
    // 挂载 SD 卡
    fr = f_mount(&SDFatFS, "0:", 1);
    if (fr != FR_OK) {
        printf("[LOGGER] FATFS 挂载失败: %d\r\n", fr);
        return 0;
    }
    printf("[LOGGER] FATFS 挂载成功\r\n");
    
    // 创建 DATA 文件夹
    fr = f_mkdir("0:/DATA");
    if (fr == FR_OK || fr == FR_EXIST) {
        printf("[LOGGER] 数据目录已就绪: 0:/DATA\r\n");
    } else {
        printf("[LOGGER] 创建目录失败: %d\r\n", fr);
    }
    return 1;
}

/**
 * @brief 存储当前传感器数据
 * @note  内部自动判断是否需要存储（偶数小时整点）
 *        内部自动检查并删除7天前的数据
 */
void DataLogger_StoreCurrent(void)
{
    SS_RTC_Time_t time;
    float temp, light;
    uint8_t humi;
    char file_path[64];
    char data_line[64];
    FIL file;
    FRESULT fr;
    
    SS_RTC_GetTime(&time);
    
    // 判断是否需要存储（偶数小时整点）
    if (!is_even_hour(&time)) {
        return;  // 不是存储时间点，直接返回
    }
    
    // 如果是 00:00，先检查并删除7天前的数据
    if (time.hours == 0 && time.minutes == 0) {
        DataLogger_CleanOldFiles();
    }
    
    // 读取传感器数据
    Sensor_Update();
    
    // 构建文件路径
    sprintf(file_path, "%s%04d%02d%02d.CSV", DATA_DIR, time.year, time.month, time.day);
    
    // 打开文件（追加模式）
    fr = f_open(&file, file_path, FA_OPEN_APPEND | FA_WRITE | FA_READ);
    if (fr == FR_NO_FILE) {
        // 文件不存在，创建新文件并写入表头
        fr = f_open(&file, file_path, FA_CREATE_NEW | FA_WRITE | FA_READ);
        if (fr == FR_OK) {
            f_write(&file, "时间,温度,湿度,光照\n", strlen("时间,温度,湿度,光照\n"), NULL);
        }
    }
    
    if (fr == FR_OK) {
        // 写入数据
        sprintf(data_line, "%02d:%02d,%.1f,%d,%.0f\n",
                time.hours, time.minutes,
                temp, humi, light);
        f_write(&file, data_line, strlen(data_line), NULL);
        f_sync(&file);
        f_close(&file);
        printf("[LOGGER] 存储成功: %s", data_line);
    } else {
        printf("[LOGGER] 存储失败，错误码: %d\r\n", fr);
    }
}

/**
 * @brief 查询某一天某类型的数据
 * @param date     日期字符串，格式 "YYYY-MM-DD"
 * @param type     数据类型：DATA_TYPE_TEMP / DATA_TYPE_HUMI / DATA_TYPE_LIGHT
 * @param out_count 输出参数，返回数据条数
 * @return float*  动态分配的数据数组指针
 * @note  调用者使用完后必须调用 DataLogger_FreeResult() 释放内存
 */
float* DataLogger_QueryByType(const char *date, DataType_t type, uint16_t *out_count)
{
    char file_path[64];
    FIL file;
    FRESULT fr;
    float *result = NULL;
    uint16_t count = 0;
    uint16_t capacity = MAX_DATA_PER_DAY;
    char line[128];  // 加大缓冲区
    
    result = (float*)malloc(capacity * sizeof(float));
    if (result == NULL) {
        *out_count = 0;
        return NULL;
    }
    
    build_file_path(file_path, date);
    fr = f_open(&file, file_path, FA_READ);
    if (fr != FR_OK) {
        free(result);
        *out_count = 0;
        return NULL;
    }
    
    // 跳过表头
    f_gets(line, sizeof(line), &file);
    
    // 逐行读取数据
    while (f_gets(line, sizeof(line), &file) != NULL) {
        char time_str[8];
        uint8_t humi = 0;
        float temp = 0, light = 0;
        float value = 0;
        
        // 使用 %f 直接解析
        int parsed = sscanf(line, "%[^,],%f,%d,%f", time_str, &temp, &humi, &light);
        
        // 检查解析是否成功
        if (parsed != 4) {
            printf("[QUERY] 解析失败: %s (parsed=%d)\r\n", line, parsed);
            continue;
        }
        
        // 根据类型提取对应值
        switch (type) {
            case DATA_TYPE_TEMP:  value = temp;  break;
            case DATA_TYPE_HUMI:  value = (float)humi;  break;
            case DATA_TYPE_LIGHT: value = light; break;
            default: continue;
        }
        
        // 存储到结果数组
        if (count < capacity) {
            result[count++] = value;
        } else {
            break;
        }
    }
    
    f_close(&file);
    *out_count = count;
    return result;
}

/**
 * @brief 释放查询结果内存
 * @param data 由 DataLogger_QueryByType 返回的指针
 */
void DataLogger_FreeResult(float *data)
{
    if (data != NULL) {
        free(data);
    }
}

/**
 * @brief 获取某一天的数据条数（不分配内存，仅查询）
 * @param date 日期字符串，格式 "YYYY-MM-DD"
 * @return 数据条数，0表示无数据或文件不存在
 */
uint16_t DataLogger_GetCount(const char *date)
{
    char file_path[64];
    FIL file;
    FRESULT fr;
    uint16_t count = 0;
    char line[64];
    
    build_file_path(file_path, date);
    fr = f_open(&file, file_path, FA_READ);
    if (fr != FR_OK) {
        return 0;
    }
    
    // 跳过表头
    f_gets(line, sizeof(line), &file);
    
    // 统计行数
    while (f_gets(line, sizeof(line), &file) != NULL) {
        if (strlen(line) > 3) count++;
    }
    
    f_close(&file);
    return count;
}

/**
 * @brief 手动触发删除7天前的文件
 * @note  通常不需要手动调用，存储时会自动检查
 */
void DataLogger_CleanOldFiles(void)
{
    SS_RTC_Time_t time;
    char file_path[64];
    FRESULT fr;
    DIR dir;
    FILINFO fno;
    int delete_count = 0;
    
    SS_RTC_GetTime(&time);
    printf("[LOGGER] 开始清理7天前的文件...\r\n");
    
    fr = f_opendir(&dir, DATA_DIR);
    if (fr != FR_OK) {
        f_mkdir(DATA_DIR);
        return;
    }
    
    while (1) {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK || fno.fname[0] == 0) break;
        
        // 只处理 .CSV 文件
        size_t len = strlen(fno.fname);
        if (len < 11 || strncmp(fno.fname + len - 4, ".CSV", 4) != 0) continue;
        
        // 从文件名提取日期
        int file_year, file_month, file_day;
        if (sscanf(fno.fname, "%d-%d-%d.CSV", &file_year, &file_month, &file_day) == 3) {
            // 计算天数差
            int diff = (time.year - file_year) * 365 + (time.month - file_month) * 30 + (time.day - file_day);
            if (diff >= MAX_DAYS) {
                sprintf(file_path, "%s%s", DATA_DIR, fno.fname);
                fr = f_unlink(file_path);
                if (fr == FR_OK) {
                    delete_count++;
                    printf("[LOGGER] 删除旧文件: %s\r\n", fno.fname);
                }
            }
        }
    }
    f_closedir(&dir);
    printf("[LOGGER] 清理完成，删除 %d 个文件\r\n", delete_count);
}

// void DataLogger_GenerateTestData(void)
// {
//     const char *short_dates[] = {"20260701", "20260702", "20260703", "20260704"};
//     const int hours[] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22};
//     char file_path[64];
//     char data_line[64];
//     FIL file;
//     FRESULT fr;
//     int day, hour;
//     UINT bw;
    
//     printf("[TEST] 开始生成测试数据...\r\n");
    
//     // ★★★ 直接定义温度、湿度、光照的数组，不需要计算 ★★★
//     // 每天12个数据点（每2小时一个）
//     float test_data[4][12][3] = {
//         // 20260701: {温度, 湿度, 光照}
//         {{22.5, 65.0, 5}, {22.8, 63.0, 6}, {23.0, 61.0, 7}, {23.2, 59.0, 150},
//          {23.5, 57.0, 300}, {23.8, 55.0, 350}, {24.0, 54.0, 380}, {23.8, 55.0, 350},
//          {23.5, 57.0, 300}, {23.2, 59.0, 150}, {23.0, 61.0, 10}, {22.7, 63.0, 6}},
        
//         // 20260702
//         {{23.0, 62.0, 5}, {23.2, 60.0, 6}, {23.5, 58.0, 7}, {23.8, 56.0, 160},
//          {24.0, 54.0, 320}, {24.3, 52.0, 380}, {24.5, 51.0, 400}, {24.3, 52.0, 380},
//          {24.0, 54.0, 320}, {23.8, 56.0, 160}, {23.5, 58.0, 10}, {23.2, 60.0, 6}},
        
//         // 20260703
//         {{25.0, 58.0, 5}, {25.2, 56.0, 6}, {25.5, 54.0, 7}, {25.8, 52.0, 140},
//          {26.0, 50.0, 280}, {26.3, 48.0, 330}, {26.5, 47.0, 360}, {26.3, 48.0, 330},
//          {26.0, 50.0, 280}, {25.8, 52.0, 140}, {25.5, 54.0, 10}, {25.2, 56.0, 6}},
        
//         // 20260704
//         {{24.0, 60.0, 5}, {24.2, 58.0, 6}, {24.5, 56.0, 7}, {24.8, 54.0, 180},
//          {25.0, 52.0, 350}, {25.3, 50.0, 420}, {25.5, 49.0, 450}, {25.3, 50.0, 420},
//          {25.0, 52.0, 350}, {24.8, 54.0, 180}, {24.5, 56.0, 10}, {24.2, 58.0, 6}}
//     };
    
//     for (day = 0; day < 4; day++) {
//         sprintf(file_path, "%s%s.CSV", DATA_DIR, short_dates[day]);
//         printf("[TEST] 尝试创建: %s\r\n", file_path);
        
//         fr = f_open(&file, file_path, FA_CREATE_ALWAYS | FA_WRITE);
//         if (fr != FR_OK) {
//             printf("[TEST] 创建文件失败: %s, 错误码: %d\r\n", short_dates[day], fr);
//             continue;
//         }
        
//         f_write(&file, "时间,温度,湿度,光照\n", strlen("时间,温度,湿度,光照\n"), &bw);
        
//         for (hour = 0; hour < 12; hour++) {
//             float temp = test_data[day][hour][0];
//             float humi = test_data[day][hour][1];
//             float light = test_data[day][hour][2];
            
//             sprintf(data_line, "%02d:%02d,%.1f,%.1f,%.0f\n",
//                     hours[hour], 0, temp, humi, light);
//             printf("[TEST] 写入: %s", data_line);
//             f_write(&file, data_line, strlen(data_line), &bw);
//         }
        
//         f_close(&file);
//         printf("[TEST] 生成完成: %s.CSV (12条数据)\r\n", short_dates[day]);
//     }
    
//     printf("[TEST] 所有测试数据生成完成!\r\n");
// }
