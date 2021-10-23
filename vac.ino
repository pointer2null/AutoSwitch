#include <Arduino.h>
#include <SerialCommands.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>


#define blinkPin 0                   // pin 5, PB0
#define relay    1                   // pin 6, PB1
#define serialTX 3                   // pin 2, PB3
#define serialRX 4                   // pin 3, PB4
#define sense    A1                  // pin 7, PB2, ADC1

#define defaultThresholdRate     2   // how fast we climb when over the trigger level
#define defaultThreshold         600 // ADC trigger level
#define defaultThresholdCountMax 10  // max counter value - affects how long we stay on after the tool is off
#define defaultTriggerLevel      6   // count point to switch on

int analogValue       = 0;           // hall sensor value
int thresholdCount    = 0;           // counter       

boolean blink = false;
boolean watch = false;

char serial_command_buffer_[32];
struct Data {
  char magic[1];                    // need some way to see if the eeprom has been initialized
  int thresholdRate;                // how fast we climb when over the current trigger level
  int threshold;                    // ADC current trigger level
  int thresholdCountMax;            // max counter value - affects how long we stay on after the tool is off
  int triggerLevel;                 // counter trigger level that turns on vac
};

Data d;


SoftwareSerial serial(serialRX, serialTX);

SerialCommands serial_commands_(&serial, serial_command_buffer_, sizeof(serial_command_buffer_), "\r\n", " ");


//This is the default handler, and gets called when no other command matches. 
void cmd_unrecognized(SerialCommands* sender, const char* cmd){
  watch = false;
  sender->GetSerial()->print("Unrecognized command [");
  sender->GetSerial()->print(cmd);
  sender->GetSerial()->println("]");
}

//called for set command
void cmd_set(SerialCommands* sender){
  watch = false;
}

//called for get command
void cmd_get(SerialCommands* sender){
  watch = false;
}

//called for watch command
void cmd_watch(SerialCommands* sender){
  watch = true;
}

//Note: Commands are case sensitive
SerialCommand cmd_set_("set", cmd_set);
SerialCommand cmd_get_("get", cmd_get);
SerialCommand cmd_watch_("watch", cmd_watch);

char magicChar[1] = {'G'};               // Magic char  

void setup(){
  serial.begin(9600);
  serial.print("\r\n GVac switch. \r\n");
  
  pinMode(sense, INPUT);
  pinMode(blinkPin, OUTPUT);
  pinMode(relay, OUTPUT);
  serial_commands_.SetDefaultHandler(cmd_unrecognized);
  serial_commands_.AddCommand(&cmd_set_);
  serial_commands_.AddCommand(&cmd_get_);
  serial_commands_.AddCommand(&cmd_watch_);
  // check eeprom - see if we've been initialized before
  
  EEPROM.get(0, d);
  if (d.magic[0] != 'G'){
    serial.print("Using defaults.\r\n");
    // this is the first ever run
    // set defaults
    d.magic[0]          = magicChar;
    d.thresholdCountMax = defaultThresholdCountMax;
    d.triggerLevel      = defaultTriggerLevel;
    d.thresholdRate     = defaultThresholdRate;
    d.threshold         = defaultThreshold;
    EEPROM.put(0, d);
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
  serial_commands_.ReadSerial();
}

void valuesDump() {
  serial.print("\r\nValue = ");
  serial.print(analogValue, DEC);
}
