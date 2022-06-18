#include <LiquidCrystal_I2C.h>

// set the LCD number of columns and rows
int lcdColumns = 20;
int lcdRows = 4;

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  

String messageStatic = "** Workshop Alarm **";
String messageToScroll = "Gary was here :)";

// Function to scroll text
// The function acepts the following arguments:
// row: row number where the text will be displayed
// message: message to scroll
// delayTime: delay between each character shifting
// lcdColumns: number of columns of your LCD
void scrollText(int row, String message, int delayTime, int lcdColumns) {
  for (int i=0; i < lcdColumns; i++) {
    message = " " + message;  
  } 
  message = message + " "; 
  for (int pos = 0; pos < message.length(); pos++) {
    lcd.setCursor(0, row);
    lcd.print(message.substring(pos, pos + lcdColumns));
    delay(delayTime);
  }
}

void setup(){
  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();

  // set cursor to first column, first row
  lcd.setCursor(0, 0);
  // print static message
  lcd.print(messageStatic);
  
  lcd.setCursor(0, 2);
  // print static message
  lcd.print(messageStatic);
  
  lcd.setCursor(0, 3);
  // print static message
  lcd.print(messageStatic);
}

void loop(){
  // print scrolling message
  scrollText(1, messageToScroll, 250, lcdColumns);
  lcd.setCursor(0, 2);
}
