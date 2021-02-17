// Load needed packages -----------------------------------------------------------------------
#include <SoftwareSerial.h>
#include "RTClib.h" //Time
#include "SD.h" //SD-card
#include "SPI.h" //needed by SD library


// Create needed variables --------------------------------------------------------------------
//Time
RTC_DS1307 rtc; //Defines the real Time Object

//SD variables
const int chipSelect = 10; //Select the pin the SD card uses for communication
  //if Pin 10 is used for something else the SD library will not work
File file; //Variable for the logging of data
//char filename[] = "yymmddhh.txt";
char filename[] = "yymmddh.txt";

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


byte out_bytes[7] = {DLE, RD, VariableID, DLE, EoF, CheckSum_High, CheckSum_Low};

// other input variables ------------------------------------------------
int intervall_s = 0;
int intervall_min = 1;
unsigned int baudrate = 38400;
long min_break = 400L;

//pins -----------------------------------
//pins used for Rx and Tx
int Rx = 7;
int Tx = 2;
SoftwareSerial Serial2(Rx, Tx); //rx tx

//control Pins to select port of serial expander
int s1 = 5;                                           //Arduino pin 6 to control pin S1
int s2 = 4;                                           //Arduino pin 5 to control pin S2
int s3 = 3;                                           //Arduino pin 4 to control pin S3
int port = 1;                                         //what port to open
int n_ports = 8;    // number of ports


//input variables --------------------------
//live Data simple 15 bytes
//byte in_bytes[15];
//live Data with Temperature 27 bytes
byte in_bytes[27];
// buffer index to fill in_bytes byte by byte
byte bufIndx = 0;

byte CO2_bytes[4]; 
byte temp_bytes[4]; 




// Setup ----------------------------------------------------------------------
void setup(){
// establish serial communication -------------------------------------
      Serial2.begin(baudrate);
      Serial2.flush();
 // datetime -----------------------------------
   if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
  }
  
//output pins
  //pinMode(chipSelect, OUTPUT); //Reserve pin 10 (chip select) as an output, dont use it for other parts of circuit
  pinMode(s1, OUTPUT);                                //Set the digital pin as output
  pinMode(s2, OUTPUT);                                //Set the digital pin as output
  pinMode(s3, OUTPUT);                                //Set the digital pin as output
  pinMode(LED_BUILTIN, OUTPUT);

//SD -------------------------------------------------------
   #if ECHO_TO_SERIAL //if USB connection exists do the following:
   Serial.begin(baudrate); //Activate Serial Monitor
   #endif ECHO_TO_SERIAL
   
  check_SD();
  get_filename();//dateiname ist yymmdd.txt und wird hier aktualisiert
  write_header();
}

// loop -----------------------------------------------------
void loop(){
  if(port == 1){
  get_filename();
  
  write_header();
  }

    File file = SD.open(filename, FILE_WRITE); //
    
  if(file){
  
   // time -------------------------------------
  SdFile::dateTimeCallback(dateTime); //Update the timestamp of the logging file

  //rtc.begin(DateTime(F(__DATE__), F(__TIME__)));
  DateTime now = rtc.now(); //Get the current time
    //open right port
  open_port();
  //print port number
  if(port > 1){
    #if ECHO_TO_SERIAL 
    Serial.print("p");
    //irgendwie kommt Sensor x erst bei port x-1 an
    Serial.print(port-1);
    Serial.print(": ");
    #endif ECHO_TO_SERIAL 
  }
  // print Datetime only when port is 1
  if(port == 1){
    // Warten
    if(intervall_min > 0){
    long pause = 1000L*60L*(intervall_min) - now.second()*1000L - 2*1000L;
    delay(pause);
    }
    if(intervall_s > 0){
      long pause = 1000L*intervall_s - n_ports*min_break;
      if(pause > 0){
      delay(pause);
      }
    }
    
    file.println("");
    file.print(now.year(), DEC);
    file.print("/");
    file.print(now.month(), DEC);
    file.print("/");
    file.print(now.day(), DEC);
    file.print(" ");
    file.print(now.hour(), DEC);
    file.print(":");
    file.print(now.minute(), DEC);
    file.print(":");
    file.print(now.second(), DEC);
    file.print(";");
    file.flush();
  
  #if ECHO_TO_SERIAL
    Serial.println("");
    Serial.print(now.year(), DEC);
    Serial.print("/");
    Serial.print(now.month(), DEC);
    Serial.print("/");
    Serial.print(now.day(), DEC);
    Serial.print(" ");
    Serial.print(now.hour(), DEC);
    Serial.print(":");
    Serial.print(now.minute(), DEC);
    Serial.print(":");
    Serial.print(now.second(), DEC);
    Serial.print(";");
    #endif ECHO_TO_SERIAL
  }
// sending bytes ------------------------------------------- 
  Serial2.write(out_bytes,(sizeof(out_bytes)));

// receiving bytes -------------------------------------------------
   if(Serial2.available()){
    //so lange Serial2 available werden bite für byte abgerufen
      while (Serial2.available()) {
          in_bytes[bufIndx] = Serial2.read();
          //der buffer Index wird jedes mal um 1 erhöht
          bufIndx ++;
   }
   //am Ende wird bufInx wieder auf 0 gesetzt
   bufIndx = 0;

  //die CO2 Werte stecken an Position 7 bis 10
  for(int i = 0; i <= sizeof(CO2_bytes);i++){
    CO2_bytes[i] = in_bytes[i+7];  // extract gas reading from sensor
  }
  //die Bytes in ein float umwandeln 
  float CO2 = *((float *)CO2_bytes); 

  //die temperatur Werte stecken an Position 11 bis 14
  for(int i = 0; i <= sizeof(CO2_bytes);i++){
    temp_bytes[i] = in_bytes[i+11];  // extract gas reading from sensor
  }
  float temp = *((float *)temp_bytes);

  //in_bytes werden wieder auf 0 gesetzt
  for(int i = 0; i <= (sizeof(in_bytes)); i++){
    in_bytes[i] = 0;
  }
  if(port > 1){
   //signal an PC console ------------------------------------------
   #if ECHO_TO_SERIAL
    Serial.print("CO2: ");
    Serial.print(CO2,2);
    Serial.print(", temp :");
    Serial.print(temp,2);
    Serial.print(", ");     
  #endif ECHO_TO_SERIAL

  //Werte in logfile schreiben ------------------------------------------
    file.print(CO2, 2);
    file.print(";");
    file.print(temp, 2);
          if(port < n_ports){ 
          file.print(";");
      }
  }
      file.close();
  // wenn kein serial2 available
  }else{
    if(port > 1){
      file.print("NA;NA");

        if(port < n_ports){ 
            file.print(";");
        }
      }
      file.close();
    
  }

  //kurz Warten -----------------------------------------------------------
    delay(min_break);

  // change port to next number
  if(port < n_ports){
  port++;
}else{
  port = 1;
}
  }else{
    port = 2;
    open_port();
    check_SD();
    //delay(200);
    port = 1;
    open_port();
    delay(200);
  }
}


//functions---------------------------------------------------------------------------------------------
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

void open_port() {                                  //this function controls what UART port is opened.

  //if (port < 1 || port > 8)port = 1;                //if the value of the port is within range (1-8) then open that port. If it’s not in range set it to port 1
  int port_num = port - 1;                                        //the multiplexer used on the serial port expander refers to its ports as 0-15, but we have them labeled 1-16 by subtracting one from the port to be opened we correct for this.

  digitalWrite(s1, bitRead(port_num, 0));               //Here we have two commands combined into one.
  digitalWrite(s2, bitRead(port_num, 1));               //The digitalWrite command sets a pin to 1/0 (high or low)
  digitalWrite(s3, bitRead(port_num, 2));               //The bitRead command tells us what the bit value is for a specific bit location of a number
  delay(20);                                         //this is needed to make sure the channel switching event has completed
}

//funktion to check if SD file is available ----------------------------------------------------
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
    //tiefen 1 bis n_ports
    for(int i = 1; i <= (n_ports-1); i++){
      file.print(";CO2_tiefe");
      file.print(i);
      file.print(";temp_tiefe");
      file.print(i);
    }
    file.close();
  }
}
