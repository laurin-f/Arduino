
// Load needed packages -----------------------------------------------------------------------
#include "RTClib.h" //Time
#include "SdFat.h" //SD-card
#include "SPI.h" //needed by SD library
#include <SoftwareSerial.h>

// Create needed variables --------------------------------------------------------------------

// other input variables ------------------------------------------------
int intervall_s = 1;
int intervall_min = 0;
int relais_h = 3; //Pause zwischen inj messungen in stunden
int ventil_mins = 6;//6; //Zeitraum in dem das ventil offen ist und die inj Kammer misst
int pumpe_mins = 1; //how many minutes does the pump pump
int kammer_intervall = 30;//10; //min
int kammer_closing = 7; //min
//int dyn_on = 1; // is dynament sensor turned on or not
int counter_01 = 1; // hilfsvariable um counter zu setzen
int counter = 0;

//initial measurements

//Time
RTC_DS1307 rtc; //Defines the real Time Object

int now_init = 0;
// Pins variables
const float VRefer = 5;       // voltage of adc reference
const int pinA0   = A0;
const int pinA1   = A1;
const int pin_ventil = 2;
const int pin_pumpe = 3;
const int pin_dyn = 4;
const int pin_kammer = 5;
const int pin_dyn_kammer = 8;
// 

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
  digitalWrite(pin_dyn,HIGH);
//SD -------------------------------------------------------
   #if ECHO_TO_SERIAL //if USB connection exists do the following:
   Serial.begin(baudrate); //Activate Serial Monitor
   #endif ECHO_TO_SERIAL
   
   //DateTime now = rtc.now(); //Get the current time
   //now_init = now.minute(); //Get the current time
}

// loop -----------------------------------------------------
void loop(){
//  Serial.print("test");
//  Serial.println(test);
//  test++;
  if(sd.begin(chipSelect, SPI_HALF_SPEED)){

  get_filename();
  get_filename_chamber();
  //write_header();
  //write_header_chamber();
  
  write_header(filename, "date; CO2_ppm; counter");
  write_header(filename_chamber, "date; CO2_ppm; chamber");
  
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

  
    /////kammer----------------------------------------------------

    //------------------------------------
  //kammer
  //-------------------------------------
  //dyn_on = 1;
  //delay(100);
  //if(dyn_on == 1){
    ////////////////////////////////////////////////////////////////////////////////////////  
    //read_CO2_RxTx();
    float CO2_chamber = readCO2(pinA1);
    
    if(file.open(filename_chamber, O_WRITE | O_APPEND)){
      file.write(date_char);
      file.write(";");
      file.write(CO2_chamber);
      file.close();
    }
    ////////////////////////////////////////////////////////////////////////////////////////

  //}//dyn_on == 1
   ////////////////////////////////////////////////////////////////////////////////////////
    
    ////////////////////////////////////////////////////////////////////////////////////////

  if((now.minute() - kammer_closing - 2)  % kammer_intervall == 0){
    //digitalWrite(pin_dyn_kammer, HIGH);
    //dyn_on = 0;
  }
//  
  if((now.minute() + 2)  % kammer_intervall == 0){
    digitalWrite(pin_dyn_kammer, LOW);
    //dyn_on = 1;
  }
  
//  Serial.print("kammer_intervall: ");
//  Serial.print(kammer_intervall);
//  Serial.print(" dyn_on:");
//  Serial.println(dyn_on);
  
  if(now.minute() % kammer_intervall == 0){
    digitalWrite(pin_kammer,LOW);    
    if(file.open(filename_chamber, O_WRITE | O_APPEND)){
      file.print(";1");
      file.close();
    }
    #if ECHO_TO_SERIAL
    Serial.println("closing chamber");
    #endif ECHO_TO_SERIAL
  }else if((now.minute() - kammer_closing) % kammer_intervall == 0){
    digitalWrite(pin_kammer,HIGH);
    if(file.open(filename_chamber, O_WRITE | O_APPEND)){
      file.print(";0");
      file.close();
    }
    #if ECHO_TO_SERIAL
    Serial.println("opening chamber");
    #endif ECHO_TO_SERIAL
  }else{
    if(file.open(filename_chamber, O_WRITE | O_APPEND)){
      if(digitalRead(pin_kammer)){
        file.print(";0");
      }else{
        file.print(";1");
      }
      file.close();
    }
  }

    
// relais 1 off and on times -----------------------------------------------------------
  if(now.hour() % relais_h == 0){
    if(file.open(filename, O_WRITE | O_APPEND)){
  
   // time -------------------------------------

    if( now.minute() >= 1 & now.minute() < (ventil_mins + 1)){
    if(counter_01 == 1){
    counter++;
    counter_01 =0; 
    }
      digitalWrite(pin_ventil,LOW);
    }else{
      counter_01 = 1;
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
    float CO2 = readCO2(pinA0);


   //signal an PC console ------------------------------------------
   #if ECHO_TO_SERIAL
    Serial.print(" CO2: ");
    Serial.println(CO2,0);   
  #endif ECHO_TO_SERIAL


  //Werte in logfile schreiben ------------------------------------------
    file.print(CO2, 0);
    file.print("; ");
    file.print(counter);
    file.close();
    }else{
      digitalWrite(pin_dyn,HIGH);
      file.close();
    }//now.minute <= ventil_mins +1
  }//file.open
  }//relais_h
  //}//if counter > n_counts

  
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
void write_header(char file_name[], char header[]) {
  if(!sd.exists(file_name)){
    file.open(file_name, O_WRITE | O_CREAT | O_EXCL | O_APPEND);
    
    file.print(header);
    file.close();
  }
}



float readVout(int pinAdc)
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
 
float readCO2(int pin)
{
    // Vout samples are with reference to 3.3V
    float Vout = readVout(pin);

    // Sauerstoffkonz Luft 20.95%
    // Gemessenes Analog Signal 1.325V
    float Concentration = ((Vout - 0.4) / 1.6) * 5000;
    return Concentration;
}
