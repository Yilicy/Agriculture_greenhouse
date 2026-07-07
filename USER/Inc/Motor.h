#ifndef MOTOR_H_
#define MOTOR_H_

#include "stm32f4xx.h"

// 风扇速度档位
#define FAN_OFF     0
#define FAN_LOW     1
#define FAN_MID     2
#define FAN_HIGH    3
#define FAN_FULL    4

// 温度阈值定义
#define TEMP_COMFORT_LOW    22.0f   // 舒适区下限
#define TEMP_COMFORT_HIGH   28.0f   // 舒适区上限（风扇启动温度）
#define TEMP_LOW_MID        31.0f   // 低速→中速 切换温度
#define TEMP_MID_HIGH       34.0f   // 中速→高速 切换温度
#define TEMP_HIGH_FULL      37.0f   // 高速→全速 切换温度
#define HYSTERESIS 1.5f   // 1.5°C 迟滞

void Fan_SetLevel(int level);

#endif /* MOTOR_H_ */
