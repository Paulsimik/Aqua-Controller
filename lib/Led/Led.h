#include <Arduino.h>

class Led
{
private:
    int _pin;
    int _duty;
    uint8_t _currentDuty = 0;
    int _currentAnalog = 0;
    int _rampUp;
    int _rampDown;
    bool _isEnable;
    bool _start = false;
    bool _stop = false;
    unsigned long _everyMillisStart;
    unsigned long _everyMillisStop;
    unsigned long _prevMillis = 0;
    
public:
    Led(int pin);
    void Tick();
    void SetParameters(int duty, int rampUp, int rampDown);
    void Enable();
    void Disable();
    void Start();
    void Stop();
    void UpdateDuty(int duty);
    void Manual();
    boolean IsEnable();
    uint8_t GetCurrentDuty();
};