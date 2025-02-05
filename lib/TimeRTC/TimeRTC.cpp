#include "TimeRTC.h"

void TimeRTC::Tick()
{
  _dt = DateTime(_rtc.getYear(), _rtc.getMonth(_century), _rtc.getDate(), _rtc.getHour(_h12, _pm), _rtc.getMinute(), _rtc.getSecond());
  _second = _dt.second();

  if(_lastSecond != _second)
  {
    _lastSecond = _second;
    _isTimeUpdated = true;
  }
  else
  {
    _isTimeUpdated = false;
  }
}

String TimeRTC::GetCurrentTimeStr()
{
  _year = _dt.year();
  _month = _dt.month();
  _day = _dt.day();
  _hour = _dt.hour();
  _minute = _dt.minute();

  String stringMinute;
  if(_minute < 10)
  {
    stringMinute = "0" + String(_minute);
  }
  else
  {
    stringMinute = String(_minute);
  }

  String stringSecond;
  if(_second < 10)
  {
    stringSecond = "0" + String(_second);
  }
  else
  {
    stringSecond = String(_second);
  }

  _timeString = String(_year) + "/" + String(_month) + "/" + String(_day) + " " + String(_hour) + ":" + stringMinute + ":" + stringSecond;

  return _timeString;
}

void TimeRTC::SetTime(DateTime dt)
{
  _rtc.setYear(dt.year() - 2000);
  _rtc.setMonth(dt.month());
  _rtc.setDate(dt.day());
  _rtc.setHour(dt.hour());
  _rtc.setMinute(dt.minute());
  _rtc.setSecond(0);
}

DateTime TimeRTC::GetDateTime()
{
  return _dt;
}

boolean TimeRTC::IsTimeUpdated()
{
  return _isTimeUpdated;
}

boolean TimeRTC::IsTime(DateTime dt)
{
  return (dt.unixtime() == _dt.unixtime());
}

boolean TimeRTC::IsTimeGreater(DateTime dt)
{
  return (dt.unixtime() < _dt.unixtime());
}
 
boolean TimeRTC::IsTimeLower(DateTime dt)
{
  return (dt.unixtime() > _dt.unixtime());
}