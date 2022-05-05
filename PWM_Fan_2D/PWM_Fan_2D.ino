#include "RTClib.h" //Time

unsigned int baudrate = 38400;  //baudrate
// with RTC
//RTC_DS1307 rtc; //Defines the real Time Object
//without RTC
RTC_Millis rtc; //Defines the real Time Object

////////////////////////////////////////////////////////////////////
// hier den Offset und Amplitude und Periodendauer T einstellen
float Amp_rel = 100;//100%
float Amp_change_rel = -20; // 20% weniger pro Stufe
int step_hours = 3; // Anzahl Stunden pro Amp Stufe
int WS_soil = 0;
float T = 60;//s
float offset_2 = 10;//s
float offset_3 = 20;//s
float offset_4 = 30;//s
float amp_offset1 = -0.5;
float amp_offset2 = -0.5;
float amp_offset3 = -0.5;
float amp_offset4 = -0.5;
/////////////////////////////////////////////////////////////////////
//umrechnung
float Amp = Amp_rel/100 * 255;
float Amp_change = Amp_change_rel/100 * 255;

//andere Variablen
int marker = 1;
int start_day = 0;
int start_hour = 0;

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



float speed;

void pwmsinus(int PIN,int relais,float period, float offset = 0, float amp_offset = 1){
    DateTime now = rtc.now();
    float sec = now.second();
    float m = now.minute();
    float min_sec = m*60 + sec;
    speed = Amp * sin((min_sec-offset)/period*2*PI) ;
    float rel_time_2 = (min_sec-offset)/period*2 - floor((min_sec-offset)/period*2);
    float rel_time = (min_sec-offset)/period - floor((min_sec-offset)/period);
    if(rel_time_2 <= 0.5){
      if(rel_time < 0.5){
       speed = Amp;
      }else{
       speed = -Amp; 
      }
    }

    if(speed <= 0){
      if(amp_offset < 0){
        speed = speed*abs(amp_offset);
      }
      digitalWrite(relais,HIGH);
    }else{
      if(amp_offset > 0){
        speed = speed*abs(amp_offset);
      }
      digitalWrite(relais,LOW);
    }



    speed = abs(speed);
    analogWrite(PIN, speed);
    Serial.print("PIN: ");
    Serial.print(PIN);
    Serial.print(" Time: ");
    Serial.print(m,0);
    Serial.print(":");
    Serial.print(sec,0);
    Serial.print(" speed: ");
    Serial.println(speed);
    
}
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

  digitalWrite(relaispin1,HIGH);
  digitalWrite(relaispin2,HIGH);
  digitalWrite(relaispin3,HIGH);
  digitalWrite(relaispin4,HIGH);

  Serial.begin(baudrate);
  // PWM Signal für Ventilatoren am Boden
  pwmfix(PWMpin5,WS_soil);
  DateTime now = rtc.now();
  start_hour = now.hour();
  start_day = now.day();
}

void loop() {
  DateTime now = rtc.now();
  if(!(now.day() == start_day & now.hour() == start_hour)){
    if((now.hour() - start_hour) % step_hours == 0 & marker == 1){
      Amp = Amp + Amp_change;
      marker = 0;
    }
  }
  if((now.hour() - start_hour) % step_hours != 0 & marker == 0){
    marker = 1;
  }
  
  // speed must be a number between 0 and 255
  pwmsinus(PWMpin1,relaispin1,T,0,amp_offset1);
  pwmsinus(PWMpin2,relaispin2,T,offset_2,amp_offset2);
  pwmsinus(PWMpin3,relaispin3,T,offset_3,amp_offset3);
  pwmsinus(PWMpin4,relaispin4,T,offset_4,amp_offset4);
  
  //pwmfix(PWMpin1);

  delay(1000);
}
