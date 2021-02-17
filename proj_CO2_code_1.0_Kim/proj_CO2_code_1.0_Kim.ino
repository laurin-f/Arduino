///////////////////////////////////////////////////////////////////////////////////////////////
// This script is currently able to monitor soil moisture content and co2-concentrations     //
// while being connected to a pc via USB or to an external power supply.                     //
// The measuring, converting and logging of the observed data is all implemented in this     //
// script.                                                                                   //
///////////////////////////////////////////////////////////////////////////////////////////////


// Load needed packages -----------------------------------------------------------------------
#include "RTClib.h" //Time
#include "SD.h" //SD-card
#include "SPI.h" //needed by SD library


// Create needed variables --------------------------------------------------------------------
//Time
RTC_Millis rtc; //Defines the real Time Object

//SD variables
const int chipSelect = 10; //Select the pin the SD card uses for communication
  //if Pin 10 is used for something else the SD library will not work
File logfile; //Variable for the logging of data

//Variables for the measured parameters
//Select the Pin which receives the input from the sensor
int co2Pin = A0;
//Variables for conversion from electrical signals to % or ppm
int co2Signal = 0;
int co2_con = 0;


String error_msg, error;
char filename[] = "CO2_log.txt";
int log_intervall_secs = 1;

//Variable for USB connection
#define ECHO_TO_SERIAL 1 //check if Arduino is connected via USB (aka to a PC)
  //if False the lines regarding the Serial Monitor are not executed


// Function to set the timestamp of the DataFile that was created on the SD card -------------
void dateTime(uint16_t* date, uint16_t* time) {
  DateTime now = rtc.now();

  //return date using FAT_DATE macro to format fields
  *date = FAT_DATE(now.year(), now.month(), now.day());

  //return time using FAT_TIME macro to format fields
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}


// Setup the logging File and the Serial Monitor ---------------------------------------------
void setup() {
   rtc.begin(DateTime(F(__DATE__), F(__TIME__))); //Create the starting point for the timestamp of every measurement

   #if ECHO_TO_SERIAL //if USB connection exists do the following:
   Serial.begin(9600); //Activate Serial Monitor

  //Initialize SD card with chipSelect connected to pin 4
  Serial.print("\nInitializing SD card..."); //print to Serial MOnitor
  pinMode(chipSelect, OUTPUT); //Reserve pin 10 (chip select) as an output, dont use it for other parts of circuit
  if (!SD.begin(chipSelect)) { //check if SD card is available and can communicate to Arduino
    Serial.println("Card failed, or not present");
  }
  Serial.println("Card initialized.");
  #endif ECHO_TO_SERIAL

  //Create a new entry to the file in order to log data
  File logfile = SD.open(filename, FILE_WRITE);
  if (logfile) {
    logfile.println("\nNew Log started!");
    logfile.println("Date;Time;co2Sensor;co2_con");
    logfile.close(); //Data is not written until the connection is closed

    #if ECHO_TO_SERIAL //if USB connection exists do the following:
    Serial.print("Logging to: ");
    Serial.println(filename);
    Serial.println("\nNew Log started!");
    Serial.println("Date;Time;co2Sensor;co2_con");
    #endif ECHO_TO_SERIAL
  } else {
    logfile.println("Couldn't open log file in void setup");
    logfile.close(); //Data is not written until the connection is closed
    #if ECHO_TO_SERIAL //if USB connection exists do the following:
    Serial.println("Couldn't open log file in void setup");
    #endif ECHO_TO_SERIAL
  }
  
  // Create a new file after every restart
  //doesnt work at the moment
  //  char filename[] = "Moist00.txt";
  //  Serial.println(SD.exists(filename));
  //  for (uint8_t i = 0; i < 100; i++){
  //    filename[5] = i/10 + '0';
  //    filename[6] = i%10 + '0';
  //    Serial.print(filename);
  //    Serial.print(" exists: ");
  //    Serial.println(SD.exists(filename));
  //    if (!SD.exists(filename)) {
  //      //only open a new file if it doesnt exist
  //      logfile = SD.open(filename, FILE_WRITE);
  //      //Serial.println(filename);
  //      //Serial.println("File exists");
  //      break; //leave the loop!
  //      }
  //      else {
  //        i = i+1;
  //      }
  //  }
}


// Loop to continously observe the measurements ---------------------------------------------
void loop() {
  //as long as the script is running redo the following: 
  SdFile::dateTimeCallback(dateTime); //Update the timestamp of the logging file
  
  co2Signal = analogRead(co2Pin); //Get the input of the CO2 Sensor
  float co2Volt = co2Signal * (5.0 / 1023.0); //translate the CO2 input into an electrical input
  co2_con = ((co2Volt - 0.4) / 1.6) * 5000; //calculate the ppm con via the electrical signal
  printValues(); //print all inputs and calculated values 
  delay(log_intervall_secs * 1000); //repeat all of that every minute (one second == 1000 milliseconds/millis)
}


// Funcion to print values to the Serial Monitor and to the logging file --------------------
void printValues () {
  //rtc.begin(DateTime(F(__DATE__), F(__TIME__)));
  DateTime now = rtc.now(); //Get the current time
  File logfile = SD.open(filename, FILE_WRITE); //
  
  if (logfile) {
    logfile.print(now.year(), DEC);
    logfile.print("/");
    logfile.print(now.month(), DEC);
    logfile.print("/");
    logfile.print(now.day(), DEC);
    logfile.print(";");
    logfile.print(now.hour(), DEC);
    logfile.print(":");
    logfile.print(now.minute(), DEC);
    logfile.print(":");
    logfile.print(now.second(), DEC);
    logfile.print(";");
    logfile.print(co2Signal);
    logfile.print(";");
    logfile.println(co2_con);
    logfile.close();
#if ECHO_TO_SERIAL //if USB connection exists do the following:
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(';');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.print(";");
    Serial.print(co2Signal);
    Serial.print(";");
    Serial.println(co2_con);
#endif //ECHO_TO_SERIAL
  }
  else { //print error message if logging file couldnt be opened
    error = "error opening";
    error_msg = error + filename;
    Serial.println(error_msg);
  }
}
