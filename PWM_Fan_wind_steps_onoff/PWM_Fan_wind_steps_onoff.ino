#include "RTClib.h" //Time

unsigned int baudrate = 38400;  //baudrate
// with RTC
//RTC_DS1307 rtc; //Defines the real Time Object
//without RTC
RTC_Millis rtc; //Defines the real Time Object

////////////////////////////////////////////////////////////////////
// hier den Offset und Amplitude und Periodendauer T einstellen
float Amp_rel = 100;//100%
float Amp_change_rel = -34; // 20% weniger pro Stufe
int n_steps = 5;
int counter = 0;
int step_hours = 1; // Anzahl Stunden pro Amp Stufe
int WS_soil = 0;
float T = 60;//s
float offset_2 = 10;//s
float offset_3 = 20;//s
float offset_4 = 30;//s
float amp_offset1 = 1;
float amp_offset2 = 1;
float amp_offset3 = 1;
float amp_offset4 = 1;

int n_versuche = 3;
int break_hours = 6; //Pause zwischen Versuchen
int init_hours = 18; //Stunden bis zum Start des Programms
int versuch_counter = 1;
/////////////////////////////////////////////////////////////////////
//umrechnung

float Amp = round(Amp_rel/100 * 255);
float Amp_change = round(Amp_change_rel/100 * 255);

//andere Variablen
int marker = 1;
int start_day = 0;
int start_hour = 0;
const unsigned long SECOND = 1000;
const unsigned long HOUR = 3600*SECOND;

//Pin Konstanten
const int PWMpin1 = 3;
const int PWMpin2 = 5;
const int PWMpin3 = 6;
const int PWMpin4 = 9;
const int PWMpin5 = 10;

const int relaispin1 = 2;
const int relaispin2 = 4;
const int relaispin3 = 8;
const int relaispin4 = 7;
const int mainpower_relais = 11;



float speed;

void pwmfix(int PIN,int speed = 255){

    analogWrite(PIN, speed);
    Serial.print("PIN: ");
    Serial.print(PIN);
    Serial.print(" speed: ");
    Serial.println(speed);
}

void setup() { 
  // put your setup code here, to run once:
  pinMode(PWMpin1, OUTPUT);
  pinMode(PWMpin2, OUTPUT);
  pinMode(PWMpin3, OUTPUT);
  pinMode(PWMpin4, OUTPUT);
  pinMode(PWMpin5, OUTPUT);

  pinMode(relaispin1, OUTPUT);
  pinMode(relaispin2, OUTPUT);
  pinMode(relaispin3, OUTPUT);
  pinMode(relaispin4, OUTPUT);
  pinMode(mainpower_relais, OUTPUT);

  digitalWrite(relaispin1,HIGH);
  digitalWrite(relaispin2,HIGH);
  digitalWrite(relaispin3,HIGH);
  digitalWrite(relaispin4,HIGH);
  digitalWrite(mainpower_relais,HIGH);


  Serial.begin(baudrate);
  // PWM Signal für Ventilatoren am Boden
  pwmfix(PWMpin5,WS_soil);

  //warten bis gestartet wird falls init_hours > 0
  delay(init_hours * HOUR);
  
  DateTime now = rtc.now();
  start_hour = now.hour();
  start_day = now.day();
}

void loop() {
  if(versuch_counter <= n_versuche){
  DateTime now = rtc.now();
  if(!(now.day() == start_day & now.hour() == start_hour)){
    if((now.hour() - start_hour) % step_hours == 0 & marker == 1){
      Amp = Amp + Amp_change;
      marker = 0;
      if(Amp > 255){
       Amp = 255;
      }
      if(Amp < 0){
       Amp = 0;
      }
      counter++;
    }
  }
  if((now.hour() - start_hour) % step_hours != 0 & marker == 0){
    marker = 1;
  }


  // speed must be a number between 0 and 255
  pwmfix(PWMpin5,abs(Amp));
  
  //pwmfix(PWMpin1);

  delay(1000);
  if(counter >= n_steps){
    counter = 0;
    versuch_counter++;
    Amp = Amp_rel/100 * 255;
    delay(break_hours * HOUR);
    DateTime now = rtc.now();
    start_hour = now.hour();
    start_day = now.day();
  }
}else{
  pwmfix(PWMpin5,WS_soil);
  delay(break_hours * HOUR);
}
}
