#ifdef DS3231_RTC

//
//
//
bool RTC_init(void)
{
  //  Serial.println("Initializing RTC");
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    delay(5000);
    return false;
  }
  Serial.println("Initializing RTC");
  pinMode(RTC_INT_PIN, INPUT_PULLUP);
  rtc.disableAlarm(ALARM_1);      //rtc.alarmInterrupt(ALARM_1, false);
  rtc.disableAlarm(ALARM_2);      //rtc.alarmInterrupt(ALARM_2, false);
  rtc.writeSqwPinMode(DS3231_OFF );    //rtc.squareWave(SQWAVE_NONE);
  rtc.clearAlarm(ALARM_1);
  rtc.clearAlarm(ALARM_2);
  // set alarm 1 to occur every second
  //rtc.setAlarm1( dt, DS3231_A1_PerSecond);     //    rtc.setAlarm(ALM1_EVERY_SECOND, 0, 0, 0, 0);
  // set alarm 2 to occur every minute
  rtc.setAlarm2( dt, DS3231_A2_PerMinute);

  //  Serial.println("RTC: Waiting to Start....");
  delay(1000);

  DateTime now = rtc.now();
  Serial.println(now.unixtime());
  
  //syncTime();
  return true;
}

#endif
