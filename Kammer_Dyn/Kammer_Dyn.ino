
// Load needed packages -----------------------------------------------------------------------
#include "RTClib.h" //Time
#include "SdFat.h" //SD-card
#include "SPI.h" //needed by SD library
#include <SoftwareSerial.h>


// Create needed variables --------------------------------------------------------------------

// other input variables ------------------------------------------------
int intervall_s = 1;
int intervall_min = 0;
int kammer_intervall = 2; //min
int kammer_closing = 1; //min

//Time
RTC_DS1307 rtc; //Defines the real Time Object

// Pins variables
const int pin_ventil = 2;
const int pin_pumpe = 3;
const int pin_dyn = 4;
const int pin_kammer = 5;
const int pin_dyn_kammer = 8;

const int rx = 6;
const int tx = 7;
SoftwareSerial Serial2(rx, tx); //rx tx

 //SD variables----------------------------------------------------
SdFat sd;
const int chipSelect = 10; //Select the pin the SD card uses for communication
  //if Pin 10 is used for something else the SD library will not work
SdFile file; //Variable for the logging of data
char filename[] = "yymmdd.TXT";
char date_char[] = "yy/mm/dd HH:MM:SS";

//Variable for USB connection
#define ECHO_TO_SERIAL 1 //check if Arduino is connected via USB (aka to a PC)
  //if False the lines regarding the Serial Monitor are not executed
unsigned int baudrate = 38400;



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
  pinMode(pin_ventil, OUTPUT);
  pinMode(pin_pumpe, OUTPUT);
  pinMode(pin_dyn, OUTPUT);
  pinMode(pin_kammer, OUTPUT);
  pinMode(pin_dyn_kammer, OUTPUT);

  digitalWrite(pin_ventil,HIGH);
  digitalWrite(pin_pumpe,HIGH);
  digitalWrite(pin_dyn,LOW);
  digitalWrite(pin_kammer,HIGH);
  digitalWrite(pin_dyn_kammer,LOW);
//SD -------------------------------------------------------
   #if ECHO_TO_SERIAL //if USB connection exists do the following:
   Serial.begin(baudrate); //Activate Serial Monitor
   #endif ECHO_TO_SERIAL

   Serial2.begin(baudrate);
   Serial2.flush();
   
}

// loop -----------------------------------------------------
void loop(){
  //if(sd.begin(chipSelect, SPI_HALF_SPEED)){

  //get_filename();
  //write_header();
  
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



  //------------------------------------
  //kammer


//  if((now.minute() + 2)  % kammer_intervall == 0){
//    digitalWrite(pin_dyn_kammer, LOW);
//  }
//  
//  if((now.minute() - kammer_closing - 2)  % kammer_intervall == 0){
//    digitalWrite(pin_dyn_kammer, HIGH);
//  }
  Serial.println(date_char);
  Serial.println(now.minute() - kammer_closing);

  if(now.minute() % kammer_intervall == 0){
      digitalWrite(pin_ventil,LOW);
  digitalWrite(pin_pumpe,LOW);
  digitalWrite(pin_dyn,HIGH);
  digitalWrite(pin_kammer,LOW);
  digitalWrite(pin_dyn_kammer,HIGH);
    
    Serial.println("closing chamber");
  }
  if((now.minute()-kammer_closing) % kammer_intervall == 0){
      digitalWrite(pin_ventil,HIGH);
  digitalWrite(pin_pumpe,HIGH);
  digitalWrite(pin_dyn,LOW);
  digitalWrite(pin_kammer,HIGH);
  digitalWrite(pin_dyn_kammer,LOW);
    
    Serial.println("opening chamber");
  }
 // }
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
    file.open(filename, O_WRITE | O_CREAT | O_EXCL | O_APPEND);
    
    file.print("date; CO2_ppm");
    file.close();
  }
}
  


  
