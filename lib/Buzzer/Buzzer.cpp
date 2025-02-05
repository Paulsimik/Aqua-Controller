#include "Buzzer.h"

Buzzer::Buzzer(int pin)
{
    _pin = pin;
    pinMode(_pin, OUTPUT);
}

void Buzzer::Beep()
{
    digitalWrite(_pin, HIGH);
    delay(5);
    digitalWrite(_pin, LOW);
}

void Buzzer::BeepDouble()
{
    digitalWrite(_pin, HIGH);
    delay(50);
    digitalWrite(_pin, LOW);
    delay(15);
    digitalWrite(_pin, HIGH);
    delay(50);
    digitalWrite(_pin, LOW);
}

void Buzzer::BeepLong()
{
    digitalWrite(_pin, HIGH);
    delay(300);
    digitalWrite(_pin, LOW);
}

void Buzzer::BeepSetDefault()
{
    digitalWrite(_pin, HIGH);
    delay(250);
    digitalWrite(_pin, LOW);
    delay(100);
    digitalWrite(_pin, HIGH);
    delay(250);
    digitalWrite(_pin, LOW);
    delay(100);
    digitalWrite(_pin, HIGH);
    delay(250);
    digitalWrite(_pin, LOW);
}