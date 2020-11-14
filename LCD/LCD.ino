// Interfacing ESP8266 (ESP-01) WiFi module with 16x2 I2C LCD


// Como usar TX y RX para I2C
// https://www.instructables.com/How-to-use-the-ESP8266-01-pins/
 
#include <Wire.h>                    // Include Wire library (required for I2C devices)
#include <LiquidCrystal_I2C.h>       // Include LiquidCrystal_I2C library 
 
LiquidCrystal_I2C lcd(0x38, 16, 2);  // Configure LiquidCrystal_I2C library with 0x38 address, 16 columns and 2 rows
 
void setup() {
  /*delay(1000);
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Initializing...");
  */
  //lcd.begin(0, 2);                   // Initialize I2C LCD module (SDA = GPIO0, SCL = GPIO2)
  lcd.begin(1, 3);                   // Initialize I2C LCD module (SDA = GPIO1 = TX, SCL = GPIO3 = RX)
 
  lcd.backlight();                   // Turn backlight ON
 
  lcd.setCursor(0, 0);               // Go to column 0, row 0
  lcd.print("ESP-01 I2C LCD");
  //Serial.println("Ends setup");
}
 
byte i = 0;
char text[4];
 
void loop() {

  //Serial.println(".");
  //lcd.noBacklight();
  sprintf(text, "%03u", i++);
  lcd.setCursor(6, 1);               // Go to column 6, row 1
  lcd.print(text);
  delay(500);
  //lcd.backlight();
  //delay(500);
 
}
