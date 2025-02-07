#include <Arduino.h>

enum Buzzer_Status
{
    BEEP_IDLE,
    BEEP_SHORT,
    BEEP_DOUBLE,
    BEEP_LONG
};

class Buzzer_Class
{
private:
    enum Buzzer_Status buzzerStatus = BEEP_IDLE;
    uint32_t _pin;
    unsigned long beepMillis;
    bool buzzerState = false;
    uint8_t beepCount = 0;
    void InternalTone();
    void InternalNoTone();

public:
    uint16_t alarmBeepTime = 1000;
    void InitBuzzer(uint32_t pin);
    void Tick();
    void Single();
    void Double();
    void Long();
};

extern Buzzer_Class BUZZER;