#include "Motor.h"
#include "tim.h"

void Fan_SetLevel(int level)
{
    int speed_table[] = {0, 50, 65, 80, 95};
    if (level < 0) level = 0;
    if (level > 5) level = 5;
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, speed_table[level]);
}
