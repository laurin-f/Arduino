







// Load needed packages -----------------------------------------------------------------------
#include <SoftwareSerial.h>
#include "RTClib.h" //Time
#include "SdFat.h" //SD-card
#include "SPI.h" //needed by SD library
#include <LiquidCrystal.h>
//#include <Wire.h>


// Create needed variables --------------------------------------------------------------------

// LCD pins
const int rs = 2, en = 3, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//Time
RTC_DS1307 rtc; //Defines the real Time Object

//SD variables
SdFat sd;

// O2 variables
const float VRefer = 5;       // voltage of adc reference
const int pinAdc   = A0;


 
const int chipSelect = 10; //Select the pin the SD card uses for communication
  //if Pin 10 is used for something else the SD library will not work
SdFile file; //Variable for the logging of data
char filename[] = "yymmdd.TXT";
char date_char[] = "yy/mm/dd HH:MM:SS";
char lcd_date[] = "dd.mm HH:MM:SS";

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
int intervall_s = 1;
int intervall_min = 0;
unsigned int baudrate = 38400;
long min_break = 400L;

//pins -----------------------------------
//pins used for Rx and Tx
int Rx = 8;
int Tx = 9;
SoftwareSerial Serial2(Rx, Tx); //rx tx



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
  lcd.begin(16, 2);
      Serial2.begin(baudrate);
      Serial2.flush();
 // datetime -----------------------------------

   if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (! rtc.isrunning()) {
//    Serial.println("RTC is NOT running!");
//    Uhrzeit einmalig adjusten dann auskommentieren
//    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//    Serial.println("RTC adjusted!");
  }
  
//output pins
  //pinMode(chipSelect, OUTPUT); //Reserve pin 10 (chip select) as an output, dont use it for other parts of circuit
  pinMode(chipSelect, OUTPUT);

//SD -------------------------------------------------------
   #if ECHO_TO_SERIAL //if USB connection exists do the following:
   Serial.begin(baudrate); //Activate Serial Monitor
   #endif ECHO_TO_SERIAL
}

// loop -----------------------------------------------------
void loop(){
  if(sd.begin(chipSelect, SPI_HALF_SPEED)){

  get_filename();
  
  write_header();
  
  
  if(file.open(filename, O_WRITE | O_APPEND)){
  
   // time -------------------------------------
  //SdFile::dateTimeCallback(dateTime); //Update the timestamp of the logging file

    DateTime now1 = rtc.now(); //Get the current time
  
  // print Datetime 
    // Warten
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
    
    file.println("");
    file.print(date_char);
    file.print(";");
    //file.flush();
  
  #if ECHO_TO_SERIAL
    Serial.println("");
    Serial.print(date_char);
    Serial.print(";");
    #endif ECHO_TO_SERIAL

    //sprintf(lcd_date," %02d.%02d %02d:%02d:%02d ", now.day(), now.month(),  now.hour(), now.minute(), now.second());
    sprintf(lcd_date,"%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    lcd.setCursor(0,0);
    lcd.print(lcd_date); 

    // read O2 Anaolog signal
    float O2 = readConcentration();
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


   //signal an PC console ------------------------------------------
   #if ECHO_TO_SERIAL
    Serial.print("CO2: ");
    Serial.print(CO2,0);
    Serial.print("O2: ");
    Serial.print(O2,2); 
    Serial.print(", temp:");
    Serial.print(temp,2);
    Serial.print(", ");     
  #endif ECHO_TO_SERIAL

    lcd.setCursor(8,0);
    lcd.print(" O:");
    lcd.print(O2,2); 
    
    lcd.setCursor(0,1);
    lcd.print("CO2:");
    if(CO2 < 1000){
      lcd.print(" ");
    }
    lcd.print(CO2,0);
    lcd.print(" T:");
    lcd.print(temp,2);    

  //Werte in logfile schreiben ------------------------------------------
    file.print(CO2, 0);
    file.print(";");
    file.print(O2, 2);
    file.print(";");
    file.print(temp, 2);

      file.close();
  // wenn kein serial2 available
  }else{
      file.print("NA;");
      file.print(O2, 2);
      file.print(";NA");
      file.close();

    lcd.setCursor(8,0);
    lcd.print(" O:");
    lcd.print(O2,2); 
    lcd.setCursor(0,1);
    lcd.print("*no Data signal*");
  }
  }
  }else{
    lcd.setCursor(0,0);
    lcd.print(" *** no SD ***  ");
    lcd.setCursor(0,1);
    lcd.print(" ** available **");
  }
}



//functions---------------------------------------------------------------------------------------------
//void storeValues () {
//
//}

// Function to set the timestamp of the DataFile that was created on the SD card -------------
//void dateTime(uint16_t* date, uint16_t* time) {
//  DateTime now = rtc.now();
//
//  //return date using FAT_DATE macro to format fields
//  *date = FAT_DATE(now.year(), now.month(), now.day());
//
//  //return time using FAT_TIME macro to format fields
//  *time = FAT_TIME(now.hour(), now.minute(), now.second());
//}


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
    file.open(filename, O_WRITE | O_CREAT | O_EXCL | O_APPEND);
    
    file.print("date; CO2_ppm; O2_vol.%; temp_C");
    file.close();
  }
}

float readO2Vout()
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
 
float readConcentration()
{
    // Vout samples are with reference to 3.3V
    float MeasuredVout = readO2Vout();

    // Sauerstoffkonz Luft 20.95%
    // Gemessenes Analog Signal 1.325V
    float Concentration = MeasuredVout / 1.325 * 20.95 ;
    return Concentration;
}
