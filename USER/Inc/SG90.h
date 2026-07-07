#ifndef SG90_H_
#define SG90_H_

#include "stm32f4xx.h"

// 舵机(卷帘)控制阈值
#define CURTAIN_OPEN_THRESHOLD  8000    // 光照大于8000 lx时打开卷帘
#define CURTAIN_CLOSE_THRESHOLD 5000    // 光照低于 5000 Lux 才关闭

void set_SG90_angle(uint8_t angle);
void Curtain_On(void);
void Curtain_Off(void);

#endif /* SG90_H_ */
