#include <Arduino.h>

class Buzzer
{
private:
    int _pin;
    unsigned long _startMillis;
    int buzzerOn = false;

public:
    Buzzer(int pin);
    void Beep();
    void BeepDouble();
    void BeepLong();
    void BeepSetDefault();
};