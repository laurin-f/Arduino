
// Load needed packages -----------------------------------------------------------------------
#include "RTClib.h" //Time
#include "SdFat.h" //SD-card
#include "SPI.h" //needed by SD library
#include <SoftwareSerial.h>

// Create needed variables --------------------------------------------------------------------

// other input variables ------------------------------------------------
int intervall_s = 1;
int intervall_min = 0;
int relais_h = 6; //Pause zwischen inj messungen in stunden
int ventil_mins = 2;//6; //Zeitraum in dem das ventil offen ist und die inj Kammer misst
int pumpe_mins = 1; //how many minutes does the pump pump
int kammer_intervall = 5;//10; //min
int kammer_closing = 1; //min
int dyn_on = 1; // is dynament sensor turned on or not
//int init_01 = 1; // hilfsvariable um t_init abzuwarten

//initial measurements
int counter = 0;//counts the initial measurements
int meas = 0;//changes between 0 = Pump and 1 =  Measurement 
int n_counts = 5;//number of initial measurements
int t_init = 0;//minutes before first measurement

//Time
RTC_DS1307 rtc; //Defines the real Time Object

int now_init = 0;
// Pins variables
const float VRefer = 5;       // voltage of adc reference
const int pinAdc   = A0;
const int pin_ventil = 2;
const int pin_pumpe = 3;
const int pin_dyn = 4;
const int pin_kammer = 5;
const int pin_dyn_kammer = 8;

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
char filename[] = "yymmdd_inj.TXT";
char filename_chamber[] = "yymmdd_chamber.TXT";
char date_char[] = "yy/mm/dd HH:MM:SS";

//Variable for USB connection
#define ECHO_TO_SERIAL 1 //0 no signal to USB 1 signal to USB
  //if False the lines regarding the Serial Monitor are not executed
unsigned int baudrate = 38400;



// Setup ----------------------------------------------------------------------
void setup(){
 // datetime -----------------------------------

   if (! rtc.begin()) {
    #if ECHO_TO_SERIAL
    Serial.println("Couldn't find RTC");
    #endif ECHO_TO_SERIAL
    while (1);
  }

  if (! rtc.isrunning()) {
//    Uhrzeit einmalig adjusten dann auskommentieren
//    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
//output pins
  pinMode(chipSelect, OUTPUT);
  pinMode(pin_ventil, OUTPUT);
  pinMode(pin_pumpe, OUTPUT);
  pinMode(pin_dyn, OUTPUT);
  pinMode(pin_kammer, OUTPUT);
  pinMode(pin_dyn_kammer, OUTPUT);

  digitalWrite(pin_kammer,HIGH);
  digitalWrite(pin_dyn_kammer,LOW);
  digitalWrite(pin_ventil,HIGH);
  digitalWrite(pin_pumpe,HIGH);
  digitalWrite(pin_dyn,LOW);
//SD -------------------------------------------------------
   #if ECHO_TO_SERIAL //if USB connection exists do the following:
   Serial.begin(baudrate); //Activate Serial Monitor
   #endif ECHO_TO_SERIAL
   
   Serial2.begin(baudrate);
   Serial2.flush();
   //DateTime now = rtc.now(); //Get the current time
   //now_init = now.minute(); //Get the current time
}

// loop -----------------------------------------------------
void loop(){
  if(sd.begin(chipSelect, SPI_HALF_SPEED)){

  get_filename();
  get_filename_chamber();
  write_header();
  write_header_chamber();
  
//  write_header(filename, "date; CO2_ppm");
//  write_header(filename_chamber, "date; CO2_ppm; temp_C");
//  
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
    #if ECHO_TO_SERIAL
    Serial.println("chamber closed");
    #endif ECHO_TO_SERIAL
  }
  if((now.minute() - kammer_closing) % kammer_intervall == 0){
    digitalWrite(pin_kammer,HIGH);
    #if ECHO_TO_SERIAL
    Serial.println("chamber open");
    #endif ECHO_TO_SERIAL
  }
  
  ///----------------------------------------------
  #if ECHO_TO_SERIAL
  Serial.print("meas: ");
  Serial.println(meas);
  Serial.print("counter: ");
  Serial.println(counter);
  #endif ECHO_TO_SERIAL
//initial measurements
// am anfang wird erstmal t_init gewartet damit der Schlauch sich mit CO2 fuellt
  if(counter == 0){
//    if(init_01 == 0){
//    now_init = now.minute(); //Get the current time
//    Serial.print("t_init_01:");
//    Serial.print(init_01);
//    init_01 = 1;
//    }
//    
//    Serial.println("");
//   
//    Serial.print("now.minute ");
//    Serial.print(now.minute());
//    Serial.print("init_time ");    
//    Serial.print(now_init);
//    Serial.print("diff");    
//    Serial.println(now.minute() -now_init);
//    
//    if(now.minute() - now_init == t_init | now.minute() - now_init == t_init - 60 ){
//      Serial.print("counter++");
//      counter++;
//    }
    
    long pause = t_init * 60L*1000L;
    #if ECHO_TO_SERIAL
    Serial.print("pause: ");
    Serial.println(t_init);
    #endif ECHO_TO_SERIAL
    delay(pause);
    counter++;
  }else if(counter <= n_counts | (counter == (n_counts +1) & now.minute() % ventil_mins == 0)){
    //dann werden n_counts messungen gemacht bevor nur noch alle relais_h stunden eine messung gemacht wird
    //wenn counter n_counts +1 ist dann wird noch ein letztes mal leergepumpt
    
    //relais 1 off and on times ---------------------------------------------
    // ventil geht in die Inj Kamer 
    //now.minute % ventil_mins != 0 heisst immer wenn die jetzige minute nicht ohne rest durch ventil_mins teilbar ist
    if(now.minute() % ventil_mins != 0){
      digitalWrite(pin_ventil,LOW);
      meas = 1;
    }else{
      digitalWrite(pin_ventil,HIGH);
    }
    // relais 2 off and on time---------------------------
    //Pumpe geht an und spuehlt inj_kammer
  if(now.minute() % ventil_mins == 0){
      digitalWrite(pin_pumpe,LOW);
      //damit nur einmal der counter hochgesetzt wird dient meas als hilfsvariable die immer nach der messung wieder auf 0 gesetzt wird
      if(meas == 1){
        counter++;
      }
      meas = 0;
    }else{
      digitalWrite(pin_pumpe,HIGH);
    }
    //CO2 auslesen und in Datei schreiben
    if(file.open(filename, O_WRITE | O_APPEND)){
  
   // time -------------------------------------
    SdFile::dateTimeCallback(dateTime); //Update the timestamp of the logging file
    //Datum print---------------------------------------------------------------
    file.println("");
    file.print(date_char);
    file.print(";");
    //file.flush();
  
    #if ECHO_TO_SERIAL
    //Serial.println("");
    Serial.print(date_char);
    Serial.print(";");
    #endif ECHO_TO_SERIAL

    
    // read CO2 Anaolog signal---------------------------------------------------
    float CO2 = readCO2();


   //signal an PC console ------------------------------------------
   #if ECHO_TO_SERIAL
    Serial.print(" CO2: ");
    Serial.println(CO2,0);   
  #endif ECHO_TO_SERIAL


  //Werte in logfile schreiben ------------------------------------------
    file.print(CO2, 0);
    file.close();
    }
    
  }else{
    if(counter == (n_counts + 1)){
    digitalWrite(pin_pumpe,HIGH);
    counter++;
    }
    
// relais 1 off and on times -----------------------------------------------------------
  if(now.hour() % relais_h == 0){
    
    if(file.open(filename, O_WRITE | O_APPEND)){
  
   // time -------------------------------------
    SdFile::dateTimeCallback(dateTime); //Update the timestamp of the logging file
    if( now.minute() >= 1 & now.minute() < (ventil_mins + 1)){
      digitalWrite(pin_ventil,LOW);
    }else{
      digitalWrite(pin_ventil,HIGH);
    }
    // relais 2 off and on time
  if(now.minute() >= (ventil_mins + 1) & now.minute() < (ventil_mins + 1 + pumpe_mins)){
      digitalWrite(pin_pumpe,LOW);
    }else{
      digitalWrite(pin_pumpe,HIGH);
    }
    if(now.minute() <= (ventil_mins + 1)){
      digitalWrite(pin_dyn,LOW);

    //Datum print-------------------------------------------------------------
    file.println("");
    file.print(date_char);
    file.print(";");
    //file.flush();
  
  #if ECHO_TO_SERIAL
    Serial.print(date_char);
    Serial.print(";");
    #endif ECHO_TO_SERIAL


    // read CO2 Anaolog signal-------------------------------------------------
    float CO2 = readCO2();


   //signal an PC console ------------------------------------------
   #if ECHO_TO_SERIAL
    Serial.print(" CO2: ");
    Serial.println(CO2,0);   
  #endif ECHO_TO_SERIAL


  //Werte in logfile schreiben ------------------------------------------
    file.print(CO2, 0);
    file.close();
    }else{
      digitalWrite(pin_dyn,HIGH);
      file.close();
    }//now.minute <= ventil_mins +1
  }//file.open
  }//relais_h
  }//if counter > n_counts
//------------------------------------
  //kammer
  //-------------------------------------
  //dyn_on = 0;
  delay(100);
  if(dyn_on == 1){
    ////////////////////////////////////////////////////////////////////////////////////////  
    //read_CO2_RxTx();
    ////////////////////////////////////////////////////////////////////////////////////////

        //Datei öffnen (ACHTUNG die Datei muss immer wieder geschlossen werden sonst treten Fehler auf!!)
    if(file.open(filename, O_WRITE | O_APPEND)){
      // Messwerte auslesen ------------------------------------------------------------------------

    //read CO2 signal with Serial Communication-----------------------------------------------
    // sending bytes ------------------------------------------- 
    Serial2.write(out_bytes,(sizeof(out_bytes)));
    // receiving bytes -------------------------------------------------
    if(Serial2.available()){
      //so lange Serial2 available werden byte für byte abgerufen
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
  float CO2_chamber = *((float *)CO2_bytes); 

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
    Serial.print(CO2_chamber,0);
    Serial.print(", temp:");
    Serial.println(temp,2);    
  #endif ECHO_TO_SERIAL
  
  //Werte in logfile schreiben ------------------------------------------
    file.println("");
    file.print(date_char);
    file.print(";");

    file.print(CO2_chamber, 0);
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
    Serial.print(date_char);
    Serial.println("; no data signal");   
  #endif ECHO_TO_SERIAL
  }//ende serial2.available
  }//ende file.open
  
   
  }//dyn_on == 1
   ////////////////////////////////////////////////////////////////////////////////////////
    
    ////////////////////////////////////////////////////////////////////////////////////////

  
  }//sd.begin
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
void get_filename_chamber(){
DateTime now = rtc.now();

filename_chamber[0] = (now.year()/10)%10 + '0'; //To get 3rd digit from year()
filename_chamber[1] = now.year()%10 + '0'; //To get 4th digit from year()
filename_chamber[2] = now.month()/10 + '0'; //To get 1st digit from month()
filename_chamber[3] = now.month()%10 + '0'; //To get 2nd digit from month()
filename_chamber[4] = now.day()/10 + '0'; //To get 1st digit from day()
filename_chamber[5] = now.day()%10 + '0'; //To get 2nd digit from day()
//filename[6] = now.hour()/12 + '0'; //To get 1st digit from day()
//filename[7] = now.hour()%10 + '0'; //To get 2nd digit from day()
}
//
//void write_header(char file_name[], char header[]) {
//  if(!sd.exists(file_name)){
//    file.open(file_name, O_WRITE | O_CREAT | O_EXCL | O_APPEND);
//    
//    file.print(header);
//    file.close();
//  }
//}

void write_header() {
  if(!sd.exists(filename)){
    file.open(filename, O_WRITE | O_CREAT | O_EXCL | O_APPEND);
    
    file.print("date; CO2_ppm");
    file.close();
  }
}
void write_header_chamber() {
  if(!sd.exists(filename_chamber)){
    file.open(filename_chamber, O_WRITE | O_CREAT | O_EXCL | O_APPEND);
    
    file.print("date; CO2_ppm; temp_C");
    file.close();
  }
}

float readVout()
{
    long sum = 0;
    for(int i=0; i<32; i++)
    {
        sum += analogRead(pinAdc);
    }
    // >>= bitshift nach rechts entspricht eine division durch 2^x also in diesem fall 2^5 also 32
    sum >>= 5;
    //Analog to Digital Converter (ADC): Analog V measured = ADC reading * System Voltage (5V) / Resolution of ADC (10 bits = 2^10 also 1023)
    float MeasuredVout = sum * (VRefer / 1023.0);
    
  
    return MeasuredVout;
}
 
float readCO2()
{
    // Vout samples are with reference to 3.3V
    float Vout = readVout();

    // Sauerstoffkonz Luft 20.95%
    // Gemessenes Analog Signal 1.325V
    float Concentration = ((Vout - 0.4) / 1.6) * 5000;
    return Concentration;
}

void read_CO2_RxTx() {
    //Datei öffnen (ACHTUNG die Datei muss immer wieder geschlossen werden sonst treten Fehler auf!!)
    if(file.open(filename, O_WRITE | O_APPEND)){
      // Messwerte auslesen ------------------------------------------------------------------------

    //read CO2 signal with Serial Communication-----------------------------------------------
    // sending bytes ------------------------------------------- 
    Serial2.write(out_bytes,(sizeof(out_bytes)));
    // receiving bytes -------------------------------------------------
    if(Serial2.available()){
      //so lange Serial2 available werden byte für byte abgerufen
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
  
   // Messwerte ausgeben --------------------------------------------
   //signal an PC console ------------------------------------------
//   #if ECHO_TO_SERIAL
//    Serial.print(date_char);
//    Serial.print(";");
//    Serial.print("CO2: ");
//    Serial.print(CO2,0);
//    Serial.print(", temp:");
//    Serial.println(temp,2);    
//  #endif ECHO_TO_SERIAL
  
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
//   #if ECHO_TO_SERIAL
//    Serial.print(date_char);
//    Serial.println("; no data signal");   
//  #endif ECHO_TO_SERIAL
  }//ende serial2.available
  }//ende file.open
}
