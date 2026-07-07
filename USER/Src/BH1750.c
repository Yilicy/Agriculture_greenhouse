#include "BH1750.h"
#include "iic.h"

extern void delay_us(uint32_t us);

/**
 * @brief       BH1750 发送指令
 */
void bh_data_send(uint8_t cmd)
{
    iic_start();
    iic_send_byte(BHAddWrite);
    iic_wait_ack();
    iic_send_byte(cmd);
    iic_wait_ack();
    iic_stop();
}

/**
 * @brief       初始化 BH1750
 */
void BH1750_Init(void)
{
    iic_init();
    
    bh_data_send(0x01);  // 上电
    HAL_Delay(100);
    bh_data_send(0x07);  // 复位
    HAL_Delay(100);
    bh_data_send(0x10);  // 设置为连续高分辨率模式
    HAL_Delay(180);
}

/**
 * @brief       读取光照值
 * @retval      光照强度 lx，失败返回 -1.0
 */
float BH1750_ReadLight(void)
{
    uint16_t raw = 0;
    uint8_t ack = 0;
    
    // 1. 发送测量指令（连续高分辨率模式）
    iic_start();
    iic_send_byte(BHAddWrite);     // 0x46
    ack = iic_wait_ack();
    if (ack) {
        iic_stop();
        return -1.0f;
    }
    iic_send_byte(0x10);            // 连续高分辨率模式
    ack = iic_wait_ack();
    if (ack) {
        iic_stop();
        return -1.0f;
    }
    iic_stop();
    
    // ★★★★★ 关键：等待 180ms 让传感器完成测量！★★★★★
    HAL_Delay(180);
    
    // 2. 读取数据
    iic_start();
    iic_send_byte(BHAddRead);       // 0x47
    ack = iic_wait_ack();
    if (ack) {
        iic_stop();
        return -1.0f;
    }
    
    raw = iic_read_byte(1) << 8;    // 读高字节，发送 ACK
    raw |= iic_read_byte(0);        // 读低字节，发送 NACK
    iic_stop();
    
    if (raw == 0xFFFF || raw == 0) {
        return -2.0f;
    }
    
    return raw / 1.2f;
}
