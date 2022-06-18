#include <Arduino.h>
#include "SerialCommands.h"

void valuesHeader();
void valuesDump();
void useage();
void cmd_get(SerialCommands* sender);
void cmd_current(SerialCommands* sender);
void cmd_period(SerialCommands* sender);
void cmd_count(SerialCommands* sender);
void cmd_watch(SerialCommands* sender);
void cmd_stop_watch(SerialCommands* sender);
void cmd_cal(SerialCommands* sender);
void cmd_setOn(SerialCommands* sender);
void cmd_setOff(SerialCommands* sender);
void cmd_setDef(SerialCommands* sender);
void cmd_setFOff(SerialCommands* sender);
void cmd_setFOn(SerialCommands* sender);
void setDefaults();
void periodicCheck();

#define blinkPin 0                     // pin 5, PB0
#define relay    1                     // pin 6, PB1
#define serialTX 3                     // pin 2, PB3
#define serialRX 4                     // pin 3, PB4
#define sense    A1                    // pin 7, PB2, ADC1

// 0.00488V (4.88mV) per unit on the ADC
// 2.5V (zero current = 512) (defaul using Vcc as ref)
// ACS712-20A has a sensitivity of 100mV/A
// So 1A = 100mV
// Means we should see a adc increase of around 20 per amp.
// 1A @ 240V is 240W  
#define defaultCurrentThreshold  520   // ADC trigger level : approx 17 counts => 200W
#define defaultThresholdCountMax 10    // max counter value - affects how long we stay on after the tool is off
#define defaultThresholdOn       3     // count point to switch on
#define defaultThresholdOff      6     // count point to switch off
#define defaultCheckPeriodMs     500   // loop poll frequency

#define calPeriodMs              10000 // 10 seconds