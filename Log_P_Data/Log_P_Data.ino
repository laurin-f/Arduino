#include <SPI.h>
#include <SD.h>
#include "RTClib.h"


const int LedPin_red = 2; // Digital pin of the indicator LED
const int LedPin_green = 3; // Digital pin of the indicator LED

RTC_DS3231 rtc;
File myFile;

int old_second = 0;
int add_mill = 0;
char msg[80]; // define max message size
char filename[] = "00000000.txt";

void setup() {
  pinMode(LedPin_red, OUTPUT);
  pinMode(LedPin_green, OUTPUT);
  
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ;// wait for serial port to connect. Needed for native USB port only
  }

  Serial.print("Initializing SD card...");
  if (!SD.begin(4)) {
    digitalWrite(LedPin_red, HIGH); 
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  if (! rtc.begin()) {
    digitalWrite(LedPin_red, HIGH); 
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  // When time needs to be set on a new device, or after a power loss, the
  // following line sets the RTC to the date & time this sketch was compiled
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));


  digitalWrite(LedPin_red, LOW); 
  digitalWrite(LedPin_green, LOW); 
}

void loop() {
  DateTime now = rtc.now();

  int year_str = now.year();
  int month_str = now.month();
  int day_str = now.day();

  int hour_str = now.hour();
  int minute_str = now.minute();
  int second_str = now.second();

  int analog_0 = analogRead(A0);
  int analog_1 = analogRead(A1);
  int analog_2 = analogRead(A2);
  int analog_3 = analogRead(A3);
  int analog_6 = analogRead(A6);
  int analog_7 = analogRead(A7);

  if (old_second == second_str){
    ++add_mill;
  } else {
    add_mill = 0;
    digitalWrite(LedPin_green, HIGH); 
  }

  old_second = second_str;
 
  if (add_mill < 10){
    snprintf(msg, 75, "%d;%d;%d;%d;%d;%d;%d-%d-%d %d:%d:%d.%d", analog_0, analog_1, analog_2, analog_3, analog_6, analog_7, year_str, month_str, day_str, hour_str, minute_str, second_str, add_mill); 
    Serial.println(msg);
      
    getFileName();
    //Serial.println(filename);
    
    myFile = SD.open(filename, FILE_WRITE);
  
    // if the file opened okay, write to it:
    if (myFile) {
      //Serial.println("Writing to file ...");
      myFile.println(msg);
      //digitalWrite(LedPin_red, HIGH); 
      // close the file:
      myFile.close();
      //Serial.println("done.");
    } else {
      // if the file didn't open, print an error:
      digitalWrite(LedPin_red, HIGH); 
      Serial.println("error opening file");
    }
  }
  delay(70); // Logging interval
  digitalWrite(LedPin_red, LOW); 
  digitalWrite(LedPin_green, LOW); 
}

void getFileName(){

  DateTime now = rtc.now();
  
  filename[0] = (now.year()/10)%10 + '0'; //To get 3rd digit from year()
  filename[1] = now.year()%10 + '0'; //To get 4th digit from year()
  
  filename[2] = now.month()/10 + '0'; //To get 1st digit from month()
  filename[3] = now.month()%10 + '0'; //To get 2nd digit from month()

  filename[4] = now.day()/10 + '0'; //To get 1st digit from day()
  filename[5] = now.day()%10 + '0'; //To get 2nd digit from day()

  filename[6] = now.hour()/10 + '0'; //To get 1st digit from hour()
  filename[7] = now.hour()%10 + '0'; //To get 2nd digit from hour()
}
