
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
  digitalWrite(pin_pumpe,HIGH);
  digitalWrite(pin_dyn,LOW);
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

  Serial.println();
  //Serial.println(date_char);
  Serial.print("meas: ");
  Serial.println(meas);
  Serial.print("counter: ");
  Serial.print(counter);
//initial measurements
// am anfang wird erstmal t_init gewartet damit der Schlauch sich mit CO2 fuellt
  if(counter == 0){
    long pause = t_init * 60L*1000L;
    Serial.print("pause: ");
    Serial.println(t_init);
    delay(pause);
    counter++;
  }else if(counter <= n_counts | (counter == (n_counts +1) & now.minute() % ventil_mins == 0)){

    //dann werden n_counts messungen gemacht bevor nur noch alle relais_h stunden eine messung gemacht wird
    //wenn counter n_counts +1 ist dann wird noch ein letztes mal leergepumpt
    
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
    Serial.println("");
    Serial.print(date_char);
    Serial.print(";");
    #endif ECHO_TO_SERIAL

    
    // read CO2 Anaolog signal---------------------------------------------------
    float CO2 = readCO2();


   //signal an PC console ------------------------------------------
   #if ECHO_TO_SERIAL
    Serial.print(" CO2: ");
    Serial.print(CO2,0);   
  #endif ECHO_TO_SERIAL


  //Werte in logfile schreiben ------------------------------------------
    file.print(CO2, 0);
    file.print(";");
    file.print(counter);

    
    //relais 1 off and on times ---------------------------------------------
    // ventil geht in die Inj Kamer 
    //now.minute % ventil_mins != 0 heisst immer wenn die jetzige minute nicht ohne rest durch ventil_mins teilbar ist
    if(now.minute() % ventil_mins != 0){
      digitalWrite(pin_ventil,LOW);
      file.print(";1");
      meas = 1;
    }else{
      digitalWrite(pin_ventil,HIGH);
      file.print(";0");
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

    if(now.minute() <= (ventil_mins + 1)){
      digitalWrite(pin_dyn,LOW);

    //Datum print-------------------------------------------------------------
    file.println("");
    file.print(date_char);
    file.print(";");
    //file.flush();
  
  #if ECHO_TO_SERIAL
    Serial.println("");
    Serial.print(date_char);
    Serial.print(";");
    #endif ECHO_TO_SERIAL


    // read CO2 Anaolog signal-------------------------------------------------
    float CO2 = readCO2();


   //signal an PC console ------------------------------------------
   #if ECHO_TO_SERIAL
    Serial.print(" CO2: ");
    Serial.print(CO2,0);   
  #endif ECHO_TO_SERIAL


  //Werte in logfile schreiben ------------------------------------------
    file.print(CO2, 0);
    file.print(";");
    file.print(counter);

    }else{
      digitalWrite(pin_dyn,HIGH);
      file.close();
    }//now.minute <= ventil_mins +1

    //ventil off and on time
    if( now.minute() >= 1 & now.minute() < (ventil_mins + 1)){
      digitalWrite(pin_ventil,LOW);
      file.print(";1");
    }else{
      digitalWrite(pin_ventil,HIGH);
      file.print(";0");
    }
    // relais 2 off and on time
  if(now.minute() >= (ventil_mins + 1) & now.minute() < (ventil_mins + 1 + pumpe_mins)){
      digitalWrite(pin_pumpe,LOW);
    }else{
      digitalWrite(pin_pumpe,HIGH);
    }
        file.close();
  }//file.open
  }//relais_h
  }//if counter > n_counts
  }else{//sd.begin
  digitalWrite(pin_ventil,LOW);
  delay(1000);
  digitalWrite(pin_ventil,HIGH);
  delay(4000);
    Serial.print("No SD found");
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
    
    file.print("date; CO2_ppm; counter; inj");
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
