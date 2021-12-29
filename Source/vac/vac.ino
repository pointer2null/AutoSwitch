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

// 0.00488V (4.88mV) per unit on the ADC
// 2.5V (zero current = 512) (defaul using Vcc as ref)
// ACS712-20A has a sensitivity of 100mV/A
// So 1A = 100mV
// Means we should see a adc increase of around 20 per amp.
// 1A @ 240V is 240W  
#define defaultCurrentThreshold  520   // ADC trigger level : approx 17 counts => 200W
#define defaultThresholdCountMax 10    // max counter value - affects how long we stay on after the tool is off
#define defaultThresholdOn       4     // count point to switch on
#define defaultThresholdOff      6     // count point to switch off
#define defaultCheckPeriodMs     500   // loop poll frequency

#define calPeriodMs              10000 // 10 seconds

int analogValue       = 0;             // hall sensor value
int thresholdCount    = 0;             // counter

boolean blink = false;
boolean watch = false;
boolean force = false;

long now           = 0;
long lastCheck     = 0;

bool blinkLed      = true;

char serial_command_buffer_[32];
struct Data {
  char magic[1];                       // need some way to see if the eeprom has been initialized
  int adcThreshold;                    // ADC current trigger level
  int thresholdCountMax;               // max counter value - affects how long we stay on after the tool is off
  int thresholdOn;                     // counter trigger level that turns on vac
  int thresholdOff;
  long checkPeriodMs;
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


void useage() {
  serial.println(F("\r\n\n******GVac switch.******\r\n"));
  serial.println(F("get          : get all config values"));
  serial.println(F("current [n]  : get or set the tool current threshold value"));
  serial.println(F("on [n]       : get or set the on trigger value"));
  serial.println(F("off [n]      : get or set the off trigger value"));
  serial.println(F("count [n]    : get or set the max count value"));
  serial.println(F("period [n]   : get or set the poll period (ms)"));

  
  serial.println(F("def          : set the default values"));
  serial.println(F("cal          : auto set the current threshold"));
  serial.println(F("watch        : watch the current system values (x to stop)"));
  serial.println(F("f<on|off>    : force switch on/off"));
}

//*** seial commands ***
//Note: Commands are case sensitive

//This is the default handler, and gets called when no other command matches.
// note the F() macro stores the const strings in flash to same ram
void cmd_unrecognized(SerialCommands* sender, const char* cmd) {
  watch = false;
  sender->GetSerial()->print(F("Unrecognized command ["));
  sender->GetSerial()->print(cmd);
  sender->GetSerial()->println("]");
  useage();
}

//get command : display current settings
void cmd_get(SerialCommands* sender) {
  watch = false;
  valuesHeader();
  valuesDump();
  useage();
}
SerialCommand cmd_get_("get", cmd_get);

//set command : get/set the adc current threshold
void cmd_current(SerialCommands* sender) {
  watch = false;
  char* valStr = sender->Next();
  if (valStr == NULL) {
    sender->GetSerial()->print(F("Current sensor threshold: "));
    sender->GetSerial()->println(d.adcThreshold, DEC);
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
SerialCommand cmd_current_("current", cmd_current);


void cmd_period(SerialCommands* sender) {
  watch = false;
  char* valStr = sender->Next();
  if (valStr == NULL) {
    sender->GetSerial()->print(F("Loop period (ms): "));
    sender->GetSerial()->println(d.checkPeriodMs, DEC);
    useage();
    return;
  }

  int val = atoi(valStr);
  if (val < 1) {
    sender->GetSerial()->println(F("Nonsensical value - ignoring"));
  } else {
    d.checkPeriodMs = val;
    EEPROM.put(0, d);
  }
  valuesHeader();
  valuesDump();
  useage();
}
SerialCommand cmd_period_("current", cmd_period);


void cmd_count(SerialCommands* sender) {
  watch = false;
  char* valStr = sender->Next();
  if (valStr == NULL) {
    sender->GetSerial()->print(F("Count max: "));
    sender->GetSerial()->println(d.thresholdCountMax, DEC);
    useage();
    return;
  }

  int val = atoi(valStr);
  if (val <= d.thresholdOff) {
    sender->GetSerial()->println(F("Count max must be greater than the off threshold"));
  } else {
    d.thresholdCountMax = val;
    EEPROM.put(0, d);
  }
  valuesHeader();
  valuesDump();
  useage();
}
SerialCommand cmd_count_("count", cmd_count);

//called for watch command
void cmd_watch(SerialCommands* sender) {
  sender->GetSerial()->println(F("Current sensor values. "));
  sender->GetSerial()->println(F("** Press x to stop. **"));
  valuesHeader();
  watch = true;
}
SerialCommand cmd_watch_("watch", cmd_watch);

void cmd_stop_watch(SerialCommands* sender) {
  watch = false;
  useage();
}
SerialCommand cmd_stop_watch_("x", cmd_stop_watch, true);

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
  calVal += 20; // approx 200W
  sender->GetSerial()->print(F("\r\nComplete. New value is ["));
  sender->GetSerial()->print(calVal);
  sender->GetSerial()->println(F("]"));
  d.adcThreshold = calVal;
  EEPROM.put(0, d);
  valuesHeader();
  valuesDump();
  useage();
}
SerialCommand cmd_cal_("cal", cmd_cal);

void cmd_setOn(SerialCommands* sender) {
  watch = false;
  char* valStr = sender->Next();
  if (valStr == NULL) {
    sender->GetSerial()->print(F("Count on threshold: "));
    sender->GetSerial()->println(d.thresholdOn, DEC);
    useage();
    return;
  }

  int val = atoi(valStr);
  if (val <= 1 || val >= d.thresholdOff) {
    useage();
    return;
  }
  d.thresholdOn = val;
  EEPROM.put(0, d);
  valuesHeader();
  valuesDump();
  useage();
}
SerialCommand cmd_setOn_("on", cmd_setOn);

void cmd_setOff(SerialCommands* sender) {
  watch = false;
  char* valStr = sender->Next();
  if (valStr == NULL) {
    sender->GetSerial()->print(F("Count off threshold: "));
    sender->GetSerial()->println(d.thresholdOff, DEC);
    useage();
    return;
  }

  int val = atoi(valStr);
  if (val <= d.thresholdOn || val >= defaultThresholdCountMax) {
    useage();
    return;
  }
  d.thresholdOff = val;
  EEPROM.put(0, d);
  valuesHeader();
  valuesDump();
  useage();
}
SerialCommand cmd_setOff_("off", cmd_setOff);

void cmd_setDef(SerialCommands* sender) {
  setDefaults();
  valuesHeader();
  valuesDump();
  useage();
}
SerialCommand cmd_setDef_("def", cmd_setDef);

void cmd_setFOff(SerialCommands* sender) {
  force = false;
  valuesHeader();
  valuesDump();
  useage();
}
SerialCommand cmd_setFOff_("foff", cmd_setFOff);

void cmd_setFOn(SerialCommands* sender) {
  force = true;
  valuesHeader();
  valuesDump();
  useage();
}
SerialCommand cmd_setFOn_("fon", cmd_setFOn);


void setup() {
  serial.begin(9600);
  serial.print(F("\r\n******GVac switch.******\r\n"));
  useage();
  pinMode(sense, INPUT);
  pinMode(blinkPin, OUTPUT);
  pinMode(relay, OUTPUT);
  serial_commands_.SetDefaultHandler(cmd_unrecognized);
  serial_commands_.AddCommand(&cmd_current_);
  serial_commands_.AddCommand(&cmd_get_);
  serial_commands_.AddCommand(&cmd_watch_);
  serial_commands_.AddCommand(&cmd_stop_watch_);
  serial_commands_.AddCommand(&cmd_cal_);
  serial_commands_.AddCommand(&cmd_setOn_);
  serial_commands_.AddCommand(&cmd_setOff_);
  serial_commands_.AddCommand(&cmd_setDef_);
  serial_commands_.AddCommand(&cmd_setFOn_);
  serial_commands_.AddCommand(&cmd_setFOff_);
  serial_commands_.AddCommand(&cmd_period_);
  serial_commands_.AddCommand(&cmd_count_);

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
  d.adcThreshold      = defaultCurrentThreshold;
  d.checkPeriodMs     = defaultCheckPeriodMs;
  EEPROM.put(0, d);
}

void loop() {
  now = millis();
  // check every 500ms
  if (force) {
    // set everything on
    digitalWrite(relay, HIGH);
    digitalWrite(blinkPin, HIGH);
  } else if (lastCheck + d.checkPeriodMs < now) {
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
