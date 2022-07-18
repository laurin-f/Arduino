
// Load needed packages -----------------------------------------------------------------------
#include "RTClib.h" //Time
#include "SdFat.h" //SD-card
#include "SPI.h" //needed by SD library
#include <SoftwareSerial.h>


// Create needed variables --------------------------------------------------------------------

// other input variables ------------------------------------------------
int intervall_s = 1;
int intervall_min = 0;
int kammer_intervall = 30; //min
int kammer_closing = 5; //min
int dyn_on = 1; // is dynament sensor turned on or not
int test = 0;
//int pin_test = 2;

//Time
RTC_DS1307 rtc; //Defines the real Time Object

// Pins variables ---------------------------------
const int pin_kammer = 5;
const int pin_dyn_kammer = 8;
const int pin_ventil = 2;


const int rx = 6;
const int tx = 7;
SoftwareSerial Serial2(rx, tx); //rx tx

//CO2 rxtx variables----------------------------------------------------
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

//bytes die an den Dynament gesendet werden
byte out_bytes[7] = {DLE, RD, VariableID, DLE, EoF, CheckSum_High, CheckSum_Low};

//live Data simple 15 bytes
//byte inBuffer[15];
//live Data with Temperature 27 bytes
byte in_bytes[50];

byte bufIndx = 0;

byte CO2_bytes[4]; 
byte temp_bytes[4]; 

 //SD variables----------------------------------------------------
SdFat sd;
const int chipSelect = 10; //Select the pin the SD card uses for communication
  //if Pin 10 is used for something else the SD library will not work
SdFile file; //Variable for the logging of data
char filename[] = "yymmdd_chamber.TXT";
char date_char[] = "yy/mm/dd HH:MM:SS";

//Variable for USB connection
#define ECHO_TO_SERIAL 1 //check if Arduino is connected via USB (aka to a PC)
  //if False the lines regarding the Serial Monitor are not executed
unsigned int baudrate = 38400;


///////////////////////////////////////////////////////////////////////////////
// Setup ----------------------------------------------------------------------
void setup(){
 // datetime -----------------------------------

   if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (! rtc.isrunning()) {
//    Uhrzeit einmalig adjusten dann auskommentieren
//    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
//output pins
  pinMode(chipSelect, OUTPUT);
  pinMode(pin_kammer, OUTPUT);
  pinMode(pin_dyn_kammer, OUTPUT);
  pinMode(pin_ventil, OUTPUT);
  //pinMode(pin_test, OUTPUT);

  digitalWrite(pin_kammer,HIGH);
  digitalWrite(pin_dyn_kammer,LOW);
  digitalWrite(pin_ventil,LOW);
  //digitalWrite(pin_test,HIGH);
//SD -------------------------------------------------------
   #if ECHO_TO_SERIAL //if USB connection exists do the following:
   Serial.begin(baudrate); //Activate Serial Monitor
   #endif ECHO_TO_SERIAL

   Serial2.begin(baudrate);
   Serial2.flush();
   
}

// loop -----------------------------------------------------
void loop(){
  #if ECHO_TO_SERIAL
  Serial.println(test);
  #endif ECHO_TO_SERIAL
//  if(test > 1){
//    digitalWrite(pin_test,LOW);
//  }else{
//    digitalWrite(pin_test,HIGH);
//  }
  if(sd.begin(chipSelect, SPI_HALF_SPEED)){

  get_filename();
  write_header();
  
    DateTime now1 = rtc.now(); //Get the current time
   
    // Warten -------------------------------------------------------------
    if(intervall_min > 0){
    long pause = 1000L*60L*(intervall_min) - now1.second()*1000L - 2*1000L;
    delay(pause);
    }
    if(intervall_s > 0){
      long pause = 1000L*intervall_s;
      if(pause > 0){
      delay(pause);
      }
    }
    DateTime now = rtc.now(); //Get the current time
    sprintf(date_char,"%02d/%02d/%02d %02d:%02d:%02d", now.year() % 100, now.month(), now.day(),  now.hour(), now.minute(), now.second());
    #if ECHO_TO_SERIAL
    Serial.println(date_char);
    #endif ECHO_TO_SERIAL

  //------------------------------------
  //kammer
  if(dyn_on == 1){  
    read_CO2_RxTx();
  }

  test++;
  if((now.minute() - kammer_closing - 2)  % kammer_intervall == 0){
    digitalWrite(pin_dyn_kammer, HIGH);
    dyn_on = 0;
  }
  
  if((now.minute() + 2)  % kammer_intervall == 0){
    digitalWrite(pin_dyn_kammer, LOW);
    dyn_on = 1;
  }
    if(now.minute() % kammer_intervall == 0){
    digitalWrite(pin_kammer,LOW);    
    if(file.open(filename, O_WRITE | O_APPEND)){
      file.print(";1");
      file.close();
    }
    #if ECHO_TO_SERIAL
    Serial.println("closing chamber");
    #endif ECHO_TO_SERIAL
  }else if((now.minute() - kammer_closing) % kammer_intervall == 0){
    digitalWrite(pin_kammer,HIGH);
    if(dyn_on){
      if(file.open(filename, O_WRITE | O_APPEND)){
        file.print(";0");
        file.close();
      }
    }
    #if ECHO_TO_SERIAL
    Serial.println("opening chamber");
    #endif ECHO_TO_SERIAL
  }else{
    if(dyn_on){
    if(file.open(filename, O_WRITE | O_APPEND)){
      if(digitalRead(pin_kammer)){
        file.print(";0");
      }else{
        file.print(";1");
      }
      file.close();
    }
    }
  }

 }else{
  digitalWrite(pin_ventil,LOW);
  delay(1000);
  digitalWrite(pin_ventil,HIGH);
  delay(4000);
    
 }
}

  //functions---------------------------------------------------------------------------------------------


// Function to set the timestamp of the DataFile that was created on the SD card -------------
void dateTime(uint16_t* date, uint16_t* time) {
  DateTime now = rtc.now();

  //return date using FAT_DATE macro to format fields
  *date = FAT_DATE(now.year(), now.month(), now.day());

  //return time using FAT_TIME macro to format fields
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}


void get_filename(){

DateTime now = rtc.now();

filename[0] = (now.year()/10)%10 + '0'; //To get 3rd digit from year()
filename[1] = now.year()%10 + '0'; //To get 4th digit from year()
filename[2] = now.month()/10 + '0'; //To get 1st digit from month()
filename[3] = now.month()%10 + '0'; //To get 2nd digit from month()
filename[4] = now.day()/10 + '0'; //To get 1st digit from day()
filename[5] = now.day()%10 + '0'; //To get 2nd digit from day()
//filename[6] = now.hour()/12 + '0'; //To get 1st digit from day()
//filename[7] = now.hour()%10 + '0'; //To get 2nd digit from day()

}

void write_header() {
  if(!sd.exists(filename)){
    file.open(filename, O_WRITE | O_CREAT | O_EXCL);
    
    file.print("date; CO2_ppm; temp_C; chamber");
    file.close();
  }
}
  
void read_CO2_RxTx() {
    //Datei öffnen (ACHTUNG die Datei muss immer wieder geschlossen werden sonst treten Fehler auf!!)
    if(file.open(filename, O_WRITE | O_APPEND)){
      SdFile::dateTimeCallback(dateTime); //Update the timestamp of the logging file
      // Messwerte auslesen ------------------------------------------------------------------------

    //read CO2 signal with Serial Communication-----------------------------------------------
    // sending bytes ------------------------------------------- 
    Serial2.write(out_bytes,(sizeof(out_bytes)));
    Serial.flush();
    // receiving bytes -------------------------------------------------
    if(Serial2.available()){
      //so lange Serial2 available werden byte für byte abgerufen
      while (Serial2.available()) {
          in_bytes[bufIndx] = Serial2.read();
          //der buffer Index wird jedes mal um 1 erhöht 
        if(bufIndx <= 39){
          bufIndx ++;
        }
   }
    #if ECHO_TO_SERIAL
        Serial.print("bufIndx");
        Serial.print(bufIndx);
        Serial.print(" ");
     #endif ECHO_TO_SERIAL
//        file.print("bufIndx:");
//        file.print(bufIndx);
//        file.print(" ");
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
  
   // Messwerte ausgeben --------------------------------------------
   //signal an PC console ------------------------------------------
   #if ECHO_TO_SERIAL
    Serial.print(date_char);
    Serial.print(";");
    Serial.print("CO2: ");
    Serial.print(CO2,0);
    Serial.print(", temp:");
    Serial.println(temp,2);    
  #endif ECHO_TO_SERIAL
  
  //Werte in logfile schreiben ------------------------------------------
    file.println("");
    file.print(date_char);
    file.print(";");

    file.print(CO2, 0);
    file.print(";");
    file.print(temp, 2);

    file.close();
      
  // wenn keine CO2 Messwerte vorhanden sind (kein serial2 available)-----------------
  }else{
      file.println("");
      file.print(date_char);
      file.print(";NA;NA");
      file.close();
   #if ECHO_TO_SERIAL
        Serial.print("bufIndx");
        Serial.print(bufIndx);
    Serial.print(date_char);
    Serial.println("; no data signal");   
  #endif ECHO_TO_SERIAL
  }//ende serial2.available
  }//ende file.open
}
