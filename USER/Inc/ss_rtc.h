#ifndef _SS_RTC_H_
#define _SS_RTC_H_

#include "stm32f4xx_hal.h"

extern const char *weekday_names[];

// RTC 
typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t day;
    uint8_t month;
    uint16_t year;
    uint8_t weekday; // 0-6:
} SS_RTC_Time_t;

// 
void SS_RTC_Init(void);
void SS_RTC_SetTime(uint8_t hours, uint8_t minutes, uint8_t seconds);
void SS_RTC_SetDate(uint8_t day, uint8_t month, uint16_t year, uint8_t weekday);
void SS_RTC_GetTime(SS_RTC_Time_t *time);
void SS_RTC_GetTimeString(char *buffer, uint16_t size);
void SS_RTC_GetDateString(char *buffer, uint16_t size);
void SS_RTC_Test(UART_HandleTypeDef *huart);  // 
uint8_t day_of_week(int year,int month,int day);
uint8_t Is_Leap_Year(uint16_t year);
void Lender_display(SS_RTC_Time_t *TIME);

#endif /* _SS_RTC_H_ */
