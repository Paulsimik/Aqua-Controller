#include "Led.h"

Led::Led(int pin)
{
    _pin = pin;
    pinMode(_pin, OUTPUT);
    Disable();
}

void Led::Tick()
{
    if(_start)
    {
        if(millis() - _prevMillis >= _everyMillisStart)
        {
            _currentAnalog++;
            if(_currentAnalog > _duty)
            {
                _isEnable = true;
                _start = false;
                _prevMillis = 0;
                return;
            }

            analogWrite(_pin, _currentAnalog);
            _currentDuty = (uint8_t)(_currentAnalog / 40.95F);
            _prevMillis = millis();
        }
    }
    else if (_stop)
    {
        if(millis() - _prevMillis >= _everyMillisStop)
        {
            _currentAnalog--;
            if(_currentAnalog < 0)
            {
                _isEnable = false;
                _stop = false;
                _prevMillis = 0;
                return;
            }

            analogWrite(_pin, _currentAnalog);
            _currentDuty = (uint8_t)(_currentAnalog / 40.95F);
            _prevMillis = millis();
        }
    }
}

void Led::SetParameters(int duty, int rampUp, int rampDown)
{
    _duty = (int)(40.95F * duty);
    _rampUp = rampUp * 60;
    _rampDown = rampDown * 60;

    _everyMillisStart = ((float)_rampUp / _duty) * 1000;
    _everyMillisStop = ((float)_rampDown / _duty) * 1000;
}

void Led::Enable()
{
    _isEnable = true;
    for(_currentAnalog = 0; _currentAnalog <= _duty;)
    {
        if(millis() - _prevMillis >= 5)
        {
            _currentAnalog++;
            if(_currentAnalog > _duty)
            {
                _prevMillis = 0;
                return;
            }

            analogWrite(_pin, _currentAnalog);
            _currentDuty = (uint8_t)(_currentAnalog / 40.95F);
            _prevMillis = millis();
        }
    }
}

void Led::Disable()
{
    _isEnable = false;
    analogWrite(_pin, 0);
}

void Led::Start()
{
    _start = true;
}

void Led::Stop()
{
    _stop = true;
}

void Led::UpdateDuty(int duty)
{
    if(!_isEnable)
        return; 

    _duty = (int)(40.95F * duty);
    _currentAnalog = _duty;
    _currentDuty = duty;
    analogWrite(_pin, _duty);
}

void Led::Manual()
{
    if(!_isEnable)
    {
        _isEnable = true;
        for(int i = 0; i <= _duty; i++)
        {
            _currentAnalog = i;
            analogWrite(_pin, _currentAnalog);
            _currentDuty = (uint8_t)(_currentAnalog / 40.95F);
            delay(1);
        }
    }
    else
    {
        for(int i = _duty; i >= 0; i--)
        {
            _currentAnalog = i;
            analogWrite(_pin, _currentAnalog);
            _currentDuty = (uint8_t)(_currentAnalog / 40.95F);
            delay(2);
        }

        _isEnable = false;
    }
}

boolean Led::IsEnable()
{
    return _isEnable;
}

uint8_t Led::GetCurrentDuty()
{
    return _currentDuty;
}