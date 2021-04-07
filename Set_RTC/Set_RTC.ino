
// Load needed packages -----------------------------------------------------------------------
#include "RTClib.h" //Time


// Create needed variables --------------------------------------------------------------------


//Time
RTC_DS1307 rtc; //Defines the real Time Object

char date_char[] = "yy/mm/dd HH:MM:SS";

//Variable for USB connection
#define ECHO_TO_SERIAL 1 //check if Arduino is connected via USB (aka to a PC)
  //if False the lines regarding the Serial Monitor are not executed

unsigned int baudrate = 38400;


// Setup ----------------------------------------------------------------------
void setup(){
 // datetime -----------------------------------

   if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (! rtc.isrunning()) {
//    Serial.println("RTC is NOT running!");
//    Uhrzeit einmalig adjusten dann auskommentieren
//    Serial.println("RTC adjusted!");
  }
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  


//SD -------------------------------------------------------
   #if ECHO_TO_SERIAL //if USB connection exists do the following:
   Serial.begin(baudrate); //Activate Serial Monitor
   #endif ECHO_TO_SERIAL
}

// loop -----------------------------------------------------
void loop(){
   DateTime now = rtc.now(); //Get the current time
   sprintf(date_char,"%02d/%02d/%02d %02d:%02d:%02d", now.year() % 100, now.month(), now.day(),  now.hour(), now.minute(), now.second());
    
   #if ECHO_TO_SERIAL //if USB connection exists do the following:
   Serial.println(date_char); //Activate Serial Monitor
   #endif ECHO_TO_SERIAL
   delay(1000);
}
