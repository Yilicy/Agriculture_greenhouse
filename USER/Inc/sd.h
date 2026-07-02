#ifndef __SD_H
#define __SD_H

#include <sdio.h>
#include <fatfs.h>

#define SD_TIMEOUT             ((uint32_t)100000000)    /* 超时时间 */
#define SD_TRANSFER_OK         ((uint8_t)0x00)
#define SD_TRANSFER_BUSY       ((uint8_t)0x01)
#define  SDIO_TRANSF_CLK_DIV        1   

#define SD_TOTAL_SIZE_BYTE(__Handle__)  (((uint64_t)((__Handle__)->SdCard.LogBlockNbr) * ((__Handle__)->SdCard.LogBlockSize)) >> 0)
#define SD_TOTAL_SIZE_KB(__Handle__)    (((uint64_t)((__Handle__)->SdCard.LogBlockNbr) * ((__Handle__)->SdCard.LogBlockSize)) >> 10)
#define SD_TOTAL_SIZE_MB(__Handle__)    (((uint64_t)((__Handle__)->SdCard.LogBlockNbr) * ((__Handle__)->SdCard.LogBlockSize)) >> 20)
#define SD_TOTAL_SIZE_GB(__Handle__)    (((uint64_t)((__Handle__)->SdCard.LogBlockNbr) * ((__Handle__)->SdCard.LogBlockSize)) >> 30)

extern SD_HandleTypeDef hsd;            /* SD卡句柄 */
extern HAL_SD_CardInfoTypeDef sd_card_info_handle; /* SD卡信息结构体 */

// SD卡底层函数
uint8_t get_sd_card_info(HAL_SD_CardInfoTypeDef *info_handle);
uint8_t get_sd_card_state(void);
uint8_t sd_read(uint8_t *pbuf, uint32_t saddr, uint32_t cnt);
uint8_t sd_write(uint8_t *pbuf, uint32_t saddr, uint32_t cnt);

// 文件系统管理
uint8_t mount_sd_fs(void);          /* 挂载SD卡文件系统 */

#endif
