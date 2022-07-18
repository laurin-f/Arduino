
// Load needed packages -----------------------------------------------------------------------
#include "RTClib.h" //Time
#include "SdFat.h" //SD-card
#include "SPI.h" //needed by SD library


// Create needed variables --------------------------------------------------------------------

// other input variables ------------------------------------------------
int intervall_s = 1;
int intervall_min = 0;
int relais_h = 6; //Pause zwischen inj messungen in stunden
int ventil_mins = 6; //Zeitraum in dem das ventil offen ist und die inj Kammer misst
int pumpe_mins = 1; //how many minutes does the pump pump

//initial measurements
int counter = 0;//counts the initial measurements
int meas = 0;//changes between 0 = Pump and 1 =  Measurement 
int n_counts = 5;//number of initial measurements
int t_init = 0;//minutes before first measurement

//Time
RTC_DS1307 rtc; //Defines the real Time Object

// Pins variables
const float VRefer = 5;       // voltage of adc reference
const int pinAdc   = A0;
const int pin_ventil = 2;
const int pin_pumpe = 3;
const int pin_dyn = 4;

 //SD variables----------------------------------------------------
SdFat sd;
const int chipSelect = 10; //Select the pin the SD card uses for communication
  //if Pin 10 is used for something else the SD library will not work
SdFile file; //Variable for the logging of data
char filename[] = "yymmdd_inj.TXT";
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

  digitalWrite(pin_ventil,HIGH);
  digitalWrite(pin_pumpe,LOW);
  digitalWrite(pin_dyn,HIGH);
//SD -------------------------------------------------------
   #if ECHO_TO_SERIAL //if USB connection exists do the following:
   Serial.begin(baudrate); //Activate Serial Monitor
   #endif ECHO_TO_SERIAL
}

// loop -----------------------------------------------------
void loop(){
  delay(60000);
  digitalWrite(pin_pumpe,HIGH);
}
