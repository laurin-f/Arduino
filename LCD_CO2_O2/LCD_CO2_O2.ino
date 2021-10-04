//-------------------------------------------------------------------------------------------//
//Script um CO2 und 02 Messungen auf der SD Karte zu loggen und auf einem Display anzuzeigen //    
// auf dem Display wird aufgrund der begrenzten Zeichenzahl (16X02) O2 als O abgekürtzt      //
//                                                                                           //
//                        scripted by Laurin Osterholt                                       // 
//-------------------------------------------------------------------------------------------//



// Load needed packages -----------------------------------------------------------------------
#include <SoftwareSerial.h>
#include "RTClib.h" //Time
#include "SdFat.h" //SD-card
#include "SPI.h" //needed by SD library
#include <LiquidCrystal.h>
//#include <Wire.h>


// Create needed variables --------------------------------------------------------------------

// Allgemeine variablen ------------------------------------------------
int intervall_s = 1;      //Messintervall in s (wenn 0 dann wird intervall_min verwendet)
int intervall_min = 0;    //Messintervall in min (wenn 0 dann wird intervall_s verwendet)

unsigned int baudrate = 38400;  //baudrate
//Variable for USB connection
#define ECHO_TO_SERIAL 1 //check if Arduino is connected via USB (aka to a PC)
  //if False the lines regarding the Serial Monitor are not executed
//Time
RTC_DS1307 rtc; //Defines the real Time Object

// LCD pins----------------------------------------------------------------
const int rs = 2, en = 3, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


//SD variables--------------------------------------------------------------
SdFat sd;
const int chipSelect = 10; //Select the pin the SD card uses for communication
  //if Pin 10 is used for something else the SD library will not work
SdFile file; //Variable for the logging of data
//dateiname auf der SD Karte ist das aktuelle Datum
// vorerst wird ein character string angelegt
char filename[] = "yymmdd.TXT";

// character strings in die später das formatierete Datum geschrieben wird 
char date_char[] = "yy/mm/dd HH:MM:SS";
//Auf dem Display wird nur die Uhrzeit angegeben 
char lcd_date[] = "HH:MM:SS";


// O2 variables---------------------------------------------------------------------
const float VRefer = 5;       // voltage of adc (Analog digital converter) reference
const int pinO2   = A0;


// CO2 variables--------------------------------------------------------------------------
// Control Bytes for Serial Communication with Dynament CO2 Sensor
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

//bytes in die CO2 und temp messwerte geschrieben werden
byte CO2_bytes[4]; 
byte temp_bytes[4]; 


//////////////////////////////////////////////////////////////////////////////
// Setup ----------------------------------------------------------------------
void setup(){
// establish serial communication -------------------------------------
   lcd.begin(16, 2);
   Serial2.begin(baudrate);
   Serial2.flush();
   
 // start RTC -----------------------------------
   if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  //wenn die Uhr nicht bereits läuft
  if (! rtc.isrunning()) {
    //wird sie hier gestellt
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }
  
//output pins
  pinMode(chipSelect, OUTPUT);  //Reserve pin 10 (chip select) as an output, dont use it for other parts of circuit
  
   #if ECHO_TO_SERIAL //if USB connection exists do the following:
   Serial.begin(baudrate); //Activate Serial Monitor
   #endif ECHO_TO_SERIAL
}

// loop -----------------------------------------------------
void loop(){
  //SD Karte initialisieren
  if(sd.begin(chipSelect, SPI_HALF_SPEED)){
 // Funktion um aktuelles Datum in Dateiname zu schreiben
  get_filename();
  // Funktion um Kopfzeile in die Datei zu schreiben falls diese noch nicht existiert
  write_header();
  
  //Datei öffnen (ACHTUNG die Datei muss immer wieder geschlossen werden sonst treten Fehler auf!!)
  if(file.open(filename, O_WRITE | O_APPEND)){
  
   // time -------------------------------------
    SdFile::dateTimeCallback(dateTime); //Update the timestamp of the logging file


  
 
    // Warten -------------------------------------------------------------
    //wenn minuten eingestellt sind
    if(intervall_min > 0){
      //Aktuelle Zeit 
      DateTime now1 = rtc.now(); //Get the current time
      //Pause ist das intervall in sekunden minus die aktuelle sekunde 
      //um immer bei sekunde 00 einer Minute zu starten
      long pause = 1000L*60L*(intervall_min) - now1.second()*1000L;
      delay(pause);
    }
    //wenn intervall in s ist werden einfach die angegeben s gewartet
    if(intervall_s > 0){
      long pause = 1000L*intervall_s;
      delay(pause);
    }

    // Datum und Uhrzeit -----------------------------------------------------
    //Uhrzeit formatieren
    DateTime now = rtc.now(); //Get the current time
    sprintf(date_char,"%02d/%02d/%02d %02d:%02d:%02d", now.year() % 100, now.month(), now.day(),  now.hour(), now.minute(), now.second());
    //Uhrzeit in Datei schreiben
    file.println("");
    file.print(date_char);
    file.print(";");

    //wenn USB dann auch hier die Uhrzeit
    #if ECHO_TO_SERIAL
    Serial.println("");
    Serial.print(date_char);
    Serial.print(";");
    #endif ECHO_TO_SERIAL

    //Uhrzeit auf dem Display anzeigen
    //sprintf(lcd_date," %02d.%02d %02d:%02d:%02d ", now.day(), now.month(),  now.hour(), now.minute(), now.second());
    sprintf(lcd_date,"%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    lcd.setCursor(0,0);
    lcd.print(lcd_date); 

    // Messwerte auslesen ------------------------------------------------------------------------
    // read O2 Anaolog signal
    float O2 = readConcentration();

    //read CO2 signal with Serial Communication-----------------------------------------------
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

   // Messwerte ausgeben --------------------------------------------
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

    // Werte auf LCD ausgeben -------------------------------------------
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
      
  // wenn keine CO2 Messwerte vorhanden sind (kein serial2 available)-----------------
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
  }//ende serial2.available
  }//ende file.open
  }else{ //wenn SD initialisierung fehlgeschlagen ist-------------------------------------
    // Auf LCD ausgeben
    lcd.setCursor(0,0);
    lcd.print(" *** no SD ***  ");
    lcd.setCursor(0,1);
    lcd.print(" ** available **");
  }
}//ende Loop


//////////////////////////////////////////////////////////////////////////////////////////////////////
//functions---------------------------------------------------------------------------------------------


// Function to set the timestamp of the DataFile that was created on the SD card -------------
void dateTime(uint16_t* date, uint16_t* time) {
  DateTime now = rtc.now();

  //return date using FAT_DATE macro to format fields
  *date = FAT_DATE(now.year(), now.month(), now.day());

  //return time using FAT_TIME macro to format fields
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}


//Function um das aktuelle Datum in filename zu schreiben
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

//Function um die Kopfzeile der Logdatei zu schreiben falls diese noch nicht existiert
void write_header() {
  //test ob die datei existiert
  if(!sd.exists(filename)){
    //wenn nicht wird hier die Datei erstellt
    // O_WRITE = Open File for Writing; O_CREATE = create File id not exists; O_EXCL = if file exists opening fails
    file.open(filename, O_WRITE | O_CREAT | O_EXCL);
    //Header schreiben
    file.print("date; CO2_ppm; O2_vol.%; temp_C");
    //datei schließen
    file.close();
  }
}

// Function um Spannung am O2 Pin auszulesen und in eine Konzentration umzurechnen
float readConcentration(){
  //um die Prezision zu erhöhen wird der Wert 32 Mal gelesen und dann durch 32 geteilt
    long sum = 0;
    int n = 3;
    int samples = pow(4,n);
    
    for(int i=0; i<samples; i++)
    {
        //das ausgelesene Signal zu sum addieren
        sum += analogRead(pinO2);
    }
    // >>= bitshift nach rechts entspricht eine division durch 2^x also in diesem fall 2^5 also 32
    sum >>= n;
    //Analog to Digital Converter (ADC): Analog V measured = ADC reading * System Voltage (5V) / Resolution of ADC (10 bits = 2^10 also 1023)
    //float MeasuredVout = sum * (VRefer / 1023);
    float bits = 2^(10 + n)-1;
    float MeasuredVout = sum * (VRefer /  bits);
    
    // Sauerstoffkonz Luft 20.95%
    // Gemessenes Analog Signal 1.325V
    float Concentration = MeasuredVout / 1.325 * 20.95 ;
    return Concentration;
}
