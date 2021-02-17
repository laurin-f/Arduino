#include <SoftwareSerial.h>
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
char filename[] = "CO2_log.txt";

//Variable for USB connection
#define ECHO_TO_SERIAL 1 //check if Arduino is connected via USB (aka to a PC)
  //if False the lines regarding the Serial Monitor are not executed


// Control Bytes -----------------------------------------------------
//Data Link Escape DLE = 0x10 (00010000)
int DLE = 0x10;
//Read RD = 0x13 (00010011)
int RD = 0x13;

//End of Frame EOF = 0x1F (00011111)
int EoF = 0x1F; 
int CheckSum_High = 0x00; 

// live data reads CO2 and Temp live Data simple only reads CO2
//read live Data 0x01 live Data simple 0x06
int VariableID = 0x01;
//int CheckSum_Low = 0x58; //live data simple
int CheckSum_Low = 0x53; //live Data


// other input variables ------------------------------------------------
int intervall_s = 1;
unsigned int baudrate = 38400;


bool print_input = false;
bool print_output = true;

//pins used for Rx and Tx
int Rx = 3;
int Tx = 4;



//live Data simple 15 bytes
//byte inBuffer[15];
//live Data with Temperature 27 bytes
byte inBuffer[15];

byte bufIndx = 0;

byte dataread_CO2[4]; 
byte dataread_temp[4]; 


//Select the Pin which receives the input from the sensor
int co2Pin = A0;
//Variables for conversion from electrical signals to % or ppm
int co2Signal = 0;
int CO2_analog = 0;


SoftwareSerial Serial2(Rx, Tx); //rx tx

byte outData[7] = {DLE, RD, VariableID, DLE, EoF, CheckSum_High, CheckSum_Low};


// Setup ----------------------------------------------------------------------
void setup(){
// establish serial communication -------------------------------------
     //Serial.begin(baudrate); 
      Serial2.begin(baudrate);
      Serial2.flush();
 // datetime -----------------------------------
   rtc.begin(DateTime(F(__DATE__), F(__TIME__))); //Create the starting point for the timestamp of every measurement
//SD -------------------------------------------------------
   #if ECHO_TO_SERIAL //if USB connection exists do the following:
   Serial.begin(baudrate); //Activate Serial Monitor
   

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
    logfile.println("Date;Time;CO2_analog;CO2_dig;temp");
    logfile.close(); //Data is not written until the connection is closed

    #if ECHO_TO_SERIAL //if USB connection exists do the following:
    Serial.print("Logging to: ");
    Serial.println(filename);
    Serial.println("\nNew Log started!");
    Serial.println("Date;Time");
    #endif ECHO_TO_SERIAL
  } else {
    logfile.println("Couldn't open log file in void setup");
    logfile.close(); //Data is not written until the connection is closed
    #if ECHO_TO_SERIAL //if USB connection exists do the following:
    Serial.println("Couldn't open log file in void setup");
    #endif ECHO_TO_SERIAL
  }
}

// loop -----------------------------------------------------
void loop(){

    co2Signal = analogRead(co2Pin); //Get the input of the CO2 Sensor
  float co2Volt = co2Signal * (5.0 / 1023.0); //translate the CO2 input into an electrical input
  CO2_analog = ((co2Volt - 0.4) / 1.6) * 5000; //calculate the ppm con via the electrical signal
// sending bytes -------------------------------------------
if(print_input){
  Serial.print("Sending: ");
  for(int i = 0; i <= (sizeof(outData)-1); i++){
    if(i < (sizeof(outData)-1)){
    Serial.print(outData[i],BIN);
    Serial.print(" ");
  }else{
    Serial.println(outData[i],BIN);
  }
    //Serial2.write(outData[i]);
  }
}
  Serial2.write(outData,(sizeof(outData)));

  // receiving bytes -------------------------------------------------

   if(Serial2.available()){
      while (Serial2.available()) {
          inBuffer[bufIndx] = Serial2.read();

          bufIndx ++;
   }
   bufIndx = 0;

  for(int i = 0; i <= sizeof(dataread_CO2);i++){
    dataread_CO2[i] = inBuffer[i+7];  // extract gas reading from sensor
  }
  float CO2 = *((float *)dataread_CO2); 

    for(int i = 0; i <= sizeof(dataread_CO2);i++){
    dataread_temp[i] = inBuffer[i+11];  // extract gas reading from sensor
  }
  float temp = *((float *)dataread_temp);
   
  if(print_output){
    Serial.print("CO2_dig ");
    Serial.print(CO2,2);
    Serial.print("CO2_analog ");
    Serial.print(CO2_analog,DEC);
    Serial.print(", temp ");
    Serial.println(temp,2); 
  }
  

 // time -------------------------------------
  SdFile::dateTimeCallback(dateTime); //Update the timestamp of the logging file

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
    logfile.print(CO2_analog, DEC);
    logfile.print(";");
    logfile.print(CO2, DEC);
    logfile.print(";");
    logfile.println(temp, DEC);
    logfile.close();
  }
  // wenn kein serial2 available
  }else{
     if(print_output){

    Serial.print("CO2_analog ");
    Serial.println(CO2_analog,DEC); 
  }
  

 // time -------------------------------------
  SdFile::dateTimeCallback(dateTime); //Update the timestamp of the logging file

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
    logfile.print(CO2_analog, DEC);
    logfile.print(";");
    logfile.print("NA");
    logfile.print(";");
    logfile.println("NA");
    logfile.close();
  }
  }
  
  //storeValues();

    delay(1000*intervall_s);
}


//void storeValues () {
//
//}

// Function to set the timestamp of the DataFile that was created on the SD card -------------
void dateTime(uint16_t* date, uint16_t* time) {
  DateTime now = rtc.now();

  //return date using FAT_DATE macro to format fields
  *date = FAT_DATE(now.year(), now.month(), now.day());

  //return time using FAT_TIME macro to format fields
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}
