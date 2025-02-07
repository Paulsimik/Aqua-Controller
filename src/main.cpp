//##############################//
//        Aqua Controller       //
//          Version 1.4         //
//            By Paul           //
//##############################//

#include <Arduino.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <TimeRTC.h>
#include <Pump.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <RotaryEncoder.h>
#include <Buzzer.h>
#include <Led.h>
#include <OneButton.h>
#include <SPI.h>

#define SD_PIN            PA4
#define ENCODER_A         PA12
#define ENCODER_B         PA11
#define RESERVED_OUTPUT   PB0
#define BUZZER_PIN        PA8

RotaryEncoder *encoder = nullptr;
TimeRTC timeRTC;
OneButton btnOk(PA15);
Pump pump_1(PA0);
Pump pump_2(PA1);
Pump pump_3(PA2);
Pump pump_4(PA3);
Led whiteLed(PA9);
Led colorLed(PA10);

#define DISP_ITEM_ROWS 3
#define DISP_CHAR_WIDTH 20
#define PACING_MS 10
#define FLASH_RST_CNT 10
#define WAKEUP 150
#define BACKLIGHT 300

enum pageType
{
  MENU_HOME,
  MENU_MAIN,
  MENU_LED_WHITE,
  MENU_LED_COLOR,
  MENU_PUMP_1,
  MENU_PUMP_1_CALIBRATION,
  MENU_PUMP_2,
  MENU_PUMP_2_CALIBRATION,
  MENU_PUMP_3,
  MENU_PUMP_3_CALIBRATION,
  MENU_PUMP_4,
  MENU_PUMP_4_CALIBRATION,
  MENU_SETTINGS
};

enum pageType currPage = MENU_HOME;
void Page_MenuHome();
void Page_MenuMain();
void Page_MenuLedWhite();
void Page_MenuLedColor();
void Page_Pump_1();
void Page_Pump_1_Calibration();
void Page_Pump_2();
void Page_Pump_2_Calibration();
void Page_Pump_3();
void Page_Pump_3_Calibration();
void Page_Pump_4();
void Page_Pump_4_Calibration();
void Page_MenuSettings();

// VARIABLES ------------------------------------------
DateTime currDateTime;
bool pump1EnableOn = true;
bool pump2EnableOn = true;
bool pump3EnableOn = true;
bool pump4EnableOn = true;
bool whiteLedOn = true;
bool colorLedOn = true;
bool wakeUp = true;
bool noBacklight = false;
unsigned long wakeUpMillis;
bool warningVolumeBottle = false;

// MENU INTERNALS -------------------------------------
uint32_t loopStartMs;
bool updateAllItems;
bool updateItemValue;
bool updateValues;
uint8_t itemCnt;
uint8_t pntrPos;
uint8_t dispOffset;
uint8_t root_pntrPos = 1;
uint8_t root_dispOffset = 0;
uint8_t flashCntr;
bool flashIsOn;
bool editMode = false;
int encoderPos;
uint8_t step = 1;
bool isLongPress = false;
bool isDoubleClick = false;
bool isClick = false;

void InitMenuPage(String title, uint8_t itemCount);
void CaptureButtonDownStates();
void AdjustBoolean(boolean *v);
void AdjustUint8_t(uint8_t *v, uint8_t min, uint8_t max);
void AdjustUint16_t(uint16_t *v, uint16_t min, uint16_t max);
void AdjustFloat(float *v, float min, float max);
void AdjustTime(byte *hour, byte *minute);
void DoPointerNavigation();
bool IsFlashChanged();
void PacintWait();
bool MenuItemPrintable(uint8_t xPos, uint8_t yPos);
void CheckPositionEncoder();
void IsLongPressStart();
void IsLongPressStop();
void IsDoubleClick();
void IsClick();
void Functions();
void CheckPumpOn();
void CheckLedOn();
void CheckLedRepeatOn();
void WakeUp();
void VolumeBottle(float *volumeBottle, float volume);

// PRINT TOOLS -------------------------------------
void PrintPointer();
void PrintEditPoint();
void PrintOnOff(bool val);
void PrintChars(uint8_t cnt, char c);
uint8_t GetUint32_tCharCnt(uint32_t value);
uint8_t GetFloatCharCnt(float value);
String GetTimeString(byte hour, byte minute);
void PrintUint8_tAtWidth(uint32_t value, uint8_t width, char c, boolean isRight);
void PrintFloatAtWidth(float value, uint8_t width, char c, boolean isRight);
void PrintTimeString(byte hour, byte minute);

// SETTINGS -------------------------------------
struct Configuration
{
  // WHITE LED
  uint8_t whiteLed_onTimeHour = 12;
  uint8_t whiteLed_onTimeMinute = 0;
  uint8_t whiteLed_offTimeHour = 20;
  uint8_t whiteLed_offTimeMinute = 0;
  uint8_t whiteLed_rampUp = 30;
  uint8_t whiteLed_rampDown = 30;
  uint8_t whiteLed_maxDuty = 100;

  // COLOR LED
  uint8_t colorLed_onTimeHour = 12;
  uint8_t colorLed_onTimeMinute = 0;
  uint8_t colorLed_offTimeHour = 20;
  uint8_t colorLed_offTimeMinute = 0;
  uint8_t colorLed_rampUp = 30;
  uint8_t colorLed_rampDown = 30;
  uint8_t colorLed_maxDuty = 100;

  // PUMP 1
  String pump1_name = "------FE-------";
  uint8_t pump1_onTimeHour = 12;
  uint8_t pump1_onTimeMinute = 0;
  uint8_t pump1_duty = 100;
  float pump1_volume_bottle = 450;
  float pump1_volume = 5;
  float pump1_calibrationOffset = 0;
  bool pump1_enable = false;

  // PUMP 2
  String pump2_name = "-----Tropica-------";
  uint8_t pump2_onTimeHour = 12;
  uint8_t pump2_onTimeMinute = 0;
  uint8_t pump2_duty = 100;
  float pump2_volume_bottle = 450;
  float pump2_volume = 5;
  float pump2_calibrationOffset = 0;
  bool pump2_enable = false;

  // PUMP 3
  String pump3_name = "-------------------";
  uint8_t pump3_onTimeHour = 12;
  uint8_t pump3_onTimeMinute = 0;
  uint8_t pump3_duty = 100;
  float pump3_volume_bottle = 450;
  float pump3_volume = 5;
  float pump3_calibrationOffset = 0;
  bool pump3_enable = false;

  // PUMP 4
  String pump4_name = "--------CO2--------";
  uint8_t pump4_onTimeHour = 12;
  uint8_t pump4_onTimeMinute = 0;
  uint8_t pump4_duty = 100;
  float pump4_volume_bottle = 450;
  float pump4_volume = 5;
  float pump4_calibrationOffset = 0;
  bool pump4_enable = false;

  // RTC
  uint16_t years = 2024;
  uint8_t months = 1;
  uint8_t days = 1;
  uint8_t hours = 0;
  uint8_t minutes = 0;
};

Configuration _config;
const char* fileName = "/config.txt";
void Set_Defaults();
void SD_Init();
void SD_Load();
void SD_Save();

// DISPLAY -------------------------------------
hd44780_I2Cexp lcd;
byte arrowSymbol[] = 
{
  B00000,
  B00100,
  B00110,
  B11111,
  B00110,
  B00100,
  B00000,
  B00000
 };
byte editSymbol[] = 
{
  B00000,
  B01110,
  B10001,
  B10101,
  B10001,
  B01110,
  B00000,
  B00000
};

void LCD_Init();

// =======================================================================//
//                                  SETUP                                 //
// =======================================================================//
void setup() 
{
  Wire.begin();
  SPI.begin();
  BUZZER.InitBuzzer(BUZZER_PIN);

  LCD_Init();
  SD_Init();
  analogWriteResolution(12);

  btnOk.attachClick(IsClick);
  btnOk.attachDoubleClick(IsDoubleClick);
  btnOk.attachLongPressStart(IsLongPressStart);
  btnOk.attachLongPressStop(IsLongPressStop);

  encoder = new RotaryEncoder(ENCODER_A, ENCODER_B, RotaryEncoder::LatchMode::TWO03);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), CheckPositionEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B), CheckPositionEncoder, CHANGE);

  colorLed.SetParameters(_config.colorLed_maxDuty, _config.colorLed_rampUp, _config.colorLed_rampDown);
  whiteLed.SetParameters(_config.whiteLed_maxDuty, _config.whiteLed_rampUp, _config.whiteLed_rampDown);
  pump_1.SetParameters(_config.pump1_duty, _config.pump1_volume, _config.pump1_calibrationOffset);
  pump_2.SetParameters(_config.pump2_duty, _config.pump2_volume, _config.pump2_calibrationOffset);
  pump_3.SetParameters(_config.pump3_duty, _config.pump3_volume, _config.pump3_calibrationOffset);
  pump_4.SetParameters(_config.pump4_duty, _config.pump4_volume, _config.pump4_calibrationOffset);

  timeRTC.Tick();
  CheckLedRepeatOn();
}

// =======================================================================//
//                                 BUTTONS                                //
// =======================================================================//

void IsLongPressStart()
{
  isLongPress = true;
  wakeUp = true;
}

void IsLongPressStop()
{
  isLongPress = false;
  wakeUp = true;
}

void IsDoubleClick()
{
  isDoubleClick = true;
  wakeUp = true;
}

void IsClick()
{
  isClick = true;
  wakeUp = true;
}

// =======================================================================//
//                                  LOOP                                  //
// =======================================================================//
void loop() 
{
  switch (currPage)
  {
    case MENU_HOME: Page_MenuHome(); break;
    case MENU_MAIN: Page_MenuMain(); break;
    case MENU_LED_WHITE: Page_MenuLedWhite(); break;
    case MENU_LED_COLOR: Page_MenuLedColor(); break;
    case MENU_PUMP_1: Page_Pump_1(); break;
    case MENU_PUMP_1_CALIBRATION: Page_Pump_1_Calibration(); break;
    case MENU_PUMP_2: Page_Pump_2(); break;
    case MENU_PUMP_2_CALIBRATION: Page_Pump_2_Calibration(); break;
    case MENU_PUMP_3: Page_Pump_3(); break;
    case MENU_PUMP_3_CALIBRATION: Page_Pump_3_Calibration(); break;
    case MENU_PUMP_4: Page_Pump_4(); break;
    case MENU_PUMP_4_CALIBRATION: Page_Pump_4_Calibration(); break;
    case MENU_SETTINGS: Page_MenuSettings(); break;
  }
}

// =======================================================================//
//                                FUNCTIONS                               //
// =======================================================================//
void Functions()
{
  timeRTC.Tick();
  currDateTime = timeRTC.GetDateTime();
  whiteLed.Tick();
  colorLed.Tick();
  pump_1.Tick();
  pump_2.Tick();
  pump_3.Tick();
  pump_4.Tick();

  CheckPumpOn();
  CheckLedOn();
  CheckLedRepeatOn();
}

void CheckPumpOn()
{
  // PUMP 1 CHECK START
  if(_config.pump1_enable && pump1EnableOn && timeRTC.IsTime(DateTime(currDateTime.year(), currDateTime.month(), currDateTime.day(), _config.pump1_onTimeHour, _config.pump1_onTimeMinute, 0)))
  {
    pump1EnableOn = false;
    pump_1.Start();
    VolumeBottle(&_config.pump1_volume_bottle, _config.pump1_volume);
  }
  else if(!pump1EnableOn && timeRTC.IsTimeLower(DateTime(currDateTime.year(), currDateTime.month(), currDateTime.day(), _config.pump1_onTimeHour, _config.pump1_onTimeMinute, 0)))
  {
    pump1EnableOn = true;
  }

  // PUMP 2 CHECK START
  if(_config.pump2_enable && pump2EnableOn && timeRTC.IsTime(DateTime(currDateTime.year(), currDateTime.month(), currDateTime.day(), _config.pump2_onTimeHour, _config.pump2_onTimeMinute, 0)))
  {
    pump2EnableOn = false;
    pump_2.Start();
    VolumeBottle(&_config.pump2_volume_bottle, _config.pump2_volume);
  }
  else if(!pump2EnableOn && timeRTC.IsTimeLower(DateTime(currDateTime.year(), currDateTime.month(), currDateTime.day(), _config.pump2_onTimeHour, _config.pump2_onTimeMinute, 0)))
  {
    pump2EnableOn = true;
  }

  // PUMP 3 CHECK START
  if(_config.pump3_enable && pump3EnableOn && timeRTC.IsTime(DateTime(currDateTime.year(), currDateTime.month(), currDateTime.day(), _config.pump3_onTimeHour, _config.pump3_onTimeMinute, 0)))
  {
    pump3EnableOn = false;
    pump_3.Start();
    VolumeBottle(&_config.pump3_volume_bottle, _config.pump3_volume);
  }
  else if(!pump3EnableOn && timeRTC.IsTimeLower(DateTime(currDateTime.year(), currDateTime.month(), currDateTime.day(), _config.pump3_onTimeHour, _config.pump3_onTimeMinute, 0)))
  {
    pump3EnableOn = true;
  }

  // PUMP 4 CHECK START
  if(_config.pump4_enable && pump4EnableOn && timeRTC.IsTime(DateTime(currDateTime.year(), currDateTime.month(), currDateTime.day(), _config.pump4_onTimeHour, _config.pump4_onTimeMinute, 0)))
  {
    pump4EnableOn = false;
    pump_4.Start();
    VolumeBottle(&_config.pump4_volume_bottle, _config.pump4_volume);
  }
  else if(!pump4EnableOn && timeRTC.IsTimeLower(DateTime(currDateTime.year(), currDateTime.month(), currDateTime.day(), _config.pump4_onTimeHour, _config.pump4_onTimeMinute, 0)))
  {
    pump4EnableOn = true;
  }

  // SAVE CONFIG PARAMETERS DUE PUMP VOLUME LEVEL CHANGED
  if(timeRTC.IsTime(DateTime(currDateTime.year(), currDateTime.month(), currDateTime.day(), _config.pump4_onTimeHour, _config.pump4_onTimeMinute + 5, 0)))
  {
    SD_Save();
  }
}

void CheckLedOn()
{
  // COLOR LED CHECK START
  if(colorLedOn && timeRTC.IsTime(DateTime(currDateTime.year(), currDateTime.month(), currDateTime.day(), _config.colorLed_onTimeHour, _config.colorLed_onTimeMinute, 0)))
  {
    colorLedOn = false;
    colorLed.Start();
  }

  // COLOR LED CHECK STOP
  if(!colorLedOn && timeRTC.IsTime(DateTime(currDateTime.year(), currDateTime.month(), currDateTime.day(), _config.colorLed_offTimeHour, _config.colorLed_offTimeMinute, 0)))
  {
    colorLedOn = true;
    colorLed.Stop();
  }

  // WHITE LED CHECK START
  if(whiteLedOn && timeRTC.IsTime(DateTime(currDateTime.year(), currDateTime.month(), currDateTime.day(), _config.whiteLed_onTimeHour, _config.whiteLed_onTimeMinute, 0)))
  {
    whiteLedOn = false;
    whiteLed.Start();
  }

  // WHITE LED CHECK STOP
  if(!whiteLedOn && timeRTC.IsTime(DateTime(currDateTime.year(), currDateTime.month(), currDateTime.day(), _config.whiteLed_offTimeHour, _config.whiteLed_offTimeMinute, 0)))
  {
    whiteLedOn = true;
    whiteLed.Stop();
  }
}

void CheckLedRepeatOn()
{
  if(colorLedOn 
  && (timeRTC.IsTimeGreater(DateTime(currDateTime.year(), currDateTime.month(), currDateTime.day(), _config.colorLed_onTimeHour, _config.colorLed_onTimeMinute, 0)) 
  && timeRTC.IsTimeLower(DateTime(currDateTime.year(), currDateTime.month(), currDateTime.day(), _config.colorLed_offTimeHour, _config.colorLed_offTimeMinute, 0))))
  {
    colorLedOn = false;
    colorLed.Enable();
  }

  if(whiteLedOn 
  && (timeRTC.IsTimeGreater(DateTime(currDateTime.year(), currDateTime.month(), currDateTime.day(), _config.whiteLed_onTimeHour, _config.whiteLed_onTimeMinute, 0)) 
  && timeRTC.IsTimeLower(DateTime(currDateTime.year(), currDateTime.month(), currDateTime.day(), _config.whiteLed_offTimeHour, _config.whiteLed_offTimeMinute, 0))))
  {
    whiteLedOn = false;
    whiteLed.Enable();
  }
}

void WakeUp()
{
  if(wakeUp)
  {
    wakeUp = false;
    noBacklight = true;
    wakeUpMillis = millis();
    lcd.backlight();
    return;
  }

  if(noBacklight && millis() - wakeUpMillis >= (BACKLIGHT * 1000))
  {
    noBacklight = false;
    lcd.noBacklight();
  }

  if(currPage != MENU_HOME && millis() - wakeUpMillis >= (WAKEUP * 1000))
  {
    currPage = MENU_HOME;
    root_pntrPos = 1; 
    root_dispOffset = 0;
    loop();
  }
}

// =======================================================================//
//                                MENU HOME                               //
// =======================================================================//
void Page_MenuHome()
{
  InitMenuPage(timeRTC.GetCurrentTimeStr(), 22);

  while (true)
  {
    Functions();
    BUZZER.Tick();

    if(timeRTC.IsTimeUpdated() || updateAllItems)
    {
      lcd.setCursor(1, 0);
      lcd.print(timeRTC.GetCurrentTimeStr() + "-");

      if(warningVolumeBottle)
      {
        if(MenuItemPrintable(1, 2)) {lcd.print("Low Level Bottle!");}
        return;
      }

      if(MenuItemPrintable(1, 1)) {lcd.print("Led White On " + GetTimeString(_config.whiteLed_onTimeHour, _config.whiteLed_onTimeMinute) + " ");}
      if(MenuItemPrintable(1, 2)) {lcd.print("Led White Off " + GetTimeString(_config.whiteLed_offTimeHour, _config.whiteLed_offTimeMinute));}
      if(MenuItemPrintable(1, 3)) {lcd.print("Led White Duty " + String(whiteLed.GetCurrentDuty()) + "%  ");}
      if(MenuItemPrintable(1, 4)) {lcd.print("Led Color On " + GetTimeString(_config.colorLed_onTimeHour, _config.colorLed_onTimeMinute)  + " ");}
      if(MenuItemPrintable(1, 5)) {lcd.print("Led Color Off " + GetTimeString(_config.colorLed_offTimeHour, _config.colorLed_offTimeMinute));}
      if(MenuItemPrintable(1, 6)) {lcd.print("Led Color Duty "  + String(colorLed.GetCurrentDuty()) + "%  ");}
      if(MenuItemPrintable(1, 7)) {lcd.print(_config.pump1_name);}
      if(MenuItemPrintable(1, 8)) {lcd.print("Pump 1 On " + GetTimeString(_config.pump1_onTimeHour, _config.pump1_onTimeMinute) + "  ");}
      if(MenuItemPrintable(1, 9)) {lcd.print("Pump 1 " + String(_config.pump1_volume) + "ml   ");}
      if(MenuItemPrintable(1, 10)) {lcd.print("Volume Bottle: " + String(_config.pump1_volume_bottle) + "ml");}
      if(MenuItemPrintable(1, 11)) {lcd.print(_config.pump2_name);}
      if(MenuItemPrintable(1, 12)) {lcd.print("Pump 2 On " + GetTimeString(_config.pump2_onTimeHour, _config.pump2_onTimeMinute) + "  ");}
      if(MenuItemPrintable(1, 13)){lcd.print("Pump 2 " + String(_config.pump2_volume) + "ml   ");}
      if(MenuItemPrintable(1, 14)) {lcd.print("Volume Bottle: " + String(_config.pump2_volume_bottle) + "ml");}
      if(MenuItemPrintable(1, 15)) {lcd.print(_config.pump3_name);}
      if(MenuItemPrintable(1, 16)){lcd.print("Pump 3 On " + GetTimeString(_config.pump3_onTimeHour, _config.pump3_onTimeMinute) + "  ");}
      if(MenuItemPrintable(1, 17)){lcd.print("Pump 3 " + String(_config.pump3_volume) + "ml   ");}
      if(MenuItemPrintable(1, 18)) {lcd.print("Volume Bottle: " + String(_config.pump3_volume_bottle) + "ml");}
      if(MenuItemPrintable(1, 19)) {lcd.print(_config.pump4_name);}
      if(MenuItemPrintable(1, 20)){lcd.print("Pump 4 On " + GetTimeString(_config.pump4_onTimeHour, _config.pump4_onTimeMinute) + "  ");}
      if(MenuItemPrintable(1, 21)){lcd.print("Pump 4 " + String(_config.pump4_volume) + "ml   ");}
      if(MenuItemPrintable(1, 22)) {lcd.print("Volume Bottle: " + String(_config.pump4_volume_bottle) + "ml");}
    }

    if(IsFlashChanged())
    {
      PrintPointer();
    }

    CaptureButtonDownStates();

    if(isLongPress)
    {
      isLongPress = false;
      BUZZER.Double();
      whiteLed.Manual();
      colorLed.Manual();
    }

    if(isClick)
    {
      BUZZER.Double();
      currPage = MENU_MAIN;
      isClick = false;
      return;
    }

    DoPointerNavigation();
    PacintWait();
  }
}

// =======================================================================//
//                                MENU MAIN                               //
// =======================================================================//
void Page_MenuMain()
{
  InitMenuPage(F("Main Menu"), 8);

  pntrPos = root_pntrPos;
  dispOffset = root_dispOffset;

  while (true)
  {
    if(updateAllItems)
    {
      if(MenuItemPrintable(1, 1)){lcd.print("Led White Menu     ");}
      if(MenuItemPrintable(1, 2)){lcd.print("Led Color Menu     ");}
      if(MenuItemPrintable(1, 3)){lcd.print("Pump #1 Menu       ");}
      if(MenuItemPrintable(1, 4)){lcd.print("Pump #2 Menu       ");}
      if(MenuItemPrintable(1, 5)){lcd.print("Pump #3 Menu       ");}
      if(MenuItemPrintable(1, 6)){lcd.print("Pump #4 Menu       ");}
      if(MenuItemPrintable(1, 7)){lcd.print("Settings           ");}
      if(MenuItemPrintable(1, 8)){lcd.print("Back               ");}
    }

    if(IsFlashChanged())
    {
      PrintPointer();
    }

    updateAllItems = false;
    CaptureButtonDownStates();

    if(isClick)
    {
      isClick = false;
      BUZZER.Double();
      root_pntrPos = pntrPos;
      root_dispOffset = dispOffset;

      switch (pntrPos)
      {
        case 1: currPage = MENU_LED_WHITE; return;
        case 2: currPage = MENU_LED_COLOR; return;
        case 3: currPage = MENU_PUMP_1; return;
        case 4: currPage = MENU_PUMP_2; return;
        case 5: currPage = MENU_PUMP_3; return;
        case 6: currPage = MENU_PUMP_4; return;
        case 7: currPage = MENU_SETTINGS; return;
        case 8: currPage = MENU_HOME; root_pntrPos = 1; root_dispOffset = 0; return;
      }
    }

    DoPointerNavigation();
    PacintWait();
  }
}

// =======================================================================//
//                              MENU LED WHITE                            //
// =======================================================================//
void Page_MenuLedWhite()
{
  InitMenuPage(F("Led White"), 7);

  while (true)
  {
    if(updateAllItems)
    {
      if(MenuItemPrintable(1, 1)){lcd.print("On Time:           ");}
      if(MenuItemPrintable(1, 2)){lcd.print("Off Time:          ");}
      if(MenuItemPrintable(1, 3)){lcd.print("Ramp Up:           ");}
      if(MenuItemPrintable(1, 4)){lcd.print("Ramp Down:         ");}
      if(MenuItemPrintable(1, 5)){lcd.print("Max Duty:          ");}
      if(MenuItemPrintable(1, 6)){lcd.print("Save               ");}
      if(MenuItemPrintable(1, 7)){lcd.print("Back               ");}
    }

    if(updateAllItems || updateItemValue)
    {
      if(MenuItemPrintable(10, 1)){PrintTimeString(_config.whiteLed_onTimeHour, _config.whiteLed_onTimeMinute);}
      if(MenuItemPrintable(11, 2)){PrintTimeString(_config.whiteLed_offTimeHour, _config.whiteLed_offTimeMinute);}
      if(MenuItemPrintable(10, 3)){PrintUint8_tAtWidth(_config.whiteLed_rampUp, 1, ' ', false); lcd.print("min ");}
      if(MenuItemPrintable(12, 4)){PrintUint8_tAtWidth(_config.whiteLed_rampDown, 1, ' ', false); lcd.print("min ");}
      if(MenuItemPrintable(11, 5)){PrintUint8_tAtWidth(_config.whiteLed_maxDuty, 1, ' ', false); lcd.print("% ");}
    }

    if(updateValues)
    {
      whiteLed.SetParameters(_config.whiteLed_maxDuty, _config.whiteLed_rampUp, _config.whiteLed_rampDown);
    }

    if(IsFlashChanged())
    {
      if(editMode)
      {
        PrintEditPoint();
      }
      else
      {
        PrintPointer();
      }
    }

    updateAllItems = false;
    updateItemValue = false;
    updateValues = false;
    CaptureButtonDownStates();

    if(isClick)
    {
      isClick = false;

      switch (pntrPos)
      {
        case 6: 
          BUZZER.Long();
          SD_Save(); 
          BUZZER.Long();
          Page_MenuLedWhite();
          return;
        case 7: 
          currPage = MENU_MAIN; 
          BUZZER.Double();
          return;
      }
    }

    if(isLongPress && !(pntrPos == 6 || pntrPos == 7))
    {
      isLongPress = false;
      updateValues = true;
      editMode = !editMode;
      BUZZER.Double();
    }

    if(editMode)
    {
      encoder->tick();
      encoderPos = encoder->getPosition();
      if(encoderPos == 2)
      {
        encoderPos = encoderPos / 2;
      }

      switch (pntrPos)
      {
        case 1: AdjustTime(&_config.whiteLed_onTimeHour, &_config.whiteLed_onTimeMinute); break;
        case 2: AdjustTime(&_config.whiteLed_offTimeHour, &_config.whiteLed_offTimeMinute); break;
        case 3: AdjustUint8_t(&_config.whiteLed_rampUp, 1, 180); break;
        case 4: AdjustUint8_t(&_config.whiteLed_rampDown, 1, 180); break;
        case 5: AdjustUint8_t(&_config.whiteLed_maxDuty, 1, 100); whiteLed.UpdateDuty(_config.whiteLed_maxDuty); break;
      }

      encoder->setPosition(0);
    }
    else
    {
      DoPointerNavigation();
    }

    PacintWait();
  }
}

// =======================================================================//
//                              MENU LED COLOR                            //
// =======================================================================//
void Page_MenuLedColor()
{
  InitMenuPage(F("Led Color"), 7);

  while (true)
  {
    if(updateAllItems)
    {
      if(MenuItemPrintable(1, 1)){lcd.print("On Time:           ");}
      if(MenuItemPrintable(1, 2)){lcd.print("Off Time:          ");}
      if(MenuItemPrintable(1, 3)){lcd.print("Ramp Up:           ");}
      if(MenuItemPrintable(1, 4)){lcd.print("Ramp Down:         ");}
      if(MenuItemPrintable(1, 5)){lcd.print("Max Duty:          ");}
      if(MenuItemPrintable(1, 6)){lcd.print("Save               ");}
      if(MenuItemPrintable(1, 7)){lcd.print("Back               ");}
    }

    if(updateAllItems || updateItemValue)
    {
      if(MenuItemPrintable(10, 1)){PrintTimeString(_config.colorLed_onTimeHour, _config.colorLed_onTimeMinute);}
      if(MenuItemPrintable(11, 2)){PrintTimeString(_config.colorLed_offTimeHour, _config.colorLed_offTimeMinute);}
      if(MenuItemPrintable(10, 3)){PrintUint8_tAtWidth(_config.colorLed_rampUp, 1, ' ', false); lcd.print("min ");}
      if(MenuItemPrintable(12, 4)){PrintUint8_tAtWidth(_config.colorLed_rampDown, 1, ' ', false); lcd.print("min ");}
      if(MenuItemPrintable(11, 5)){PrintUint8_tAtWidth(_config.colorLed_maxDuty, 1, ' ', false); lcd.print("% ");}
    }

    if(updateValues)
    {
      colorLed.SetParameters(_config.colorLed_maxDuty, _config.colorLed_rampUp, _config.colorLed_rampDown);
    }

    if(IsFlashChanged())
    {
      if(editMode)
      {
        PrintEditPoint();
      }
      else
      {
        PrintPointer();
      }
    }

    updateAllItems = false;
    updateItemValue = false;
    updateValues = false;
    CaptureButtonDownStates();

    if(isClick)
    {
      isClick = false;

      switch (pntrPos)
      {
        case 6: 
          BUZZER.Long();
          SD_Save(); 
          BUZZER.Long();
          Page_MenuLedColor();
          break;
        case 7: 
          currPage = MENU_MAIN; 
          BUZZER.Double();
          return;
      }
    }

    if(isLongPress && !(pntrPos == 6 || pntrPos == 7))
    {
      isLongPress = false;
      updateValues = true;
      editMode = !editMode;
      BUZZER.Double();
    }

    if(editMode)
    {
      encoder->tick();
      encoderPos = encoder->getPosition();
      if(encoderPos == 2)
      {
        encoderPos = encoderPos / 2;
      }

      switch (pntrPos)
      {
        case 1: AdjustTime(&_config.colorLed_onTimeHour, &_config.colorLed_onTimeMinute); break;
        case 2: AdjustTime(&_config.colorLed_offTimeHour, &_config.colorLed_offTimeMinute); break;
        case 3: AdjustUint8_t(&_config.colorLed_rampUp, 1, 120); break;
        case 4: AdjustUint8_t(&_config.colorLed_rampDown, 1, 120); break;
        case 5: AdjustUint8_t(&_config.colorLed_maxDuty, 1, 100); colorLed.UpdateDuty(_config.colorLed_maxDuty); break;
      }

      encoder->setPosition(0);
    }
    else
    {
      DoPointerNavigation();
    }

    PacintWait();
  }
}

// =======================================================================//
//                               MENU PUMP 1                              //
// =======================================================================//
void Page_Pump_1()
{
  InitMenuPage(F("Pump #1"), 9);

  while (true)
  {
    if(updateAllItems)
    {
      if(MenuItemPrintable(1, 1)){lcd.print("On Time:           ");}
      if(MenuItemPrintable(1, 2)){lcd.print("Volume:            ");}
      if(MenuItemPrintable(1, 3)){lcd.print("Duty:              ");}
      if(MenuItemPrintable(1, 4)){lcd.print("Pump:              ");}
      if(MenuItemPrintable(1, 5)){lcd.print("Fertilize Start    ");}
      if(MenuItemPrintable(1, 6)){lcd.print("Start Calibration  ");}
      if(MenuItemPrintable(1, 7)){lcd.print("Reset Bottle Volume");}
      if(MenuItemPrintable(1, 8)){lcd.print("Save               ");}
      if(MenuItemPrintable(1, 9)){lcd.print("Back               ");}
    }

    if(updateAllItems || updateItemValue)
    {
      if(MenuItemPrintable(10, 1))  {PrintTimeString(_config.pump1_onTimeHour, _config.pump1_onTimeMinute);}
      if(MenuItemPrintable(9, 2))   {PrintFloatAtWidth(_config.pump1_volume, 1, ' ', false); lcd.print("ml ");}
      if(MenuItemPrintable(7, 3))   {PrintUint8_tAtWidth(_config.pump1_duty, 1, ' ', false); lcd.print("% ");}
      if(MenuItemPrintable(7, 4))   {PrintOnOff(_config.pump1_enable);}
    }

    if(updateValues)
    {
      pump_1.SetParameters(_config.pump1_duty, _config.pump1_volume, _config.pump1_calibrationOffset);
    }

    if(IsFlashChanged())
    {
      if(editMode)
      {
        PrintEditPoint();
      }
      else
      {
        PrintPointer();
      }
    }

    updateAllItems = false;
    updateItemValue = false;
    updateValues = false;
    CaptureButtonDownStates();

    while(pump_1.IsEnable())
    {
      pump_1.Tick();
    }

    if(isClick)
    {
      isClick = false;

      switch (pntrPos)
      {
        case 5:
          BUZZER.Single();
          pump_1.Start();
          VolumeBottle(&_config.pump1_volume_bottle, _config.pump1_volume);
          break;
        case 6:
          encoder->setPosition(0);
          currPage = MENU_PUMP_1_CALIBRATION;
          BUZZER.Double();
          return;
        case 8: 
          BUZZER.Long();
          SD_Save(); 
          BUZZER.Long();
          Page_Pump_1();
          break;
        case 9: 
          currPage = MENU_MAIN; 
          BUZZER.Double();
          return;
      }
    }

    if(isLongPress && !(pntrPos == 5 || pntrPos == 6 || pntrPos == 8 || pntrPos == 9))
    {
      isLongPress = false;

      if(pntrPos == 7)
      {
        _config.pump1_volume_bottle = 450;
        warningVolumeBottle = false;
        BUZZER.Long();
        return;
      }

      updateValues = true;
      editMode = !editMode;
      BUZZER.Double();
    }

    if(editMode)
    {
      encoder->tick();
      encoderPos = encoder->getPosition();
      if(encoderPos == 2)
      {
        encoderPos = encoderPos / 2;
      }

      switch (pntrPos)
      {
        case 1: AdjustTime(&_config.pump1_onTimeHour, &_config.pump1_onTimeMinute); break;
        case 2: AdjustFloat(&_config.pump1_volume, 0.2, 50); break;
        case 3: AdjustUint8_t(&_config.pump1_duty, 1, 100); break;
        case 4: AdjustBoolean(&_config.pump1_enable); break;
      }

      encoder->setPosition(0);
    }
    else
    {
      DoPointerNavigation();
    }

    PacintWait();
  }
}

// =======================================================================//
//                        MENU PUMP 1 CALIBRARTION                        //
// =======================================================================//
void Page_Pump_1_Calibration()
{
  InitMenuPage(F("Pump 1 Calibration"), 0);
  pump_1.SetParameters(_config.pump1_duty , 5, 0);

  // ########### STEP 1 ############
  while (step == 1)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #1       ");
      lcd.setCursor(0, 2);
      lcd.print("Long Press OK       ");
      lcd.setCursor(0, 3);
      lcd.print("for fill            ");
    }

    updateAllItems = false;
    CaptureButtonDownStates();

    if(isLongPress)
    {
      pump_1.Enable();
    }

    if(!isLongPress && pump_1.IsEnable())
    {
      pump_1.Disable();
    }

    if(isClick)
    {
      isClick = false;
      BUZZER.Single();
      updateAllItems = true;
      step = 2;
    }
  }

  // ########### STEP 2 ############
  while (step == 2)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #2       ");
      lcd.setCursor(0, 2);
      lcd.print("Press OK and measure");
      lcd.setCursor(0, 3);
      lcd.print("current volume level");
    }

    updateAllItems = false;
    CaptureButtonDownStates();
    pump_1.Tick();

    if(isClick && !pump_1.IsEnable())
    {
      isClick = false;
      pump_1.Start();
    }

    if(pump_1.IsCycleComplete())
    {
      BUZZER.Double();
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      updateAllItems = true;
      step = 3;
    }
  }

  // ########### STEP 3 ############
  while (step == 3)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #3       ");
      lcd.setCursor(0, 2);
      lcd.print("Set current volume  ");
    }

    if(updateAllItems || updateItemValue)
    {
      lcd.setCursor(0, 3);
      lcd.print(String(_config.pump1_calibrationOffset) + "ml ");
    }

    updateAllItems = false;
    updateItemValue = false;
    CaptureButtonDownStates();

    encoder->tick();
    encoderPos = encoder->getPosition();
    if(encoderPos == 2)
    {
      encoderPos = encoderPos / 2;
    }

    AdjustFloat(&_config.pump1_calibrationOffset, 0.0F, 50.0F);

    if(isClick)
    {
      isClick = false;
      pump_1.SetParameters(_config.pump1_duty, 5, _config.pump1_calibrationOffset);
      BUZZER.Single();
      updateAllItems = true;
      step = 4;
    }
  }

  // ########### STEP 4 ############
  while (step == 4)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #4       ");
      lcd.setCursor(0, 2);
      lcd.print("Run Test Fill       ");
      lcd.setCursor(0, 3);
      lcd.print("                    ");
    }

    updateAllItems = false;
    CaptureButtonDownStates();
    pump_1.Tick();

    if(isClick && !pump_1.IsEnable())
    {
      isClick = false;
      pump_1.Start();
    }

    if(pump_1.IsCycleComplete())
    {
      BUZZER.Double();
      updateAllItems = true;
      step = 5;
    }
  }

  // ########### STEP 5 ############
  while (step == 5)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #5       ");
      lcd.setCursor(0, 2);
      lcd.print("Long Press OK for  ");
      lcd.setCursor(0, 3);
      lcd.print("Repeat or OK Done  ");
    }

    updateAllItems = false;
    CaptureButtonDownStates();

    if(isLongPress)
    {
      BUZZER.Long();
      updateAllItems = true;
      step = 1;
    }

    if(isClick)
    {
      isClick = false;
      pump_1.SetParameters(_config.pump1_duty, _config.pump1_volume, _config.pump1_calibrationOffset);
      encoder->setPosition(0);
      BUZZER.Double();
      currPage = MENU_PUMP_1;
      step = 1;
    }
  }
}

// =======================================================================//
//                               MENU PUMP 2                              //
// =======================================================================//
void Page_Pump_2()
{
  InitMenuPage(F("Pump #2"), 9);

  while (true)
  {
    if(updateAllItems)
    {
      if(MenuItemPrintable(1, 1)){lcd.print("On Time:           ");}
      if(MenuItemPrintable(1, 2)){lcd.print("Volume:            ");}
      if(MenuItemPrintable(1, 3)){lcd.print("Duty:              ");}
      if(MenuItemPrintable(1, 4)){lcd.print("Pump:              ");}
      if(MenuItemPrintable(1, 5)){lcd.print("Fertilize Start    ");}
      if(MenuItemPrintable(1, 6)){lcd.print("Start Calibration  ");}
      if(MenuItemPrintable(1, 7)){lcd.print("Reset Bottle Volume");}
      if(MenuItemPrintable(1, 8)){lcd.print("Save               ");}
      if(MenuItemPrintable(1, 9)){lcd.print("Back               ");}
    }

    if(updateAllItems || updateItemValue)
    {
      if(MenuItemPrintable(10, 1))  {PrintTimeString(_config.pump2_onTimeHour, _config.pump2_onTimeMinute);}
      if(MenuItemPrintable(9, 2))   {PrintFloatAtWidth(_config.pump2_volume, 1, ' ', false); lcd.print("ml ");}
      if(MenuItemPrintable(7, 3))   {PrintUint8_tAtWidth(_config.pump2_duty, 1, ' ', false); lcd.print("% ");}
      if(MenuItemPrintable(7, 4))   {PrintOnOff(_config.pump2_enable);}
    }

    if(updateValues)
    {
      pump_2.SetParameters(_config.pump2_duty, _config.pump2_volume, _config.pump2_calibrationOffset);
    }

    if(IsFlashChanged())
    {
      if(editMode)
      {
        PrintEditPoint();
      }
      else
      {
        PrintPointer();
      }
    }

    updateAllItems = false;
    updateItemValue = false;
    updateValues = false;
    CaptureButtonDownStates();

    while(pump_2.IsEnable())
    {
      pump_2.Tick();
    }

    if(isClick)
    {
      isClick = false;

      switch (pntrPos)
      {
        case 5:
          BUZZER.Single();
          pump_2.Start();
          VolumeBottle(&_config.pump2_volume_bottle, _config.pump2_volume);
        break;
        case 6:
          encoder->setPosition(0);
          currPage = MENU_PUMP_2_CALIBRATION;
          BUZZER.Double();
          return;
        case 8: 
          BUZZER.Long();
          SD_Save(); 
          BUZZER.Long();
          Page_Pump_2();
          break;
        case 9: 
          currPage = MENU_MAIN; 
          BUZZER.Double();
          return;
      }
    }

    if(isLongPress && !(pntrPos == 5 || pntrPos == 6 || pntrPos == 8 || pntrPos == 9))
    {
      isLongPress = false;

      if(pntrPos == 7)
      {
        _config.pump2_volume_bottle = 450;
        warningVolumeBottle = false;
        BUZZER.Long();
        return;
      }

      updateValues = true;
      editMode = !editMode;
      BUZZER.Double();
    }

    if(editMode)
    {
      encoder->tick();
      encoderPos = encoder->getPosition();
      if(encoderPos == 2)
      {
        encoderPos = encoderPos / 2;
      }

      switch (pntrPos)
      {
        case 1: AdjustTime(&_config.pump2_onTimeHour, &_config.pump2_onTimeMinute); break;
        case 2: AdjustFloat(&_config.pump2_volume, 0.2, 50); break;
        case 3: AdjustUint8_t(&_config.pump2_duty, 1, 100); break;
        case 4: AdjustBoolean(&_config.pump2_enable); break;
      }

      encoder->setPosition(0);
    }
    else
    {
      DoPointerNavigation();
    }

    PacintWait();
  }
}

// =======================================================================//
//                        MENU PUMP 2 CALIBRARTION                        //
// =======================================================================//
void Page_Pump_2_Calibration()
{
  InitMenuPage(F("Pump 2 Calibration"), 0);
  pump_2.SetParameters(_config.pump2_duty, 5, 0);

  // ########### STEP 1 ############
  while (step == 1)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #1       ");
      lcd.setCursor(0, 2);
      lcd.print("Long Press OK       ");
      lcd.setCursor(0, 3);
      lcd.print("for fill            ");
    }

    updateAllItems = false;
    CaptureButtonDownStates();

    if(isLongPress)
    {
      pump_2.Enable();
    }

    if(!isLongPress && pump_2.IsEnable())
    {
      pump_2.Disable();
    }

    if(isClick)
    {
      isClick = false;
      BUZZER.Single();
      updateAllItems = true;
      step = 2;
    }
  }

  // ########### STEP 2 ############
  while (step == 2)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #2       ");
      lcd.setCursor(0, 2);
      lcd.print("Press OK and measure");
      lcd.setCursor(0, 3);
      lcd.print("current volume level");
    }

    updateAllItems = false;
    CaptureButtonDownStates();
    pump_2.Tick();

    if(isClick && !pump_2.IsEnable())
    {
      isClick = false;
      pump_2.Start();
    }

    if(pump_2.IsCycleComplete())
    {
      BUZZER.Double();
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      updateAllItems = true;
      step = 3;
    }
  }

  // ########### STEP 3 ############
  while (step == 3)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #3       ");
      lcd.setCursor(0, 2);
      lcd.print("Set current volume  ");
    }

    if(updateAllItems || updateItemValue)
    {
      lcd.setCursor(0, 3);
      lcd.print(String(_config.pump2_calibrationOffset) + "ml ");
    }

    updateAllItems = false;
    updateItemValue = false;
    CaptureButtonDownStates();

    encoder->tick();
    encoderPos = encoder->getPosition();
    if(encoderPos == 2)
    {
      encoderPos = encoderPos / 2;
    }

    AdjustFloat(&_config.pump2_calibrationOffset, 0.0F, 50.0F);

    if(isClick)
    {
      isClick = false;
      pump_2.SetParameters(_config.pump2_duty, 5, _config.pump2_calibrationOffset);
      BUZZER.Single();
      updateAllItems = true;
      step = 4;
    }
  }

  // ########### STEP 4 ############
  while (step == 4)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #4       ");
      lcd.setCursor(0, 2);
      lcd.print("Run Test Fill       ");
      lcd.setCursor(0, 3);
      lcd.print("                    ");
    }

    updateAllItems = false;
    CaptureButtonDownStates();
    pump_2.Tick();

    if(isClick && !pump_2.IsEnable())
    {
      isClick = false;
      pump_2.Start();
    }

    if(pump_2.IsCycleComplete())
    {
      BUZZER.Double();
      updateAllItems = true;
      step = 5;
    }
  }

  // ########### STEP 5 ############
  while (step == 5)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #5       ");
      lcd.setCursor(0, 2);
      lcd.print("Long Press OK for  ");
      lcd.setCursor(0, 3);
      lcd.print("Repeat or OK Done  ");
    }

    updateAllItems = false;
    CaptureButtonDownStates();

    if(isLongPress)
    {
      BUZZER.Long();
      updateAllItems = true;
      step = 1;
    }

    if(isClick)
    {
      isClick = false;
      pump_2.SetParameters(_config.pump2_duty, _config.pump2_volume, _config.pump2_calibrationOffset);
      encoder->setPosition(0);
      BUZZER.Double();
      currPage = MENU_PUMP_2;
      step = 1;
    }
  }
}

// =======================================================================//
//                               MENU PUMP 3                              //
// =======================================================================//
void Page_Pump_3()
{
  InitMenuPage(F("Pump #3"), 9);

  while (true)
  {
    if(updateAllItems)
    {
      if(MenuItemPrintable(1, 1)){lcd.print("On Time:           ");}
      if(MenuItemPrintable(1, 2)){lcd.print("Volume:            ");}
      if(MenuItemPrintable(1, 3)){lcd.print("Duty:              ");}
      if(MenuItemPrintable(1, 4)){lcd.print("Pump:              ");}
      if(MenuItemPrintable(1, 5)){lcd.print("Fertilize Start    ");}
      if(MenuItemPrintable(1, 6)){lcd.print("Start Calibration  ");}
      if(MenuItemPrintable(1, 7)){lcd.print("Reset Bottle Volume");}
      if(MenuItemPrintable(1, 8)){lcd.print("Save               ");}
      if(MenuItemPrintable(1, 9)){lcd.print("Back               ");}
    }

    if(updateAllItems || updateItemValue)
    {
      if(MenuItemPrintable(10, 1))  {PrintTimeString(_config.pump3_onTimeHour, _config.pump3_onTimeMinute);}
      if(MenuItemPrintable(9, 2))   {PrintFloatAtWidth(_config.pump3_volume, 1, ' ', false); lcd.print("ml ");}
      if(MenuItemPrintable(7, 3))   {PrintUint8_tAtWidth(_config.pump3_duty, 1, ' ', false); lcd.print("% ");}
      if(MenuItemPrintable(7, 4))   {PrintOnOff(_config.pump3_enable);}
    }

    if(updateValues)
    {
      pump_3.SetParameters(_config.pump3_duty, _config.pump3_volume, _config.pump3_calibrationOffset);
    }

    if(IsFlashChanged())
    {
      if(editMode)
      {
        PrintEditPoint();
      }
      else
      {
        PrintPointer();
      }
    }

    updateAllItems = false;
    updateItemValue = false;
    updateValues = false;
    CaptureButtonDownStates();

    while(pump_3.IsEnable())
    {
      pump_3.Tick();
    }

    if(isClick)
    {
      isClick = false;

      switch (pntrPos)
      {
        case 5:
          BUZZER.Single();
          pump_3.Start();
          VolumeBottle(&_config.pump3_volume_bottle, _config.pump3_volume);
          break;
        case 6:
          encoder->setPosition(0);
          currPage = MENU_PUMP_3_CALIBRATION;
          BUZZER.Double();
          return;
        case 8: 
          BUZZER.Long();
          SD_Save(); 
          BUZZER.Long();
          Page_Pump_3();
          break;
        case 9: 
          currPage = MENU_MAIN; 
          BUZZER.Double();
          return;
      }
    }

    if(isLongPress && !(pntrPos == 5 || pntrPos == 6 || pntrPos == 8 || pntrPos == 9))
    {
      isLongPress = false;

      if(pntrPos == 7)
      {
        _config.pump3_volume_bottle = 450;
        warningVolumeBottle = false;
        BUZZER.Long();
        return;
      }

      updateValues = true;
      editMode = !editMode;
      BUZZER.Double();
    }

    if(editMode)
    {
      encoder->tick();
      encoderPos = encoder->getPosition();
      if(encoderPos == 2)
      {
        encoderPos = encoderPos / 2;
      }

      switch (pntrPos)
      {
        case 1: AdjustTime(&_config.pump3_onTimeHour, &_config.pump3_onTimeMinute); break;
        case 2: AdjustFloat(&_config.pump3_volume, 0.2, 50); break;
        case 3: AdjustUint8_t(&_config.pump3_duty, 1, 100); break;
        case 4: AdjustBoolean(&_config.pump3_enable); break;
      }

      encoder->setPosition(0);
    }
    else
    {
      DoPointerNavigation();
    }

    PacintWait();
  }
}

// =======================================================================//
//                        MENU PUMP 3 CALIBRARTION                        //
// =======================================================================//
void Page_Pump_3_Calibration()
{
  InitMenuPage(F("Pump 3 Calibration"), 0);
  pump_3.SetParameters(_config.pump3_duty, 5, 0);

  // ########### STEP 1 ############
  while (step == 1)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #1       ");
      lcd.setCursor(0, 2);
      lcd.print("Long Press OK       ");
      lcd.setCursor(0, 3);
      lcd.print("for fill            ");
    }

    updateAllItems = false;
    CaptureButtonDownStates();

    if(isLongPress)
    {
      pump_3.Enable();
    }

    if(!isLongPress && pump_3.IsEnable())
    {
      pump_3.Disable();
    }

    if(isClick)
    {
      isClick = false;
      BUZZER.Single();
      updateAllItems = true;
      step = 2;
    }
  }

  // ########### STEP 2 ############
  while (step == 2)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #2       ");
      lcd.setCursor(0, 2);
      lcd.print("Press OK and measure");
      lcd.setCursor(0, 3);
      lcd.print("current volume level");
    }

    updateAllItems = false;
    CaptureButtonDownStates();
    pump_3.Tick();

    if(isClick && !pump_3.IsEnable())
    {
      isClick = false;
      pump_3.Start();
    }

    if(pump_3.IsCycleComplete())
    {
      BUZZER.Double();
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      updateAllItems = true;
      step = 3;
    }
  }

  // ########### STEP 3 ############
  while (step == 3)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #3       ");
      lcd.setCursor(0, 2);
      lcd.print("Set current volume  ");
    }

    if(updateAllItems || updateItemValue)
    {
      lcd.setCursor(0, 3);
      lcd.print(String(_config.pump3_calibrationOffset) + "ml ");
    }

    updateAllItems = false;
    updateItemValue = false;
    CaptureButtonDownStates();

    encoder->tick();
    encoderPos = encoder->getPosition();
    if(encoderPos == 2)
    {
      encoderPos = encoderPos / 2;
    }

    AdjustFloat(&_config.pump3_calibrationOffset, 0.0F, 50.0F);

    if(isClick)
    {
      isClick = false;
      pump_3.SetParameters(_config.pump3_duty, 5, _config.pump3_calibrationOffset);
      BUZZER.Single();
      updateAllItems = true;
      step = 4;
    }
  }

  // ########### STEP 4 ############
  while (step == 4)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #4       ");
      lcd.setCursor(0, 2);
      lcd.print("Run Test Fill       ");
      lcd.setCursor(0, 3);
      lcd.print("                    ");
    }

    updateAllItems = false;
    CaptureButtonDownStates();
    pump_3.Tick();

    if(isClick && !pump_3.IsEnable())
    {
      isClick = false;
      pump_3.Start();
    }

    if(pump_3.IsCycleComplete())
    {
      BUZZER.Double();
      updateAllItems = true;
      step = 5;
    }
  }

  // ########### STEP 5 ############
  while (step == 5)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #5       ");
      lcd.setCursor(0, 2);
      lcd.print("Long Press OK for  ");
      lcd.setCursor(0, 3);
      lcd.print("Repeat or OK Done  ");
    }

    updateAllItems = false;
    CaptureButtonDownStates();

    if(isLongPress)
    {
      BUZZER.Long();
      updateAllItems = true;
      step = 1;
    }

    if(isClick)
    {
      isClick = false;
      pump_3.SetParameters(_config.pump3_duty, _config.pump3_volume, _config.pump3_calibrationOffset);
      encoder->setPosition(0);
      BUZZER.Double();
      currPage = MENU_PUMP_3;
      step = 1;
    }
  }
}

// =======================================================================//
//                               MENU PUMP 4                              //
// =======================================================================//
void Page_Pump_4()
{
  InitMenuPage(F("Pump #4"), 9);

  while (true)
  {
    if(updateAllItems)
    {
      if(MenuItemPrintable(1, 1)){lcd.print("On Time:           ");}
      if(MenuItemPrintable(1, 2)){lcd.print("Volume:            ");}
      if(MenuItemPrintable(1, 3)){lcd.print("Duty:              ");}
      if(MenuItemPrintable(1, 4)){lcd.print("Pump:              ");}
      if(MenuItemPrintable(1, 5)){lcd.print("Fertilize Start    ");}
      if(MenuItemPrintable(1, 6)){lcd.print("Start Calibration  ");}
      if(MenuItemPrintable(1, 7)){lcd.print("Reset Bottle Volume");}
      if(MenuItemPrintable(1, 8)){lcd.print("Save               ");}
      if(MenuItemPrintable(1, 9)){lcd.print("Back               ");}
    }

    if(updateAllItems || updateItemValue)
    {
      if(MenuItemPrintable(10, 1))  {PrintTimeString(_config.pump4_onTimeHour, _config.pump4_onTimeMinute);}
      if(MenuItemPrintable(9, 2))   {PrintFloatAtWidth(_config.pump4_volume, 1, ' ', false); lcd.print("ml ");}
      if(MenuItemPrintable(7, 3))   {PrintUint8_tAtWidth(_config.pump4_duty, 1, ' ', false); lcd.print("% ");}
      if(MenuItemPrintable(7, 4))   {PrintOnOff(_config.pump4_enable);}
    }

    if(updateValues)
    {
      pump_4.SetParameters(_config.pump4_duty, _config.pump4_volume, _config.pump4_calibrationOffset);
    }

    if(IsFlashChanged())
    {
      if(editMode)
      {
        PrintEditPoint();
      }
      else
      {
        PrintPointer();
      }
    }

    updateAllItems = false;
    updateItemValue = false;
    updateValues = false;
    CaptureButtonDownStates();

    while(pump_4.IsEnable())
    {
      pump_4.Tick();
    }

    if(isClick)
    {
      isClick = false;

      switch (pntrPos)
      {
        case 5:
          BUZZER.Single();
          pump_4.Start();
          VolumeBottle(&_config.pump4_volume_bottle, _config.pump4_volume);
        break;
        case 6:
          encoder->setPosition(0);
          currPage = MENU_PUMP_4_CALIBRATION;
          BUZZER.Double();
          return;
        case 8: 
          BUZZER.Long();
          SD_Save(); 
          BUZZER.Long();
          Page_Pump_4();
          break;
        case 9: 
          currPage = MENU_MAIN; 
          BUZZER.Double();
          return;
      }
    }

    if(isLongPress && !(pntrPos == 5 || pntrPos == 6 || pntrPos == 8 || pntrPos == 9))
    {
      isLongPress = false;

      if(pntrPos == 7)
      {
        _config.pump4_volume_bottle = 450;
        warningVolumeBottle = false;
        BUZZER.Long();
        return;
      }

      updateValues = true;
      editMode = !editMode;
      BUZZER.Double();
    }

    if(editMode)
    {
      encoder->tick();
      encoderPos = encoder->getPosition();
      if(encoderPos == 2)
      {
        encoderPos = encoderPos / 2;
      }

      switch (pntrPos)
      {
        case 1: AdjustTime(&_config.pump4_onTimeHour, &_config.pump4_onTimeMinute); break;
        case 2: AdjustFloat(&_config.pump4_volume, 0.2, 50); break;
        case 3: AdjustUint8_t(&_config.pump4_duty, 1, 100); break;
        case 4: AdjustBoolean(&_config.pump4_enable); break;
      }

      encoder->setPosition(0);
    }
    else
    {
      DoPointerNavigation();
    }

    PacintWait();
  }
}

// =======================================================================//
//                        MENU PUMP 4 CALIBRARTION                        //
// =======================================================================//
void Page_Pump_4_Calibration()  
{
  InitMenuPage(F("Pump 4 Calibration"), 0);
  pump_4.SetParameters(_config.pump4_duty, 5, 0);

  // ########### STEP 1 ############
  while (step == 1)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #1       ");
      lcd.setCursor(0, 2);
      lcd.print("Long Press OK       ");
      lcd.setCursor(0, 3);
      lcd.print("for fill            ");
    }

    updateAllItems = false;
    CaptureButtonDownStates();

    if(isLongPress)
    {
      pump_4.Enable();
    }

    if(!isLongPress && pump_4.IsEnable())
    {
      pump_4.Disable();
    }

    if(isClick)
    {
      isClick = false;
      BUZZER.Single();
      updateAllItems = true;
      step = 2;
    }
  }

  // ########### STEP 2 ############
  while (step == 2)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #2       ");
      lcd.setCursor(0, 2);
      lcd.print("Press OK and measure");
      lcd.setCursor(0, 3);
      lcd.print("current volume level");
    }

    updateAllItems = false;
    CaptureButtonDownStates();
    pump_4.Tick();

    if(isClick && !pump_4.IsEnable())
    {
      isClick = false;
      pump_4.Start();
    }

    if(pump_4.IsCycleComplete())
    {
      BUZZER.Double();
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      updateAllItems = true;
      step = 3;
    }
  }

  // ########### STEP 3 ############
  while (step == 3)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #3       ");
      lcd.setCursor(0, 2);
      lcd.print("Set current volume  ");
    }

    if(updateAllItems || updateItemValue)
    {
      lcd.setCursor(0, 3);
      lcd.print(String(_config.pump4_calibrationOffset) + "ml ");
    }

    updateAllItems = false;
    updateItemValue = false;
    CaptureButtonDownStates();

    encoder->tick();
    encoderPos = encoder->getPosition();
    if(encoderPos == 2)
    {
      encoderPos = encoderPos / 2;
    }

    AdjustFloat(&_config.pump4_calibrationOffset, 0.0F, 50.0F);

    if(isClick)
    {
      isClick = false;
      pump_4.SetParameters(_config.pump4_duty, 5, _config.pump4_calibrationOffset);
      BUZZER.Single();
      updateAllItems = true;
      step = 4;
    }
  }

  // ########### STEP 4 ############
  while (step == 4)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #4       ");
      lcd.setCursor(0, 2);
      lcd.print("Run Test Fill       ");
      lcd.setCursor(0, 3);
      lcd.print("                    ");
    }

    updateAllItems = false;
    CaptureButtonDownStates();
    pump_4.Tick();

    if(isClick && !pump_4.IsEnable())
    {
      isClick = false;
      pump_4.Start();
    }

    if(pump_4.IsCycleComplete())
    {
      BUZZER.Double();
      updateAllItems = true;
      step = 5;
    }
  }

  // ########### STEP 5 ############
  while (step == 5)
  {
    if(updateAllItems)
    {
      lcd.setCursor(0, 1);
      lcd.print("      Step #5       ");
      lcd.setCursor(0, 2);
      lcd.print("Long Press OK for  ");
      lcd.setCursor(0, 3);
      lcd.print("Repeat or OK Done  ");
    }

    updateAllItems = false;
    CaptureButtonDownStates();

    if(isLongPress)
    {
      BUZZER.Long();
      updateAllItems = true;
      step = 1;
    }

    if(isClick)
    {
      isClick = false;
      pump_4.SetParameters(_config.pump4_duty, _config.pump4_volume, _config.pump4_calibrationOffset);
      encoder->setPosition(0);
      BUZZER.Double();
      currPage = MENU_PUMP_4;
      step = 1;
    }
  }
}

// =======================================================================//
//                              MENU SETTINGS                             //
// =======================================================================//
void Page_MenuSettings()
{
  InitMenuPage(F("Settings"), 8);
  currDateTime = timeRTC.GetDateTime();
  _config.years = currDateTime.year();
  _config.months = currDateTime.month();
  _config.days = currDateTime.day();
  _config.hours = currDateTime.hour();
  _config.minutes = currDateTime.minute();

  while (true)
  {
    if(updateAllItems)
    {
      if(MenuItemPrintable(1, 1)){lcd.print("Minutes:           ");}
      if(MenuItemPrintable(1, 2)){lcd.print("Hours:             ");}
      if(MenuItemPrintable(1, 3)){lcd.print("Day:               ");}
      if(MenuItemPrintable(1, 4)){lcd.print("Month:             ");}
      if(MenuItemPrintable(1, 5)){lcd.print("Year:              ");}
      if(MenuItemPrintable(1, 6)){lcd.print("Save               ");}
      if(MenuItemPrintable(1, 7)){lcd.print("Set Defaults       ");}
      if(MenuItemPrintable(1, 8)){lcd.print("Back               ");}
    }

    if(updateAllItems || updateItemValue)
    {
      if(MenuItemPrintable(10, 1)){PrintUint8_tAtWidth(_config.minutes, 3, ' ', false);}
      if(MenuItemPrintable(8, 2)){PrintUint8_tAtWidth(_config.hours, 3, ' ', false);}
      if(MenuItemPrintable(6, 3)){PrintUint8_tAtWidth(_config.days, 3, ' ', false);}
      if(MenuItemPrintable(8, 4)){PrintUint8_tAtWidth(_config.months, 3, ' ', false);}
      if(MenuItemPrintable(7, 5)){PrintUint8_tAtWidth(_config.years, 3, ' ', false);}
    }

    if(IsFlashChanged() && !editMode)
    {
      PrintPointer();
    }

    updateAllItems = false;
    updateItemValue = false;
    CaptureButtonDownStates();

    if(isDoubleClick && pntrPos == 4)
    {
      //buzzer.BeepSetDefault();
      Set_Defaults();
      Page_MenuSettings();
    }

    if(isClick)
    {
      isClick = false;

      switch (pntrPos)
      {
        case 6: 
          BUZZER.Long();
          timeRTC.SetTime(DateTime(_config.years, _config.months, _config.days, _config.hours, _config.minutes));
          break;
        case 8: 
          currPage = MENU_MAIN; 
          BUZZER.Double();
          return;
      }
    }

    if(isLongPress && !(pntrPos == 6 || pntrPos == 7 || pntrPos == 8))
    {
      isLongPress = false;
      editMode = !editMode;
      BUZZER.Double();
    }

    if(editMode)
    {
      PrintEditPoint();
      encoder->tick();
      encoderPos = encoder->getPosition();
      if(encoderPos == 2)
      {
        encoderPos = encoderPos / 2;
      }

      switch (pntrPos)
      {
        case 1: AdjustUint8_t(&_config.minutes, 0, 59);       break;
        case 2: AdjustUint8_t(&_config.hours, 0, 23);         break;
        case 3: AdjustUint8_t(&_config.days, 1, 31);          break;
        case 4: AdjustUint8_t(&_config.months, 1, 12);        break;
        case 5: AdjustUint16_t(&_config.years, 2024, 9999);   break;
      }

      encoder->setPosition(0);
    }
    else
    {
      DoPointerNavigation();
    }

    PacintWait();
  }
}

// =======================================================================//
//                                  TOOLS                                 //
// =======================================================================//
void InitMenuPage(String title, uint8_t itemCount)
{
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);

  uint8_t fillCnt = (DISP_CHAR_WIDTH - title.length()) / 2;
  if(fillCnt > 0)
  {
    for(uint8_t i = 0; i < fillCnt; i++)
    {
      lcd.print("-");
    }

    lcd.print(title);
  }
  if((title.length() % 2) == 1)
  {
    fillCnt++;
  }
  if(fillCnt > 0)
  {
    for(uint8_t i; i < fillCnt; i++)
    {
      lcd.print("-");
    }
  }

  //btnOk.ClearWasDown();

  itemCnt = itemCount;
  pntrPos = 1;
  dispOffset = 0;
  flashCntr = 0;
  flashIsOn = false;
  updateAllItems = true;
  loopStartMs = millis();
}

void CaptureButtonDownStates()
{
  WakeUp();
  btnOk.tick();
}

void AdjustBoolean(bool *v)
{
  if(encoderPos > 0 || encoderPos < 0)
  {
    *v = !*v;
    BUZZER.Single();
    updateItemValue = true;
  }
}

void AdjustUint8_t(uint8_t *v, uint8_t min, uint8_t max)
{
  if(encoderPos < 0)
  {
    if(*v > min)
    {
      *v = *v - 1;
      BUZZER.Single();
      encoder->setPosition(0);
      updateItemValue = true;
    }
  }

  if(encoderPos > 0)
  {
    if(*v < max)
    {
      *v = *v + 1;
      BUZZER.Single();
      encoder->setPosition(0);
      updateItemValue = true;
    }
  }
}

void AdjustUint16_t(uint16_t *v, uint16_t min, uint16_t max)
{
  if(encoderPos < 0)
  {
    if(*v > min)
    {
      *v = *v - 1;
      BUZZER.Single();
      encoder->setPosition(0);
      updateItemValue = true;
    }
  }

  if(encoderPos > 0)
  {
    if(*v < max)
    {
      *v = *v + 1;
      BUZZER.Single();
      encoder->setPosition(0);
      updateItemValue = true;
    }
  }
}

void AdjustFloat(float *v, float min, float max)
{
  if(encoderPos < 0)
  {
    if(*v > min)
    {
      *v = *v - 0.1;
      BUZZER.Single();
      updateItemValue = true;
    }
  }

  if(encoderPos > 0)
  {
    if(*v < max)
    {
      *v = *v + 0.1;
      BUZZER.Single();
      updateItemValue = true;
    }
  }

  encoder->setPosition(0);
}

void AdjustTime(byte *hour, byte *minute)
{
  if(encoderPos > 0)
  {
    *minute = *minute + 1;
    if(*minute > 59)
    {
      *hour = *hour + 1;
      *minute = 0;

      if(*hour > 23)
      {
        *hour = 0;
      }
    }

    BUZZER.Single();
    encoder->setPosition(0);
    updateItemValue = true;
  }

  if(encoderPos < 0)
  {
    *minute = *minute - 1;
    if(*minute > 59)
    {
      *hour = *hour - 1;
      *minute = 59;

      if(*hour < 0)
      {
        *hour = 23;
      }
    }

    BUZZER.Single();
    encoder->setPosition(0);
    updateItemValue = true;
  }
}

void DoPointerNavigation()
{
  encoder->tick();
  int newPos = encoder->getPosition();
  if(newPos == 2) {newPos = newPos / 2;};

  if(newPos < 0 && pntrPos > 1)
  {
    flashIsOn = false;
    wakeUp = true;
    flashCntr = 0;
    PrintPointer();
    BUZZER.Single();

    if(pntrPos - dispOffset == 1)
    {
      updateAllItems = true;
      dispOffset--;
    }

    pntrPos--;
  }
  else if(newPos > 0 && pntrPos < itemCnt)
  {
    flashIsOn = false;
    wakeUp = true;
    flashCntr = 0;
    PrintPointer();
    BUZZER.Single();

    if(pntrPos - dispOffset == DISP_ITEM_ROWS)
    {
      updateAllItems = true;
      dispOffset++;
    }

    pntrPos++;
  }

  encoder->setPosition(0);
}

bool IsFlashChanged()
{
  if(flashCntr == 0)
  {
    flashIsOn = !flashIsOn;
    flashCntr = FLASH_RST_CNT;
    return true;
  }
  else
  {
    flashCntr--;
    return false;
  }
}

void PacintWait()
{
  while (millis() - loopStartMs < PACING_MS)
  {
    delay(1);
  } 

  loopStartMs = millis();
}

bool MenuItemPrintable(uint8_t xPos, uint8_t yPos)
{
  if(!(updateAllItems || (updateItemValue && pntrPos == yPos)))
  {
    return false;
  }

  uint8_t yMaxOffset = 0;
  if(yPos > DISP_ITEM_ROWS)
  {
    yMaxOffset = yPos - DISP_ITEM_ROWS;
  }

  if(dispOffset <= (yPos - 1) && dispOffset >= yMaxOffset)
  {
    lcd.setCursor(xPos, yPos - dispOffset);
    return true;
  }

  return false;
}

void PrintPointer()
{
  if(itemCnt < 2)
  {
    return;
  }

  lcd.setCursor(0, pntrPos - dispOffset);
  if(flashIsOn)
  {
    lcd.print(F("\01"));
  }
  else
  {
    lcd.print(" ");
  }
}

void PrintEditPoint()
{
  lcd.setCursor(0, pntrPos - dispOffset);
  if(flashIsOn)
  {
    lcd.print(F("\02"));
  }
  else
  {
    lcd.print(" ");
  }
}

void PrintOnOff(bool val)
{
  if(val)
  {
    lcd.print("ON ");
  }
  else
  {
    lcd.print("OFF");
  }
}

void PrintChars(uint8_t cnt, char c)
{
  if(cnt > 0)
  {
    char cc[] = " ";
    cc[0] = c;

    for(uint8_t i = 1; i <= cnt; i++)
    {
      lcd.print(cc);
    }
  }
}

uint8_t GetUint32_tCharCnt(uint32_t value)
{
  if(value == 0)
  {
    return 1;
  }

  uint32_t tensCalc = 10;
  uint8_t cnt = 1;

  while (tensCalc <= value && cnt > 20)
  {
    tensCalc *= 10;
    cnt += 1;
  }
  
  return cnt;
}

uint8_t GetFloatCharCnt(float value)
{
  if(value == 0)
  {
    return 1;
  }

  uint32_t tensCalc = 10;
  uint8_t cnt = 1;

  while (tensCalc <= value && cnt > 20)
  {
    tensCalc *= 10;
    cnt += 1;
  }
  
  return cnt;
}

String GetTimeString(byte hour, byte minute)
{
  String time = String(hour) + ":";

  if(minute < 10)
  {
    time += "0" + String(minute);
  }
  else
  {
    time += String(minute);
  }

  return time;
}

void PrintTimeString(byte hour, byte minute)
{
  String time = String(hour) + ":";

  if(minute < 10)
  {
    time += "0" + String(minute);
  }
  else
  {
    time += String(minute);
  }

  lcd.print(time + " ");
}

void PrintUint8_tAtWidth(uint32_t value, uint8_t width, char c, boolean isRight)
{
  uint8_t numChars = GetUint32_tCharCnt(value);
  if(isRight)
  {
    PrintChars(width - numChars, c);
  }

  lcd.print(value);
  if(!isRight)
  {
    PrintChars(width - numChars, c);
  }
}

void PrintFloatAtWidth(float value, uint8_t width, char c, boolean isRight)
{
  uint8_t numChars = GetFloatCharCnt(value);
  if(isRight)
  {
    PrintChars(width - numChars, c);
  }

  lcd.print(value);
  if(!isRight)
  {
    PrintChars(width - numChars, c);
  }
}

void CheckPositionEncoder()
{
  encoder->tick();
}

void VolumeBottle(float *volumeBottle, float volume)
{
  *volumeBottle = *volumeBottle - volume; 

  if(*volumeBottle < 50)
  {
    BUZZER.Long();
    BUZZER.Double();
    BUZZER.Long();
    warningVolumeBottle = true;
  }
}

void Set_Defaults()
{
  SD.remove(fileName);
  File file = SD.open(fileName, FILE_WRITE);
  lcd.clear();

  while(!file) 
  {
    lcd.setCursor(0, 0);
    lcd.print("Create File Failed! ");
  }

  JsonDocument doc;

  // WHITE LED
  doc["whiteLed_onTimeHour"] = 12;
  doc["whiteLed_onTimeMinute"] = 0;
  doc["whiteLed_offTimeHour"] = 20;
  doc["whiteLed_offTimeMinute"] = 0;
  doc["whiteLed_rampUp"] = 30;
  doc["whiteLed_rampDown"] = 30;
  doc["whiteLed_maxDuty"] = 100;

  // COLOR LED
  doc["colorLed_onTimeHour"] = 12;
  doc["colorLed_onTimeMinute"] = 0;
  doc["colorLed_offTimeHour"] = 20;
  doc["colorLed_offTimeMinute"] = 0;
  doc["colorLed_rampUp"] = 30;
  doc["colorLed_rampDown"] = 30;
  doc["colorLed_maxDuty"] = 100;

  // PUMP 1
  doc["pump1_onTimeHour"] = 12;
  doc["pump1_onTimeMinute"] = 0;
  doc["pump1_duty"] = 100;
  doc["pump1_volume"] = 5;
  doc["pump1_calibrationOffset"] = 0;
  doc["pump1_enable"] = false;

  // PUMP 2
  doc["pump2_onTimeHour"] = 12;
  doc["pump2_onTimeMinute"] = 0;
  doc["pump2_duty"] = 100;
  doc["pump2_volume"] = 5;
  doc["pump2_calibrationOffset"] = 0;
  doc["pump2_enable"] = false;

  // PUMP 3
  doc["pump3_onTimeHour"] = 12;
  doc["pump3_onTimeMinute"] = 0;
  doc["pump3_duty"] = 100;
  doc["pump3_volume"] = 5;
  doc["pump3_calibrationOffset"] = 0;
  doc["pump3_enable"] = false;

  // PUMP 4
  doc["pump4_onTimeHour"] = 12;
  doc["pump4_onTimeMinute"] = 0;
  doc["pump4_duty"] = 100;
  doc["pump4_volume"] = 5;
  doc["pump4_calibrationOffset"] = 0;
  doc["pump4_enable"] = false;

  if(serializeJson(doc, file) == 0) 
  {
    lcd.setCursor(0, 1);
    lcd.print("Write To File Failed");
  }
  else
  {
    lcd.setCursor(0, 2);
    lcd.print("Default Values Set  ");
    delay(1500);
  }

  file.close();
  SD_Load();
  //buzzer.BeepSetDefault();
}

void SD_Init()
{
  while (!SD.begin(SD_PIN)) 
  {
    lcd.setCursor(0, 0);
    lcd.print(F("Failed To Initialize"));
    lcd.setCursor(0, 1);
    lcd.print(F("      SD Card!      "));
    BUZZER.Long();
  }

  lcd.setCursor(0, 0);
  lcd.print(F("SD Card Initialized!"));
  lcd.setCursor(0, 1);
  lcd.print(F("Data Loading....    "));
  delay(500);
  SD_Load();
}

void SD_Load()
{
  File file = SD.open(fileName);
  if(!file)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("File Not Available! "));
    lcd.setCursor(0, 1);
    lcd.print(F("Creating New File..."));
    delay(1000);
    SD_Save();
  }
  else
  {
    lcd.setCursor(0, 2);
    lcd.print(F("Data Initialized!   "));
    delay(1000);
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  if(error)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Failed To Read File!"));
    return;
  }

  // WHITE LED
  _config.whiteLed_onTimeHour = doc["whiteLed_onTimeHour"];
  _config.whiteLed_onTimeMinute = doc["whiteLed_onTimeMinute"];
  _config.whiteLed_offTimeHour = doc["whiteLed_offTimeHour"];
  _config.whiteLed_offTimeMinute = doc["whiteLed_offTimeMinute"];
  _config.whiteLed_rampUp = doc["whiteLed_rampUp"];
  _config.whiteLed_rampDown = doc["whiteLed_rampDown"];
  _config.whiteLed_maxDuty = doc["whiteLed_maxDuty"];

  // COLOR LED
  _config.colorLed_onTimeHour = doc["colorLed_onTimeHour"];
  _config.colorLed_onTimeMinute = doc["colorLed_onTimeMinute"];
  _config.colorLed_offTimeHour = doc["colorLed_offTimeHour"];
  _config.colorLed_offTimeMinute = doc["colorLed_offTimeMinute"];
  _config.colorLed_rampUp = doc["colorLed_rampUp"];
  _config.colorLed_rampDown = doc["colorLed_rampDown"];
  _config.colorLed_maxDuty = doc["colorLed_maxDuty"];

  // PUMP 1
  _config.pump1_onTimeHour = doc["pump1_onTimeHour"];
  _config.pump1_onTimeMinute = doc["pump1_onTimeMinute"];
  _config.pump1_duty = doc["pump1_duty"];
  _config.pump1_volume_bottle = doc["pump1_volume_bottle"];
  _config.pump1_volume = doc["pump1_volume"];
  _config.pump1_calibrationOffset = doc["pump1_calibrationOffset"];
  _config.pump1_enable = doc["pump1_enable"];

  // PUMP 2
  _config.pump2_onTimeHour = doc["pump2_onTimeHour"];
  _config.pump2_onTimeMinute = doc["pump2_onTimeMinute"];
  _config.pump2_duty = doc["pump2_duty"];
  _config.pump2_volume_bottle = doc["pump2_volume_bottle"];
  _config.pump2_volume = doc["pump2_volume"];
  _config.pump2_calibrationOffset = doc["pump2_calibrationOffset"];
  _config.pump2_enable = doc["pump2_enable"];

  // PUMP 3
  _config.pump3_onTimeHour = doc["pump3_onTimeHour"];
  _config.pump3_onTimeMinute = doc["pump3_onTimeMinute"];
  _config.pump3_duty = doc["pump3_duty"];
  _config.pump3_volume_bottle = doc["pump3_volume_bottle"];
  _config.pump3_volume = doc["pump3_volume"];
  _config.pump3_calibrationOffset = doc["pump3_calibrationOffset"];
  _config.pump3_enable = doc["pump3_enable"];

  // PUMP 4
  _config.pump4_onTimeHour = doc["pump4_onTimeHour"];
  _config.pump4_onTimeMinute = doc["pump4_onTimeMinute"];
  _config.pump4_duty = doc["pump4_duty"];
  _config.pump4_volume_bottle = doc["pump4_volume_bottle"];
  _config.pump4_volume = doc["pump4_volume"];
  _config.pump4_calibrationOffset = doc["pump4_calibrationOffset"];
  _config.pump4_enable = doc["pump4_enable"];

  // RTC
  _config.years = currDateTime.year();
  _config.months = currDateTime.month();
  _config.days = currDateTime.day();
  _config.hours = currDateTime.hour();
  _config.minutes = currDateTime.minute();

  file.close();
}

void SD_Save()
{
  SD.remove(fileName);
  File file = SD.open(fileName, FILE_WRITE);
  lcd.clear();

  while(!file) 
  {
    lcd.setCursor(0, 0);
    lcd.print("Create File Failed! ");
    BUZZER.Long();
    delay(1000);
  }

  JsonDocument doc;

  // WHITE LED
  doc["whiteLed_onTimeHour"] = _config.whiteLed_onTimeHour;
  doc["whiteLed_onTimeMinute"] = _config.whiteLed_onTimeMinute;
  doc["whiteLed_offTimeHour"] = _config.whiteLed_offTimeHour;
  doc["whiteLed_offTimeMinute"] = _config.whiteLed_offTimeMinute;
  doc["whiteLed_rampUp"] = _config.whiteLed_rampUp;
  doc["whiteLed_rampDown"] = _config.whiteLed_rampDown;
  doc["whiteLed_maxDuty"] = _config.whiteLed_maxDuty;

  // COLOR LED
  doc["colorLed_onTimeHour"] = _config.colorLed_onTimeHour;
  doc["colorLed_onTimeMinute"] = _config.colorLed_onTimeMinute;
  doc["colorLed_offTimeHour"] = _config.colorLed_offTimeHour;
  doc["colorLed_offTimeMinute"] = _config.colorLed_offTimeMinute;
  doc["colorLed_rampUp"] = _config.colorLed_rampUp;
  doc["colorLed_rampDown"] = _config.colorLed_rampDown;
  doc["colorLed_maxDuty"] = _config.colorLed_maxDuty;

  // PUMP 1
  doc["pump1_onTimeHour"] = _config.pump1_onTimeHour;
  doc["pump1_onTimeMinute"] = _config.pump1_onTimeMinute;
  doc["pump1_duty"] = _config.pump1_duty;
  doc["pump1_volume_bottle"] = _config.pump1_volume_bottle;
  doc["pump1_volume"] = _config.pump1_volume;
  doc["pump1_calibrationOffset"] = _config.pump1_calibrationOffset;
  doc["pump1_enable"] = _config.pump1_enable;

  // PUMP 2
  doc["pump2_onTimeHour"] = _config.pump2_onTimeHour;
  doc["pump2_onTimeMinute"] = _config.pump2_onTimeMinute;
  doc["pump2_duty"] = _config.pump2_duty;
  doc["pump2_volume_bottle"] = _config.pump2_volume_bottle;
  doc["pump2_volume"] = _config.pump2_volume;
  doc["pump2_calibrationOffset"] = _config.pump2_calibrationOffset;
  doc["pump2_enable"] = _config.pump2_enable;

  // PUMP 3
  doc["pump3_onTimeHour"] = _config.pump3_onTimeHour;
  doc["pump3_onTimeMinute"] = _config.pump3_onTimeMinute;
  doc["pump3_duty"] = _config.pump3_duty;
  doc["pump3_volume_bottle"] = _config.pump3_volume_bottle;
  doc["pump3_volume"] = _config.pump3_volume;
  doc["pump3_calibrationOffset"] = _config.pump3_calibrationOffset;
  doc["pump3_enable"] = _config.pump3_enable;

  // PUMP 4
  doc["pump4_onTimeHour"] = _config.pump4_onTimeHour;
  doc["pump4_onTimeMinute"] = _config.pump4_onTimeMinute;
  doc["pump4_duty"] = _config.pump4_duty;
  doc["pump4_volume_bottle"] = _config.pump4_volume_bottle;
  doc["pump4_volume"] = _config.pump4_volume;
  doc["pump4_calibrationOffset"] = _config.pump4_calibrationOffset;
  doc["pump4_enable"] = _config.pump4_enable;

  // RTC
  //doc["hour"] = _config.hours;
  //doc["minute"] = _config.minutes;

  while(serializeJson(doc, file) == 0)
  {
    lcd.setCursor(0, 1);
    lcd.print("Write To File Failed");
    BUZZER.Long();
    delay(1000);
  }

  if(serializeJson(doc, file) != 0)
  {
    lcd.setCursor(0, 1);
    lcd.print("  Write To File Ok  ");
    delay(1000);
  }

  file.close();
}

void LCD_Init()
{
  lcd.begin(20, 4);
  lcd.backlight();
  lcd.createChar(1, arrowSymbol);
  lcd.createChar(2, editSymbol);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("####################");
  lcd.setCursor(2, 1);
  lcd.print("Aqua Controller");
  lcd.setCursor(4, 2);
  lcd.print("V1.4 by Paul");
  lcd.setCursor(0, 3);
  lcd.print("####################");
  delay(1500);
  lcd.clear();
}