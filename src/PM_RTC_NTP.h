#ifdef DS3231_RTC
#include <RTClib.h>

#define GMT_TIME_ZONE 0

RTC_DS3231 rtc;

DateTime dt;

#define ALARM_1 1
#define ALARM_2 2

#define RTC_INT_PIN 35

void RTC_Init(void);

#endif
