#include <SPI.h>
//#include <SD.h>
#include "SdFat.h" //SD-card

#include "RTClib.h"

SdFat sd;


const int LedPin_red = 2; // Digital pin of the indicator LED
const int LedPin_green = 3; // Digital pin of the indicator LED

RTC_DS3231 rtc;
//File myFile;
SdFile myFile; //Variable for the logging of data

int old_second = 0;
int add_mill = 0;
char msg[80]; // define max message size
char filename[] = "yymmdd.TXT";

void setup() {
  pinMode(LedPin_red, OUTPUT);
  pinMode(LedPin_green, OUTPUT);
  
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ;// wait for serial port to connect. Needed for native USB port only
  }

  Serial.print("Initializing SD card...");
  if (!sd.begin(4)) {
  //if (!SD.begin(4)) {
    digitalWrite(LedPin_red, HIGH); 
    Serial.println("initialization failed!");
    //while (1);
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
  if (sd.begin(4)) {

  //Serial.print("test");
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
    //myFile = SD.open(filename, FILE_WRITE);
   
  
    // if the file opened okay, write to it:
    if (myFile.open(filename, O_WRITE | O_APPEND | O_CREAT)) {
      SdFile::dateTimeCallback(dateTime); //Update the timestamp of the logging file
    //if (myFile) {
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
  delay(70); //eigentlich70// Logging interval
  digitalWrite(LedPin_green, LOW); 
  digitalWrite(LedPin_red, LOW); 
  delay(900);//longer logging intervall
  }else{
          // if the file didn't open, print an error:
      digitalWrite(LedPin_red, HIGH); 
      Serial.println("no SD Card");
  }
}

//////////////////////////////////////////////////////////////////////
//////functions //////////////////////////////////////////////////

// Function to set the timestamp of the DataFile that was created on the SD card -------------
void dateTime(uint16_t* date, uint16_t* time) {
  DateTime now = rtc.now();

  //return date using FAT_DATE macro to format fields
  *date = FAT_DATE(now.year(), now.month(), now.day());

  //return time using FAT_TIME macro to format fields
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}

void getFileName(){

  DateTime now = rtc.now();
  
  filename[0] = (now.year()/10)%10 + '0'; //To get 3rd digit from year()
  filename[1] = now.year()%10 + '0'; //To get 4th digit from year()
  
  filename[2] = now.month()/10 + '0'; //To get 1st digit from month()
  filename[3] = now.month()%10 + '0'; //To get 2nd digit from month()

  filename[4] = now.day()/10 + '0'; //To get 1st digit from day()
  filename[5] = now.day()%10 + '0'; //To get 2nd digit from day()

  //filename[6] = now.hour()/10 + '0'; //To get 1st digit from hour()
  //filename[7] = now.hour()%10 + '0'; //To get 2nd digit from hour()
}
