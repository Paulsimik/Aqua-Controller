#include <Arduino.h>

class Pump
{
private:
    int _pin;
    int _duty;
    unsigned long _pumpOnTime;
    bool _isEnable = false;
    unsigned long _startMillis;
    bool _isCycleComplete = false;

public:
    Pump(int pin);
    void Tick();
    void Enable();
    void Disable();
    void Start();
    void SetParameters(int duty, float volume, float calibrationOffset);
    boolean IsEnable();
    boolean IsCycleComplete();
};