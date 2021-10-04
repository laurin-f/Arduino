//-------------------------------------------------------------------------------------------//
//  Script um die im DataloggerShield integrierte RealTimeClock RTC zu stellen               //    
// dieses Script muss einmal auf den Arduino gespielt werden                                 //
// und danach von einem anderen Script überschrieben werden                                  //
//                        scripted by Laurin Osterholt                                       // 
//-------------------------------------------------------------------------------------------//


// Load needed packages -----------------------------------------------------------------------
#include "RTClib.h" //Time


// Create needed variables --------------------------------------------------------------------


//Time
RTC_DS1307 rtc; //Defines the real Time Object

// character fur formatiertes Datum
char date_char[] = "yy/mm/dd HH:MM:SS";

//Variable for USB connection
#define ECHO_TO_SERIAL 1 //check if Arduino is connected via USB (aka to a PC)
  //if False the lines regarding the Serial Monitor are not executed

//baudrate definieren
unsigned int baudrate = 38400;


// Setup ----------------------------------------------------------------------
void setup(){
 // adjust reals time clock -----------------------------------
    rtc.begin();
    rtc.isrunning();
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
// -------------------------------------------------------
   #if ECHO_TO_SERIAL //if USB connection exists do the following:
   Serial.begin(baudrate); //Activate Serial Monitor
   #endif ECHO_TO_SERIAL
}

// loop -----------------------------------------------------
void loop(){

  //Get the current time
   DateTime now = rtc.now();
   //datum formatiert in date_char schreiben 
   sprintf(date_char,"%02d/%02d/%02d %02d:%02d:%02d", now.year() % 100, now.month(), now.day(),  now.hour(), now.minute(), now.second());

   //wenn USB connection existiert dann wird das datum angezeigt
   #if ECHO_TO_SERIAL //if USB connection exists do the following:
   Serial.println(date_char); //Activate Serial Monitor
   #endif ECHO_TO_SERIAL
   // eine s warten (1000ms)
   delay(1000);
}
