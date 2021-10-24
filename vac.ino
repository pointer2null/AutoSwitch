#include <Arduino.h>
#include <SerialCommands.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>


#define blinkPin 0                     // pin 5, PB0
#define relay    1                     // pin 6, PB1
#define serialTX 3                     // pin 2, PB3
#define serialRX 4                     // pin 3, PB4
#define sense    A1                    // pin 7, PB2, ADC1

#define defaultThresholdRate     2     // how fast we climb when over the trigger level
#define defaultThreshold         600   // ADC trigger level
#define defaultThresholdCountMax 10    // max counter value - affects how long we stay on after the tool is off
#define defaultTriggerLevel      6     // count point to switch on

#define calPeriodMs              10000 // 10 seconds
 
int analogValue       = 0;             // hall sensor value
int thresholdCount    = 0;             // counter       

boolean blink = false;
boolean watch = false;

char serial_command_buffer_[32];
struct Data {
  char magic[1];                       // need some way to see if the eeprom has been initialized
  int thresholdRate;                   // how fast we climb when over the current trigger level
  int threshold;                       // ADC current trigger level
  int thresholdCountMax;               // max counter value - affects how long we stay on after the tool is off
  int triggerLevel;                    // counter trigger level that turns on vac
};

Data d;

SoftwareSerial serial(serialRX, serialTX);

SerialCommands serial_commands_(&serial, serial_command_buffer_, sizeof(serial_command_buffer_), "\r", " ");


//This is the default handler, and gets called when no other command matches. 
// note the F() macro stores the const strings in flash to same ram
void cmd_unrecognized(SerialCommands* sender, const char* cmd){
  watch = false;
  sender->GetSerial()->print(F("Unrecognized command ["));
  sender->GetSerial()->print(cmd);
  sender->GetSerial()->println("]");
  useage();
}

void useage() {
  serial.println(F("\r\n\n******GVac switch.******\r\n"));
  serial.println(F("set <n> : set the current threshold"));
  serial.println(F("get     : get the current threshold"));
  serial.println(F("watch   : watch the current sensor value"));
  serial.println(F("cal     : auto set the current threshold"));
}

//called for set command
void cmd_set(SerialCommands* sender){
  watch = false;
  char* valStr = sender->Next();
  if (valStr == NULL){
    useage();
    return;
  }

  int val = atoi(valStr);
  d.threshold = val;
  EEPROM.put(0, d);
}

//called for get command
void cmd_get(SerialCommands* sender){
  watch = false;
  sender->GetSerial()->print(F("Current sensor threshold:"));
  sender->GetSerial()->println(d.threshold);
  useage();
}

//called for watch command
void cmd_watch(SerialCommands* sender){
  sender->GetSerial()->println(F("Current sensor values. "));
  sender->GetSerial()->println(F("** Press x to stop. **"));
  watch = true;
}

void cmd_stop_watch(SerialCommands* sender){
  watch = false;
  useage();
}

void cmd_cal(SerialCommands* sender){
  watch = false;
  sender->GetSerial()->println(F("calibrating: do not switch on load..."));
  int calVal = 0;
  unsigned long stopwatch = millis(); // Store a snapshot of the current time since the program started executing
  // Collect the max ADC value from the current sensor for the predetermined sample period
  while(millis() - stopwatch < calPeriodMs) { 
    calVal = max(calVal, analogRead(sense));
    sender->GetSerial()->print(".");
    delay(250);
  }
  calVal = calVal * 1.1; // 110% truncated down
  sender->GetSerial()->print(F("\r\nComplete. New value is ["));
  sender->GetSerial()->print(calVal);
  sender->GetSerial()->println(F("]"));
  d.threshold = calVal;
  EEPROM.put(0, d);
  useage();
}

//Note: Commands are case sensitive
SerialCommand cmd_set_("set", cmd_set);
SerialCommand cmd_get_("get", cmd_get);
SerialCommand cmd_watch_("watch", cmd_watch);
SerialCommand cmd_stop_watch_("x", cmd_stop_watch, true);
SerialCommand cmd_cal_("cal", cmd_cal);

 

void setup(){
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
  
  // check eeprom - see if we've been initialized before
  EEPROM.get(0, d);
  if (d.magic[0] != 'G'){
    serial.print(F("First run, using defaults.\r\n"));
    // this is the first ever run
    // set defaults
    d.magic[0]          = 'G';
    d.thresholdCountMax = defaultThresholdCountMax;
    d.triggerLevel      = defaultTriggerLevel;
    d.thresholdRate     = defaultThresholdRate;
    d.threshold         = defaultThreshold;
    EEPROM.put(0, d);
  } else {
    serial.print(F("Program data loaded.\r\n"));
  }
}

void loop(){
  analogValue = analogRead(sense);
  if ((analogValue >= d.threshold) &&
      (thresholdCount <= d.thresholdCountMax)) {
    // count up fast
    thresholdCount += d.thresholdRate;
    blink = true;
  } else {
    blink = false;
  }
  if ((analogValue < d.threshold) && (thresholdCount > 0 )) {
    // count down slow
    thresholdCount--;
  }
  if (watch) {
    valuesDump();
  }
  if (thresholdCount > d.triggerLevel) {
    digitalWrite(blinkPin, HIGH);
    digitalWrite(relay, HIGH);
    blink = false;
  } else {
    digitalWrite(relay, LOW);
    digitalWrite(blinkPin, LOW);
    // if we want some positive feedback for a clean switch off, uncomment the line below
    // thresholdCount = 0;
  }
  if (blink) {
    digitalWrite(blinkPin, HIGH);
    delay(500);
    digitalWrite(blinkPin, LOW);
    delay(500);
  } else {
    delay(1000);
  }
  //serial.write(serial.read());
  serial_commands_.ReadSerial();
}

void valuesDump() {
  serial.print("\r\nValue = ");
  serial.print(analogValue, DEC);
}
