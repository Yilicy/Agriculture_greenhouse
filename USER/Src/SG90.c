#include "SG90.h"
#include "tim.h"

void set_SG90_angle(uint8_t angle)
{
    if(angle>180) angle = 180; // 限制角度在0-180度之间
    uint16_t pulse = 10*(angle+45)/9; // 将角度转换为PWM脉冲宽度，50对应0度，250对应180度
    
    __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, pulse);
}

void Curtain_On(void)
{
    // 打开窗帘，设置舵机角度为180度
    set_SG90_angle(180);
}

void Curtain_Off(void)
{
    // 关闭窗帘，设置舵机角度为90度
    set_SG90_angle(90);
}

/* 顺时针旋转 0°~180°，对应的脉冲宽度为 0.5ms~2.5ms
* 频率为 50Hz，周期为 20ms
* 0°(0.5ms)     500/20000 = 0.025       0.025*2000 = 50
* 45°(1ms)      1000/20000 = 0.05       0.05*2000 = 100
* 90°(1.5ms)    1500/20000 = 0.075      0.075*2000 = 150
* 135°(2ms)     2000/20000 = 0.1        0.1*2000 = 200
* 180°(2.5ms)   2500/20000 = 0.125      0.125*2000 = 250
*/
