#ifndef __SENSOR_DATA_H
#define __SENSOR_DATA_H

#include "main.h"
#include "fatfs.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief 存储间隔（小时）
 * @note  默认2小时，可修改为1,3,4,6,8,12等
 *        修改后存储时间点自动变化
 */
#define LOGGER_INTERVAL_HOURS   2

/**
 * @brief 存储触发分钟（0=整点）
 * @note  默认0，可修改为30（每小时的30分触发）
 */
#define LOGGER_TRIGGER_MINUTE   0

/**
 * @brief 数据文件保存路径
 */
#define DATA_DIR                "0:/DATA"
#define DATA_FILE_EXT           ".csv"

/**
 * @brief 单条数据最大长度（时间+3个数值）
 */
#define LOGGER_LINE_MAX         64

/**
 * @brief 查询返回缓冲区大小（一天最多24条数据）
 * @note  24小时/间隔小时 + 1行表头 + 预留
 */
#define LOGGER_BUFFER_SIZE      (24 / LOGGER_INTERVAL_HOURS * LOGGER_LINE_MAX + 128)

/**
 * @brief 环境数据点
 */
typedef struct {
    float temperature;      /* 温度（摄氏度） */
    float humidity;         /* 湿度（百分比） */
    uint16_t light;         /* 光照强度（ADC值或Lux） */
} EnvData_t;

/**
 * @brief 数据记录模块初始化
 * @retval 0:成功, 其他:失败
 */
uint8_t data_logger_init(void);

/**
 * @brief 更新环境数据（外部定时调用）
 * @param data 环境数据指针
 * @retval 1:已存储, 0:未存储（未到记录时间点）
 * @note  调用频率建议每秒1次，内部自动判断是否到存储时间
 */
uint8_t data_logger_update(EnvData_t *data);

/**
 * @brief 按日期查询数据（给前端对接）
 * @param date_str  日期字符串，格式 "YYYY-MM-DD"，如 "2026-07-01"
 * @param buffer    输出缓冲区
 * @param buf_size  缓冲区大小
 * @retval 1:查询成功（buffer中有数据）, 0:查询失败/无数据
 * @note   buffer中为CSV格式数据：
 *         time,temperature,humidity,light
 *         00:00,25.3,60.5,320
 *         02:00,26.1,58.2,280
 *         ...
 */
uint8_t data_logger_get_data(const char *date_str, char *buffer, uint16_t buf_size);

/**
 * @brief 获取当前存储状态
 * @retval 1:已存储过（有数据文件）, 0:从未存储
 */
uint8_t data_logger_has_data(void);

/**
 * @brief 手动触发存储（调试用或紧急存储）
 * @param data 环境数据指针
 * @retval 0:成功, 其他:失败
 */
uint8_t data_logger_force_store(EnvData_t *data);

#endif /* __SENSOR_DATA_H */