#include "sd.h"
#include "lcd.h"
#include "lcd.h"
#include "touch.h"
#include "FreeRTOS.h"  
#include "semphr.h"  
#include <string.h>
#include <stdio.h>

/** @def MAX_IMAGES 最大支持图片数量（序号1~100）*/
#define MAX_IMAGES 100

/**
 * @brief 图片槽位结构体
 * @note 每个槽位对应一个固定的序号（槽位0对应IMG_001.BIN，槽位1对应IMG_002.BIN...）
 */
typedef struct {
    uint8_t number;     /* 图片序号（1-based），固定为槽位号+1 */
    uint8_t valid;      /* 1=图片存在，0=图片已删除（空缺） */
} ImageSlot_t;

static uint8_t sd_fs_mounted = 0;           /* SD卡文件系统挂载标志 */

extern SemaphoreHandle_t MutexHandle;       /* 互斥锁，保护LCD操作 */
HAL_SD_CardInfoTypeDef sd_card_info_handle; /* SD卡信息结构体 */

// SD卡底层函数（扇区读写）
/**
 * @brief 获取SD卡信息
 * @param info_handle 信息结构体指针
 * @return HAL_OK=成功，其他=失败
 */
uint8_t get_sd_card_info(HAL_SD_CardInfoTypeDef *info_handle)
{
    uint8_t sta = HAL_OK;
    sta = HAL_SD_GetCardInfo(&hsd, info_handle);
    return sta;
}

/**
 * @brief 获取SD卡状态
 * @return SD_TRANSFER_OK=空闲，SD_TRANSFER_BUSY=忙碌
 */
uint8_t get_sd_card_state(void)
{
    return ((HAL_SD_GetCardState(&hsd) == HAL_SD_CARD_TRANSFER) ? SD_TRANSFER_OK : SD_TRANSFER_BUSY);
}

/**
 * @brief SD卡扇区读取函数
 * @param pbuf 数据缓冲区
 * @param saddr 起始扇区地址（512字节对齐）
 * @param cnt 扇区数量
 * @return HAL_OK=成功，其他=失败
 * @note 关闭中断确保DMA传输不被中断
 */
uint8_t sd_read(uint8_t *pbuf, uint32_t saddr, uint32_t cnt)
{
    uint8_t sta = HAL_OK;
    uint32_t timeout = SD_TIMEOUT;
    long long lsector = saddr;

    __disable_irq();  // 关闭总中断（POLLING模式，严禁打断SDIO读操作）
    sta = HAL_SD_ReadBlocks(&hsd, (uint8_t*)pbuf, lsector, cnt, SD_TIMEOUT);

    // 等待SD卡读完
    while (get_sd_card_state() != SD_TRANSFER_OK)
    {
        if (timeout-- == 0)
        {
            sta = SD_TRANSFER_BUSY;
        }
    }
    __enable_irq(); // 开启总中断
    return sta;
}

/**
 * @brief SD卡扇区写入函数
 * @param pbuf 数据缓冲区
 * @param saddr 起始扇区地址（512字节对齐）
 * @param cnt 扇区数量
 * @return HAL_OK=成功，其他=失败
 * @note 关闭中断确保DMA传输不被中断
 */
uint8_t sd_write(uint8_t *pbuf, uint32_t saddr, uint32_t cnt)
{ 
    uint8_t sta = HAL_OK; 
    uint32_t timeout = SD_TIMEOUT; 
    long long lsector = saddr; 
    __disable_irq();    // 关闭总中断(POLLING模式,严禁中断打断SDIO写操作) 
    sta = HAL_SD_WriteBlocks(&hsd, (uint8_t *)pbuf, lsector, cnt, SD_TIMEOUT);
    
    // 等待SD卡写完
    while (get_sd_card_state() != SD_TRANSFER_OK) 
    { 
        if (timeout-- == 0) 
        { 
            sta = SD_TRANSFER_BUSY; 
        } 
    } 
    __enable_irq(); // 开启总中断
    return sta;
}

uint8_t mount_sd_fs(void)
{
    extern FATFS SDFatFS;
    
    if (!sd_fs_mounted)
    {
        // 使用硬编码盘符 "0:" 
        FRESULT fr = f_mount(&SDFatFS, "0:", 1);
        if (fr == FR_OK)
        {
            sd_fs_mounted = 1;
            printf("SD卡文件系统挂载成功\r\n");
        }
        else
        {
            printf("SD卡文件系统挂载失败: %d\r\n", fr);
            return 1;
        }
    }
    return 0;
}
