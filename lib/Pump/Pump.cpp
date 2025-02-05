#include "Pump.h"

Pump::Pump(int pin)
{
    _pin = pin;
    pinMode(_pin, OUTPUT);
    Disable();
}

void Pump::Tick()
{
    if(_isEnable && (millis() - _startMillis >= _pumpOnTime))
    {
        Pump::Disable();
        _isCycleComplete = true;
        return;
    }

    _isCycleComplete = false;
}

void Pump::SetParameters(int duty, float volume, float calibrationOffset)
{   
    _duty = (int)(40.95F * duty);
    _pumpOnTime = volume * 1000;

    if(calibrationOffset > 0.0F)
    {
        float value = 5 / calibrationOffset;
        _pumpOnTime = long(value * volume) * 1000;
    }
}

void Pump::Enable()
{
    _isEnable = true;
    analogWrite(_pin, _duty);
}

void Pump::Disable()
{
    _isEnable = false;
    analogWrite(_pin, 0);
}

void Pump::Start()
{
    _isEnable = true;
    _isCycleComplete = false;
    _startMillis = millis();
    analogWrite(_pin, _duty);
}

boolean Pump::IsEnable()
{
    return _isEnable;
}

boolean Pump::IsCycleComplete()
{
    return _isCycleComplete;
}