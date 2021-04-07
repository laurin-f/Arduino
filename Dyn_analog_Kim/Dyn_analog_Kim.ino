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
File file; //Variable for the logging of data

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
  File file = SD.open(filename, FILE_WRITE);
  if (file) {
    file.println("\nNew Log started!");
    file.println("Date;Time;co2Sensor;co2_con");
    file.close(); //Data is not written until the connection is closed

    #if ECHO_TO_SERIAL //if USB connection exists do the following:
    Serial.print("Logging to: ");
    Serial.println(filename);
    Serial.println("\nNew Log started!");
    Serial.println("Date;Time;co2Sensor;co2_con");
    #endif ECHO_TO_SERIAL
  } else {
    file.println("Couldn't open log file in void setup");
    file.close(); //Data is not written until the connection is closed
    #if ECHO_TO_SERIAL //if USB connection exists do the following:
    Serial.println("Couldn't open log file in void setup");
    #endif ECHO_TO_SERIAL
      get_filename();//dateiname ist yymmdd.txt und wird hier aktualisiert
      write_header();
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
  //      file = SD.open(filename, FILE_WRITE);
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
  get_filename();//dateiname ist yymmdd.txt und wird hier aktualisiert
  write_header();
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
  File file = SD.open(filename, FILE_WRITE); //
  
  if (file) {
    file.print(now.year(), DEC);
    file.print("/");
    file.print(now.month(), DEC);
    file.print("/");
    file.print(now.day(), DEC);
    file.print(";");
    file.print(now.hour(), DEC);
    file.print(":");
    file.print(now.minute(), DEC);
    file.print(":");
    file.print(now.second(), DEC);
    file.print(";");
//    file.print(co2Signal);
//    file.print(";");
    file.println(co2_con);
    file.close();
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
//    Serial.print(co2Signal);
//    Serial.print(";");
    Serial.println(co2_con);
#endif //ECHO_TO_SERIAL
  }
  else { //print error message if logging file couldnt be opened
    error = "error opening";
    error_msg = error + filename;
    Serial.println(error_msg);
  }
}

//functions --------------------------------------------

void check_SD() {
   pinMode(chipSelect, OUTPUT);
   SD.begin(chipSelect);
   }

void get_filename(){

DateTime now = rtc.now();

filename[0] = (now.year()/10)%10 + '0'; //To get 3rd digit from year()
filename[1] = now.year()%10 + '0'; //To get 4th digit from year()
filename[2] = now.month()/10 + '0'; //To get 1st digit from month()
filename[3] = now.month()%10 + '0'; //To get 2nd digit from month()
filename[4] = now.day()/10 + '0'; //To get 1st digit from day()
filename[5] = now.day()%10 + '0'; //To get 2nd digit from day()
filename[6] = now.hour()/12 + '0'; //To get 1st digit from day()
//filename[7] = now.hour()%10 + '0'; //To get 2nd digit from day()

}

void write_header() {
  if(!SD.exists(filename)){
    File file = SD.open(filename, FILE_WRITE);
    
    file.print("date");
    file.println("CO2_ppm");
    file.close();
  }
}
