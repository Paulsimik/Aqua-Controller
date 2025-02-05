#include <Arduino.h>
#include <DS3231.h>

class TimeRTC
{
private:
    DS3231 _rtc;
    DateTime _dt;
    bool _century, _h12, _pm;
    bool _isTimeUpdated;
    uint16_t _year;
    uint8_t _month, _day, _hour, _minute, _second;
    uint8_t _lastSecond = -1;
    String _timeString;

public:
    void Tick();
    String GetCurrentTimeStr();
    void SetTime(DateTime dt);
    DateTime GetDateTime();
    boolean IsTimeUpdated();
    boolean IsTime(DateTime dt);
    boolean IsTimeGreater(DateTime dt);
    boolean IsTimeLower(DateTime dt);
};
