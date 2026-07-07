#include "ss_rtc.h"
#include <stdio.h>
#include <string.h>
#include "lcd_draw.h"

extern RTC_HandleTypeDef hrtc;
void Error_Handler(void);
const uint8_t mon_table[12]= {31,28,31,30,31,30,31,31,30,31,30,31};
const char *weekday_names[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};   // 星期名称数组

/**
 * @brief
 */
void SS_RTC_Init(void)
{

    if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != 0x32F2)
    {
        RTC_TimeTypeDef sTime = {0};
        RTC_DateTypeDef sDate = {0};
     
        sTime.Hours = 10;
        sTime.Minutes = 57;
        sTime.Seconds = 0;
        sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
        sTime.StoreOperation = RTC_STOREOPERATION_RESET;
        
        if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
        {
            Error_Handler();
        }
        sDate.WeekDay = RTC_WEEKDAY_WEDNESDAY;
        sDate.Month = RTC_MONTH_JANUARY;
        sDate.Date = 7;
        sDate.Year = 26;  
        
        if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
        {
            Error_Handler();
        }
        
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0x32F2);//
    }
}

/**
 * @brief 
 */
void SS_RTC_SetTime(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    RTC_TimeTypeDef sTime = {0};
    
    sTime.Hours = hours;
    sTime.Minutes = minutes;
    sTime.Seconds = seconds;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;//夏令时
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief ????
 */
void SS_RTC_SetDate(uint8_t day, uint8_t month, uint16_t year, uint8_t weekday)
{
    RTC_DateTypeDef sDate = {0};
    
    sDate.WeekDay = weekday;  // RTC_WEEKDAY_MONDAY?
    sDate.Month = month;      // RTC_MONTH_JANUARY?
    sDate.Date = day;
    sDate.Year = year - 2000; 
    
    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief 
 */
void SS_RTC_GetTime(SS_RTC_Time_t *time)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    
  
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    
    time->hours = sTime.Hours;
    time->minutes = sTime.Minutes;
    time->seconds = sTime.Seconds;
    time->day = sDate.Date;
    time->month = sDate.Month;
    time->year = sDate.Year + 2000; 
    time->weekday = sDate.WeekDay;
}

/**
 * @brief 
 */
void SS_RTC_GetTimeString(char *buffer, uint16_t size)
{
    SS_RTC_Time_t time;
    SS_RTC_GetTime(&time);
    
    snprintf(buffer, size, "%02d:%02d:%02d", time.hours, time.minutes, time.seconds);
}

/**
 * @brief 
 */
void SS_RTC_GetDateString(char *buffer, uint16_t size)
{
    SS_RTC_Time_t time;
    SS_RTC_GetTime(&time);
    
    snprintf(buffer, size, "%04d/%02d/%02d %s", 
             time.year, time.month, time.day, 
             weekday_names[time.weekday % 7]);
}

/**
 * @brief 
 */
void SS_RTC_Test(UART_HandleTypeDef *huart)
{
    SS_RTC_Time_t time;
    char buffer[64];
    
    SS_RTC_GetTime(&time);
    
    snprintf(buffer, sizeof(buffer), 
             "Time: %04d-%02d-%02d %02d:%02d:%02d Week:%s\r\n",
             time.year, time.month, time.day,
             time.hours, time.minutes, time.seconds,
             weekday_names[time.weekday % 7]);
}

//判断是否是闰年函数
//月份   1  2  3  4  5  6  7  8  9  10 11 12
//闰年   31 29 31 30 31 30 31 31 30 31 30 31
//非闰年 31 28 31 30 31 30 31 31 30 31 30 31
//输入:年份
//输出:该年份是不是闰年：1-是;0-不是
uint8_t Is_Leap_Year(uint16_t year)
{
    if(year%4==0) //必须能被4整除
    {
        if(year%100==0)
        {
            if(year%400==0)return 1;//如果以00结尾,还要能被400整除
            else return 0;
        } else return 1;
    } else return 0;
}

/* 使用蔡勒公式计算星期几 */
uint8_t day_of_week(int year,int month,int day)
{
    if(month<3)
    {
        month+=12;
        year-=1;
    }
    uint8_t k=year%100; //年份后两位
    uint8_t j=year/100; //世纪数

    uint8_t h=(day+(13*(month+1))/5+k+k/4+j/4+5*j)%7; //蔡勒公式

    uint8_t dow=(h+6)%7; // 0=周日, 1=周一, ..., 6=周六
    return dow;
}

//日历显示
void Lender_display(SS_RTC_Time_t *TIME)
{
    int week=day_of_week(TIME->year,TIME->month,1);
    int x=1,y=205;
    x=(week-1)*46+1;
    if(week==0)
        x=6*45+1;
    printf("x=%d\r\n",x);
    for(int i=1;i<=mon_table[TIME->month-1];i++)
    {
        if(i<10)
            lcd_shownum(x+8,y,32,i,LIGHTBLUE);
        else
            lcd_shownum(x,y,32,i,LIGHTBLUE);
        if(i==TIME->day){
            lcd_fill_circle(x+16,y+16,22,BLUE);
            if(i<10)
                lcd_shownum(x+8,y,32,i,WHITE);
            else
                lcd_shownum(x,y,32,i,WHITE);
        }
        x+=45;
        if(x>300)
        {
            x=1;
            y+=36;
        }
    }
}
