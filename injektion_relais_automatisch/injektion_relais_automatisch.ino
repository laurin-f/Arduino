
// Load needed packages -----------------------------------------------------------------------
#include "RTClib.h" //Time
#include "SdFat.h" //SD-card
#include "SPI.h" //needed by SD library


// Create needed variables --------------------------------------------------------------------


//Time
RTC_DS1307 rtc; //Defines the real Time Object

//SD variables
SdFat sd;

// O2 variables
const float VRefer = 5;       // voltage of adc reference
const int pinAdc   = A0;
const int pin_ventil = 2;
const int pin_pumpe = 3;
const int pin_dyn = 4;
 
const int chipSelect = 10; //Select the pin the SD card uses for communication
  //if Pin 10 is used for something else the SD library will not work
SdFile file; //Variable for the logging of data
char filename[] = "yymmdd.TXT";
char date_char[] = "yy/mm/dd HH:MM:SS";

//Variable for USB connection
#define ECHO_TO_SERIAL 1 //check if Arduino is connected via USB (aka to a PC)
  //if False the lines regarding the Serial Monitor are not executed


// other input variables ------------------------------------------------
int intervall_s = 1;
int relais_h = 6;
int intervall_min = 0;
unsigned int baudrate = 38400;
long min_break = 400L;





// Setup ----------------------------------------------------------------------
void setup(){
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
  pinMode(pin_ventil, OUTPUT);
  pinMode(pin_pumpe, OUTPUT);
  pinMode(pin_dyn, OUTPUT);

  digitalWrite(pin_ventil,HIGH);
  digitalWrite(pin_pumpe,HIGH);
  digitalWrite(pin_dyn,HIGH);
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

// relais 1 off and on times
  if(now.hour() % relais_h == 0){
    
    if(file.open(filename, O_WRITE | O_APPEND)){
  
   // time -------------------------------------
    SdFile::dateTimeCallback(dateTime); //Update the timestamp of the logging file
    if( now.minute() >= 1 & now.minute() < 7){
      digitalWrite(pin_ventil,LOW);
    }else{
      digitalWrite(pin_ventil,HIGH);
    }
    // relais 2 off and on time
  if(now.minute() >= 7 & now.minute() < 8){
      digitalWrite(pin_pumpe,LOW);
    }else{
      digitalWrite(pin_pumpe,HIGH);
    }
    if(now.minute() <= 7){
      digitalWrite(pin_dyn,LOW);
    
    file.println("");
    file.print(date_char);
    file.print(";");
    //file.flush();
  
  #if ECHO_TO_SERIAL
    Serial.println("");
    Serial.print(date_char);
    Serial.print(";");
    #endif ECHO_TO_SERIAL


    // read CO2 Anaolog signal
    float CO2 = readCO2();


   //signal an PC console ------------------------------------------
   #if ECHO_TO_SERIAL
    Serial.print(" CO2: ");
    Serial.print(CO2,0);   
  #endif ECHO_TO_SERIAL


  //Werte in logfile schreiben ------------------------------------------
    file.print(CO2, 0);
    file.close();
    }else{
      digitalWrite(pin_dyn,HIGH);
      file.close();
    }
  }
  }
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
    file.open(filename, O_WRITE | O_CREAT | O_EXCL | O_APPEND);
    
    file.print("date; CO2_ppm");
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
