#include <SendOnlySoftwareSerial.h>

int blinkPin = 0;  // pin 5, PB0
int relay    = 1;  // pin 6, PB1
int serialTX = 3;  // pin 2, PB3
int sense    = A1; // pin 7, PB2, ADC1

int analogValue       = 0;

int thresholdRate     = 2;   // how fast we climb when over the trigger level
int threshold         = 600; // ADC trigger level
int thresholdCount    = 0;   // counter
int thresholdCountMax = 10;  // max counter value - affects how long we stay on after the tool is off
int triggerLevel      = 6;   // counter trigger level that turns on vac

boolean blink = false;

SendOnlySoftwareSerial serial(serialTX);

void setup()
{
  serial.begin(9600);
  serial.print("\n\n\n\r");
  pinMode(sense, INPUT);
  pinMode(blinkPin, OUTPUT);
  pinMode(relay, OUTPUT);
}

void loop()
{
  analogValue = analogRead(sense);
  if ((analogValue >= threshold) &&
      (thresholdCount <= thresholdCountMax)) {
    // count up fast
    thresholdCount += thresholdRate;
    blink = true;
  } else {
    blink = false;
  }
  if ((analogValue < threshold) && (thresholdCount > 0 )) {
    // count down slow
    thresholdCount--;
  }
  serialDump();
  if (thresholdCount > triggerLevel) {
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

}

void serialDump() {
  serial.print("\r                               ");
  serial.print("\rValue = ");
  serial.print(analogValue, DEC);
  serial.print(" ThCnt = ");
  serial.print(thresholdCount, DEC);
}
