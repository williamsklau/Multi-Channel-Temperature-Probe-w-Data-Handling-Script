/* 4 Channel DS18B20 Temperature Data Logger with Display

 Designed & Programmed by William Lau
 7/17/2017

 Hardware - Arduino Uno 
          - 16x2 Arduino Keypad LCD shield
          - 9V~12V DC, minimum 250mA Power Supply
          - 4 DS18B20 temperature sensors
          - a 4K7 1/4W resistor 

  This program performs the following tasks:
  1. Request & sends data from the DS18B20 to the computer through serial. It has a minimum delay of 0.45 seconds.
  2. Displays temperature on a LCD screen

      *Note: Sensor will send value of -127.0 or 85.0 if there is a problem connecting to a sensor.
      
  3. Displays an error message if a sensor is not detected
  4. Automatically turn off backlight after a set period of time

*/

/***************** IMPORTANT *************************

16 x 2 LCD Keypad Shield Backlight Bug

D10 (digital pin I/O 10) is used for controlling backlight of the LCD.
Normally, D10 is set to OUTPUT and set HIGH to turn on and LOW to turn off.

However, there is a critical bug and as soon as D10 is set to OUTPUT and HIGH,
there is a significant amount of current drawn to MCU and it could damage or shorten the life of MCU.

To bypass this problem, following has been changed.

1. To set the backlight on, set pin 10 as input.
2. To set the backlight off, set pin 10 as output, and immediately set output low (never high!)

******************************************************/

//Library Headers
#include <OneWire.h>
#include <DallasTemperature.h>  // DS18B20 library
#include <LiquidCrystal.h>  // LCD library
#include <DS3231.h>

#define ONE_WIRE_BUS 2 //sets I2C to PORT 2

OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);  // Pass our oneWire reference to Dallas Temperature. 

// initialize LiquidCrystal.h by associating any needed LCD interface pin with the arduino pin number
const int rs = 8, en = 9, d4 = 4, d5 = 5, d6 = 6, d7 = 7;   
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Init the DS3231 using the hardware interface
DS3231  rtc(SDA, SCL);

// Global vars used in backlight control
const byte interruptPin = 3;
int backlightPin = 10;
int limit = 30;   // limits how many loop()cycles the backlight before turning off
bool flag = 1;    // flag sets high when interruptPin has NGT
volatile int count = 0; // counts up to value of limit before turning backlight off
double secPerSample = -1;  // in mS. used to generate an accurate timing interval
//char serialRX = 0;
unsigned long timeVal, timestart;

//========================Initial SETUP================================
void setup(void) { 
  Serial.begin(19200); // start serial port
  sensors.begin();  // initialize i2c comm 
  lcd.begin(16, 2);  // declares the LCD's number of columns and rows:
  rtc.begin();

  attachInterrupt(digitalPinToInterrupt(interruptPin), backlightISR, FALLING);   // Enable PIN3 ISR on a NGT

  lcd.print("Start program   ");
  lcd.setCursor(0,1);
  lcd.print("to log data. :) ");

  // Wait for log rate
  portNumConnect();

  lcd.clear();
  bootupText();
  
  secPerSample *= 1000;	// convert secPerSample to mSecs

  pinMode(backlightPin, INPUT); // See important note above
  pinMode(interruptPin, INPUT_PULLUP); 
  lcd.clear();  
  printTNum();

  sensors.setResolution(11);  // set global ADC resolution to 11-bits 
  timestart = rtc.getUnixTime(rtc.getTime());  
}


//================= PROGRAM LOOP =================================
void loop(void) 
{ 
    float t1, t0;
    t0 = millis();
	
    sensors.requestTemperatures();    // start temp conversion on all device
    sensors.setWaitForConversion(1); // set WaitForConversion flag high
    
    printSerial();	// send data to serial port

    // Sudo ISR for auto backlight shutoff
    if( flag  == 1 && count < limit){
      count++;
    }
    else{		
	  pinMode(backlightPin, OUTPUT);
      digitalWrite(backlightPin, 0);
      count = 0;
      flag = 0;
    }
    
    printLCD();

    t1 = millis();
    if((t1 - t0) < secPerSample){
       delay(secPerSample - (t1 - t0));
    }
}

//=========================== backlightISR ================================
//===== This function sets the global variable "flag" high when       =====
//===== interruptPin has a NGT.                                       =====
//=====                                                               =====
//===== Parameter: None                                               =====
//===== Return: None                                                  =====
//=========================================================================
void backlightISR(){
  pinMode(backlightPin, INPUT);
  flag = 1;
}

//=========================== printTNum ===================================
//===== This function prints "T1", "T2", "T3", "T4" to the LCD        =====
//=====                                                               =====
//===== Parameter: None                                               =====
//===== Return: None                                                  =====
//=========================================================================
void printTNum(void){
   // display T1, T2, T3,T4
  lcd.setCursor(0,0);
  lcd.print("1:");
  
  lcd.setCursor(10,0);
  lcd.print("2:");

  lcd.setCursor(0,1);
  lcd.print("3:");
  
  lcd.setCursor(10,1);
  lcd.print("4:");
}

//=========================== printLCD ====================================
//===== This function prints data sent from DS18B20 to LCD.           =====
//===== DS18B20 returns -127.0 or 85.0 if sensor is not detected      =====
//=====                                                               =====
//===== Parameter: None                                               =====
//===== Return: None                                                  =====
//=========================================================================
void printLCD(){
  float data;
  // Set cursor & prints sensor value to LCD display
  for(int n = 0; n < 4; n++){
    switch(n){
      case 0:
        lcd.setCursor(2,0);
        break;
      case 1:
        lcd.setCursor(12,0);
        break;
      case 2:
        lcd.setCursor(2,1);
        break;
      case 3:
        lcd.setCursor(12,1);
        break;
     }   
     
    data = sensors.getTempCByIndex(n);
    if(data != -127.00 || data != 85.00){
      lcd.print(data, 1); // prints sensor data to 1 decimal place
    }
    else {
      lcd.print("Err");
    }
  } 
  
  lcd.setCursor(6,0);
  lcd.print("  ");
  lcd.setCursor(6,1);
  lcd.print("  ");
}

//=========================== printSerial==================================
//===== This function sends data from DS18B20 to serial port.         =====
//===== This data will be read & compiled by a C script               =====
//=====                                                               =====
//===== Parameter: None                                               =====
//===== Return: None                                                  =====
//=========================================================================
void printSerial(){
    // Send data to serial port 
    if(sensors.isConversionComplete() == 1){
      timeVal = rtc.getUnixTime(rtc.getTime()) - timestart;   // calculate time passed since logging began     
      Serial.print(rtc.getTimeStr());
      Serial.print(" , ");  
      Serial.print(timeVal);
      Serial.print(" , "); 
      Serial.print(sensors.getTempCByIndex(0), 1);
      Serial.print(" , "); 
      Serial.print(sensors.getTempCByIndex(1), 1);
      Serial.print(" , ");
      Serial.print(sensors.getTempCByIndex(2), 1);
      Serial.print(" , "); 
      Serial.print(sensors.getTempCByIndex(3), 1);  
      Serial.print("\n"); // serial must send '\n' instead of println() to start new set of data
    }
}

//=========================== bootupText===================================
//===== This function sends the bootup text to the LCD display        =====
//=====                                                               =====
//===== Parameter: None                                               =====
//===== Return: None                                                  =====
//=========================================================================
void bootupText(){
  // Display bootup text
  lcd.clear(); 
  lcd.print("Temp. Logger");
  lcd.setCursor(0,1);
  lcd.print("Designed by WL");
  delay(2000); 

  
  // Display # of sensors detected
  lcd.clear(); 
  lcd.print("Sensors detected");
  lcd.setCursor(0,1);
  lcd.print(sensors.getDeviceCount());
  delay(2000); 
  
  lcd.clear();
  lcd.print("Sec per Sample:");
  lcd.setCursor(0, 1);
  lcd.print(secPerSample);
  delay(2000);
}

//=========================== portNumConnect===============================
//===== This function transmits a handshake char continously and      =====
//===== stops when a value between 1 to 127 is received.	  		      =====	
//=====                                                               =====
//===== Parameter: None                                               =====
//===== Return: None                                                  =====
//=========================================================================
void portNumConnect(){
  while(secPerSample < 1 || secPerSample > 127){
    Serial.print("W");
    delay(5);
    secPerSample = Serial.read();
  }
}
