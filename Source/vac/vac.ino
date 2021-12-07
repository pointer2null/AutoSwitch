#include <Arduino.h>
#include "SerialCommands.h"
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <Time.h>


#define blinkPin 0                     // pin 5, PB0
#define relay    1                     // pin 6, PB1
#define serialTX 3                     // pin 2, PB3
#define serialRX 4                     // pin 3, PB4
#define sense    A1                    // pin 7, PB2, ADC1

#define defaultThresholdRate     2     // how fast we climb when over the trigger level
#define defaultCurrentThreshold  600   // ADC trigger level
#define defaultThresholdCountMax 20    // max counter value - affects how long we stay on after the tool is off
#define defaultThresholdOn       6     // count point to switch on
#define defaultThresholdOff      10    // count point to switch on

#define calPeriodMs              10000 // 10 seconds
#define countPeriodMs            750   // 3/4 second

int analogValue       = 0;             // hall sensor value
int thresholdCount    = 0;             // counter

boolean blink = false;
boolean watch = false;

long now           = 0;
long lastCheck     = 0;
long checkPeriodMs = 500;
bool blinkLed      = true;

char serial_command_buffer_[32];
struct Data {
  char magic[1];                       // need some way to see if the eeprom has been initialized
  int thresholdRate;                   // how fast we climb when over the current trigger level
  int adcThreshold;                    // ADC current trigger level
  int thresholdCountMax;               // max counter value - affects how long we stay on after the tool is off
  int thresholdOn;                     // counter trigger level that turns on vac
  int thresholdOff;
};

Data d;

SoftwareSerial serial(serialRX, serialTX);
SerialCommands serial_commands_(&serial, serial_command_buffer_, sizeof(serial_command_buffer_), "\r", " ");

void valuesHeader() {
  serial.print(F("\r\n\nAnalog\tAnalog\t\tThreshold\tOn trigger\tOff trigger\tCounter"));
  serial.print(F("\r\nvalue\tthreshold\tcounter\t\tlevel\t\tlevel\t\tmax\r\n"));
}

void valuesDump() {
  serial.print(analogValue, DEC);
  serial.print(F("\t"));
  serial.print(d.adcThreshold, DEC);
  serial.print(F("\t\t"));
  serial.print(thresholdCount, DEC);
  serial.print(F("\t\t"));
  serial.print(d.thresholdOn, DEC);
  serial.print(F("\t\t"));
  serial.print(d.thresholdOff, DEC);
  serial.print(F("\t\t"));
  serial.print(d.thresholdCountMax, DEC);
  serial.print(F("\r\n"));
}

//This is the default handler, and gets called when no other command matches.
// note the F() macro stores the const strings in flash to same ram
void cmd_unrecognized(SerialCommands* sender, const char* cmd) {
  watch = false;
  sender->GetSerial()->print(F("Unrecognized command ["));
  sender->GetSerial()->print(cmd);
  sender->GetSerial()->println("]");
  useage();
}

void useage() {
  serial.println(F("\r\n\n******GVac switch.******\r\n"));
  serial.println(F("get      : get config values"));
  serial.println(F("set <n>  : set the tool current threshold value"));
  serial.println(F("def      : set the default values"));
  serial.println(F("cal      : auto set the current threshold"));
  serial.print  (F("on <n>   : set the on trigger value (0 - "));
  serial.print(defaultThresholdCountMax, DEC);
  serial.println(F(", must be less than off value)"));
  serial.print  (F("off <n>  : set the off trigger value (0 - "));
  serial.print(defaultThresholdCountMax, DEC);
  serial.println(F(", must be more than on value)"));
  serial.println(F("watch    : watch the hall current sensor output value"));
}

//called for set command
void cmd_set(SerialCommands* sender) {
  watch = false;
  char* valStr = sender->Next();
  if (valStr == NULL) {
    useage();
    return;
  }

  int val = atoi(valStr);
  d.adcThreshold = val;
  EEPROM.put(0, d);
  valuesHeader();
  valuesDump();
  useage();
}

//called for get command
void cmd_get(SerialCommands* sender) {
  watch = false;
  valuesHeader();
  valuesDump();
  useage();
}

//called for watch command
void cmd_watch(SerialCommands* sender) {
  sender->GetSerial()->println(F("Current sensor values. "));
  sender->GetSerial()->println(F("** Press x to stop. **"));
  valuesHeader();
  watch = true;
}

void cmd_stop_watch(SerialCommands* sender) {
  watch = false;
  useage();
}

void cmd_cal(SerialCommands* sender) {
  watch = false;
  sender->GetSerial()->println(F("calibrating: do not switch on load..."));
  int calVal = 0;
  unsigned long stopwatch = millis(); // Store a snapshot of the current time since the program started executing
  // Collect the max ADC value from the current sensor for the predetermined sample period
  while (millis() - stopwatch < calPeriodMs) {
    calVal = max(calVal, analogRead(sense));
    sender->GetSerial()->print(".");
    delay(250);
  }
  calVal = calVal * 1.1; // 110% truncated down
  sender->GetSerial()->print(F("\r\nComplete. New value is ["));
  sender->GetSerial()->print(calVal);
  sender->GetSerial()->println(F("]"));
  d.adcThreshold = calVal;
  EEPROM.put(0, d);
  valuesHeader();
  valuesDump();
  useage();
}

void cmd_setOn(SerialCommands* sender) {
  watch = false;
  char* valStr = sender->Next();
  if (valStr == NULL) {
    useage();
    return;
  }

  int val = atoi(valStr);
  if (val < 1 || val > d.thresholdOff) {
    useage();
    return;
  }
  d.thresholdOn = val;
  EEPROM.put(0, d);
  valuesHeader();
  valuesDump();
  useage();
}

void cmd_setOff(SerialCommands* sender) {
  watch = false;
  char* valStr = sender->Next();
  if (valStr == NULL) {
    useage();
    return;
  }

  int val = atoi(valStr);
  if (val < d.thresholdOn || val > defaultThresholdCountMax) {
    useage();
    return;
  }
  d.thresholdOff = val;
  EEPROM.put(0, d);
  valuesHeader();
  valuesDump();
  useage();
}

void cmd_setDef(SerialCommands* sender) {
  setDefaults();
  valuesHeader();
  valuesDump();
  useage();
}

//Note: Commands are case sensitive
SerialCommand cmd_set_("set", cmd_set);
SerialCommand cmd_get_("get", cmd_get);
SerialCommand cmd_watch_("watch", cmd_watch);
SerialCommand cmd_stop_watch_("x", cmd_stop_watch, true);
SerialCommand cmd_cal_("cal", cmd_cal);
SerialCommand cmd_setOn_("on", cmd_setOn);
SerialCommand cmd_setOff_("off", cmd_setOff);
SerialCommand cmd_setDef_("def", cmd_setDef);



void setup() {
  serial.begin(9600);
  serial.print(F("\r\n******GVac switch.******\r\n"));
  useage();
  pinMode(sense, INPUT);
  pinMode(blinkPin, OUTPUT);
  pinMode(relay, OUTPUT);
  serial_commands_.SetDefaultHandler(cmd_unrecognized);
  serial_commands_.AddCommand(&cmd_set_);
  serial_commands_.AddCommand(&cmd_get_);
  serial_commands_.AddCommand(&cmd_watch_);
  serial_commands_.AddCommand(&cmd_stop_watch_);
  serial_commands_.AddCommand(&cmd_cal_);
  serial_commands_.AddCommand(&cmd_setOn_);
  serial_commands_.AddCommand(&cmd_setOff_);
  serial_commands_.AddCommand(&cmd_setDef_);

  // check eeprom - see if we've been initialized before
  EEPROM.get(0, d);
  if (d.magic[0] != 'G') {
    serial.print(F("First run, using defaults.\r\n"));
    // this is the first ever run
    // set defaults
    setDefaults();
  } else {
    serial.print(F("Program data loaded.\r\n"));
  }
  valuesHeader();
  valuesDump();
}

void setDefaults() {
  d.magic[0]          = 'G';
  d.thresholdCountMax = defaultThresholdCountMax;
  d.thresholdOn       = defaultThresholdOn;
  d.thresholdOff      = defaultThresholdOff;
  d.thresholdRate     = defaultThresholdRate;
  d.adcThreshold      = defaultCurrentThreshold;
  EEPROM.put(0, d);
}

void loop() {
  now = millis();
  // check every 500ms
  if (lastCheck + checkPeriodMs < now) {
    periodicCheck();
    lastCheck = now;
  }
  serial_commands_.ReadSerial();

}

void periodicCheck() {
  analogValue = analogRead(sense);
  if (watch) {
    valuesDump();
  }

  if ((analogValue >= d.adcThreshold) &&
      (thresholdCount <= d.thresholdCountMax)) {
    // switching on - over trigger level, increase counter
    //thresholdCount += d.thresholdRate;
    thresholdCount++;
    blink = true; // start blink to show tool is on
  }
  if ((analogValue < d.adcThreshold) && (thresholdCount > 0 )) {
    // current sense is below trigger
    blink = false;
    // count down slow
    thresholdCount--;
  }

  if ((thresholdCount > d.thresholdOn) && (analogValue > d.adcThreshold)) {
    // switch on
    digitalWrite(relay, HIGH);
    digitalWrite(blinkPin, HIGH);
    blinkLed = true;
    blink = false; // cancel blink as we've switched on
  }

  if ((thresholdCount < d.thresholdOff) && (analogValue < d.adcThreshold)) {
    // switch off
    digitalWrite(relay, LOW);
    blink = false;
    blinkLed = false;
    // positive feedback for a clean switch off
    thresholdCount = 0;
  }
  if (blink) {
    blinkLed = !blinkLed;
  }

  digitalWrite(blinkPin, blinkLed);
}
