#include "sensor_data.h"
#include "ss_rtc.h"
#include "sd.h"
#include <stdio.h>
#include <string.h>

/* ==================== 内部状态 ==================== */

static uint8_t logger_initialized = 0;      /* 模块是否已初始化 */
static uint8_t last_stored_hour = 0xFF;     /* 上次存储的小时（用于去重） */
static uint8_t last_stored_date_valid = 0;  /* 上次存储日期是否有效 */
static uint8_t last_stored_day = 0;         /* 上次存储的日 */
static uint8_t last_stored_month = 0;       /* 上次存储的月 */
static uint16_t last_stored_year = 0;       /* 上次存储的年 */
static EnvData_t latest_data;               /* 最新环境数据（缓存） */

/* ==================== 内部函数 ==================== */
SS_RTC_Time_t rtc_time;  /* RTC时间缓存 */
/**
 * @brief 判断是否需要存储
 * @retval 1:需要存储, 0:不需要
 */
static uint8_t is_time_to_store(void)
{
    RTC_DateTypeDef date;
    uint8_t hour;
    uint8_t minute;
    
    /* 获取当前RTC时间 */
   SS_RTC_GetTime(&rtc_time);  // 获取当前时间
    
    hour = rtc_time.hours;
    minute = rtc_time.minutes;
    
    /* 检查分钟是否匹配 */
    if (minute != LOGGER_TRIGGER_MINUTE) {
        return 0;
    }
    
    /* 检查小时是否在存储序列中 (0, 2, 4, 6, ..., 22) */
    if (hour % LOGGER_INTERVAL_HOURS != 0) {
        return 0;
    }
    
    /* 检查是否已经存储过这个小时（防止重复触发） */
    if (last_stored_date_valid &&
        last_stored_year == date.Year + 2000 &&
        last_stored_month == date.Month &&
        last_stored_day == date.Date &&
        last_stored_hour == hour) {
        return 0;  /* 已经存过了 */
    }
    
    /* 更新上次存储信息 */
    last_stored_year = date.Year + 2000;
    last_stored_month = date.Month;
    last_stored_day = date.Date;
    last_stored_hour = hour;
    last_stored_date_valid = 1;
    
    return 1;
}

/**
 * @brief 生成日期文件名
 * @param date_str 日期字符串 "YYYY-MM-DD"
 * @param filename 输出缓冲区
 * @param size     缓冲区大小
 */
static void make_filename(const char *date_str, char *filename, uint16_t size)
{
    snprintf(filename, size, "%s/%s%s", DATA_DIR, date_str, DATA_FILE_EXT);
}

/**
 * @brief 格式化时间字符串
 * @param time  RTC时间结构体
 * @param buf   输出缓冲区 (至少6字节)
 */
static void format_time_str(RTC_TimeTypeDef *time, char *buf)
{
    snprintf(buf, 6, "%02d:%02d", time->Hours, time->Minutes);
}

/**
 * @brief 写入CSV表头（如果文件为空）
 * @param file 文件指针
 * @retval 0:成功, 其他:失败
 */
static uint8_t write_csv_header(FIL *file)
{
    FRESULT fr;
    UINT bw;
    const char *header = "time,temperature,humidity,light\n";
    
    fr = f_write(file, header, strlen(header), &bw);
    if (fr != FR_OK || bw != strlen(header)) {
        return 1;
    }
    return 0;
}

/**
 * @brief 追加一条数据到CSV文件
 * @param date_str 日期字符串 "YYYY-MM-DD"
 * @param data     环境数据
 * @retval 0:成功, 其他:失败
 */
static uint8_t append_data_to_file(const char *date_str, EnvData_t *data)
{
    FRESULT fr;
    FIL file;
    char filename[64];
    char line[LOGGER_LINE_MAX];
    UINT bw;
    uint8_t is_new_file = 0;
    
    /* 生成文件路径 */
    make_filename(date_str, filename, sizeof(filename));
    
    /* 检查文件是否存在 */
    fr = f_open(&file, filename, FA_READ);
    if (fr == FR_OK) {
        /* 文件存在，关闭后以追加模式打开 */
        f_close(&file);
        fr = f_open(&file, filename, FA_OPEN_APPEND | FA_WRITE);
    } else {
        /* 文件不存在，创建新文件 */
        fr = f_open(&file, filename, FA_CREATE_NEW | FA_WRITE);
        is_new_file = 1;
    }
    
    if (fr != FR_OK) {
        printf("打开数据文件失败: %d\r\n", fr);
        return 1;
    }
    
    /* 新文件写入表头 */
    if (is_new_file) {
        write_csv_header(&file);
    }
    
    /* 获取当前时间 */
    SS_RTC_GetTime(&rtc_time);  // 获取当前时间
    
    /* 格式化数据行: "HH:MM,temp,humi,light\n" */
    snprintf(line, sizeof(line), "%02d:%02d,%.1f,%.1f,%d\n",
             rtc_time.hours, rtc_time.minutes,
             data->temperature,
             data->humidity,
             data->light);
    
    /* 写入数据 */
    fr = f_write(&file, line, strlen(line), &bw);
    if (fr != FR_OK || bw != strlen(line)) {
        printf("写入数据失败: %d\r\n", fr);
        f_close(&file);
        return 1;
    }
    
    /* 同步到SD卡 */
    f_sync(&file);
    f_close(&file);
    
    printf("数据已存储: %s", line);
    return 0;
}

/* ==================== 对外接口实现 ==================== */

/**
 * @brief 数据记录模块初始化
 */
uint8_t data_logger_init(void)
{
    FRESULT fr;
    
    printf("=== 数据记录模块初始化 ===\r\n");
    
    /* 挂载SD卡文件系统 */
    if (mount_sd_fs() != 0) {
        printf("SD卡挂载失败，数据记录不可用\r\n");
        return 1;
    }
    
    /* 创建DATA文件夹（如果不存在） */
    fr = f_mkdir(DATA_DIR);
    if (fr == FR_OK) {
        printf("创建目录 %s 成功\r\n", DATA_DIR);
    } else if (fr == FR_EXIST) {
        printf("目录 %s 已存在\r\n", DATA_DIR);
    } else {
        printf("创建目录失败: %d\r\n", fr);
        return 1;
    }
    
    /* 初始化状态 */
    last_stored_hour = 0xFF;
    last_stored_date_valid = 0;
    latest_data.temperature = 0;
    latest_data.humidity = 0;
    latest_data.light = 0;
    
    logger_initialized = 1;
    printf("数据记录模块初始化完成\r\n");
    printf("存储间隔: %d 小时\r\n", LOGGER_INTERVAL_HOURS);
    printf("触发分钟: %d\r\n", LOGGER_TRIGGER_MINUTE);
    
    return 0;
}

/**
 * @brief 更新环境数据
 */
uint8_t data_logger_update(EnvData_t *data)
{
    char date_str[16];
    
    if (!logger_initialized) {
        printf("数据记录模块未初始化\r\n");
        return 0;
    }
    
    if (data == NULL) {
        return 0;
    }
    
    /* 缓存最新数据 */
    latest_data = *data;
    
    /* 判断是否到存储时间 */
    if (!is_time_to_store()) {
        return 0;
    }
    
    /* 获取当前日期 */
    SS_RTC_GetTime(&rtc_time);  // 获取当前日期
    snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d", rtc_time.year, rtc_time.month, rtc_time.day);
    
    /* 存储数据 */
    if (append_data_to_file(date_str, data) == 0) {
        return 1;  /* 已存储 */
    }
    
    return 0;
}

/**
 * @brief 按日期查询数据
 */
uint8_t data_logger_get_data(const char *date_str, char *buffer, uint16_t buf_size)
{
    FRESULT fr;
    FIL file;
    char filename[64];
    UINT br;
    uint16_t total_read = 0;
    uint8_t has_data = 0;
    
    if (!logger_initialized) {
        printf("数据记录模块未初始化\r\n");
        buffer[0] = '\0';
        return 0;
    }
    
    if (buffer == NULL || buf_size < 64) {
        return 0;
    }
    
    /* 检查日期格式 */
    if (strlen(date_str) != 10) {
        buffer[0] = '\0';
        return 0;
    }
    
    /* 生成文件路径 */
    make_filename(date_str, filename, sizeof(filename));
    printf("查询文件: %s\r\n", filename);
    
    /* 打开文件 */
    fr = f_open(&file, filename, FA_READ);
    if (fr != FR_OK) {
        printf("文件不存在或打开失败: %d\r\n", fr);
        buffer[0] = '\0';
        return 0;
    }
    
    /* 读取全部内容 */
    buffer[0] = '\0';
    while (1) {
        char chunk[128];
        fr = f_read(&file, chunk, sizeof(chunk) - 1, &br);
        if (fr != FR_OK || br == 0) {
            break;
        }
        chunk[br] = '\0';
        
        /* 检查缓冲区是否足够 */
        if (total_read + br >= buf_size - 1) {
            printf("缓冲区不足，数据被截断\r\n");
            break;
        }
        
        strcat(buffer, chunk);
        total_read += br;
        has_data = 1;
    }
    
    f_close(&file);
    
    if (!has_data) {
        buffer[0] = '\0';
        return 0;
    }
    
    printf("查询成功，读取 %d 字节\r\n", total_read);
    return 1;
}

/**
 * @brief 获取当前存储状态
 */
uint8_t data_logger_has_data(void)
{
    FRESULT fr;
    DIR dir;
    
    if (!logger_initialized) {
        return 0;
    }
    
    /* 打开DATA目录，检查是否有文件 */
    fr = f_opendir(&dir, DATA_DIR);
    if (fr != FR_OK) {
        return 0;
    }
    
    f_closedir(&dir);
    
    /* 如果有目录，认为可能有数据（实际通过查询文件判断） */
    return 1;
}

/**
 * @brief 手动触发存储
 */
uint8_t data_logger_force_store(EnvData_t *data)
{
    char date_str[16];
    
    if (!logger_initialized) {
        return 1;
    }
    
    if (data == NULL) {
        data = &latest_data;
    }
    
    /* 获取当前日期 */
    SS_RTC_GetTime(&rtc_time);
    snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d",
             rtc_time.year, rtc_time.month, rtc_time.day);
    
    /* 强制存储 */
    if (append_data_to_file(date_str, data) == 0) {
        /* 更新上次存储记录，避免正常定时重复存储 */
        SS_RTC_GetTime(&rtc_time);
        last_stored_hour = rtc_time.hours;
        last_stored_date_valid = 1;
        last_stored_year = rtc_time.year;
        last_stored_month = rtc_time.month;
        last_stored_day = rtc_time.day;
        return 0;
    }
    
    return 1;
}
