#include "Motor.h"
#include "tim.h"

void Fan_SetLevel(int level)
{
    int speed_table[] = {0, 50, 65, 80, 95};
    if (level < 0) level = 0;
    if (level > 5) level = 5;
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, speed_table[level]);
}

void Light_On(void)
{
    HAL_GPIO_WritePin(LED2_GPIO_Port,LED2_Pin,GPIO_PIN_RESET);
}

void Light_Off(void)
{
    HAL_GPIO_WritePin(LED2_GPIO_Port,LED2_Pin,GPIO_PIN_SET);
}
