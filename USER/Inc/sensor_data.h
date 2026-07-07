#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <stdint.h>
#include "main.h"
#include "cmsis_os.h"

// 数据类型枚举
typedef enum {
    DATA_TYPE_TEMP = 0,
    DATA_TYPE_HUMI = 1,
    DATA_TYPE_LIGHT = 2
} DataType_t;

typedef struct {
    float temperature;   // 温度
    uint8_t humidity;    // 湿度
    float light;         // 光照
    uint32_t timestamp;  // 采集时间戳（用于判断数据是否过期）
} SensorData_t;

// 全局变量声明
extern SensorData_t g_sensor_data;
extern uint8_t g_sensor_valid;  // 0=无效, 1=有效

// 传感器函数
void Sensor_Update();

// 数据记录API
uint8_t DataLogger_Init(void);

void DataLogger_StoreCurrent(void);
float* DataLogger_QueryByType(const char *date, DataType_t type, uint16_t *out_count);
void DataLogger_FreeResult(float *data);
uint16_t DataLogger_GetCount(const char *date);
void DataLogger_CleanOldFiles(void);

// ===================== 测试函数 =====================
void DataLogger_GenerateTestData(void);

#endif /* SENSOR_DATA_H */
