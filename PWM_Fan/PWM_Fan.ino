#include "RTClib.h" //Time

unsigned int baudrate = 38400;  //baudrate
// with RTC
//RTC_DS1307 rtc; //Defines the real Time Object
//without RTC
RTC_Millis rtc; //Defines the real Time Object

////////////////////////////////////////////////////////////////////
// hier den Offset und Amplitude und Periodendauer T einstellen
float Amp = 255;
float T = 60;//s
float offset_2 = 10;//s
float offset_3 = 20;//s
float offset_4 = 30;//s
/////////////////////////////////////////////////////////////////////

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

void pwmsinus(int PIN,int relais,float period, float offset = 0){
    DateTime now = rtc.now();
    float sec = now.second();
    float m = now.minute();
    float min_sec = m*60 + sec;
    speed = Amp * sin((min_sec-offset)/period*2*PI) ;


    if(speed <= 0){
      digitalWrite(relais,HIGH);
    }else{
      digitalWrite(relais,LOW);
    }

    float rel_time = min_sec/period*2 - floor(min_sec/period*2);
    if(rel_time <= 0.5) speed = Amp;
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
  pwmfix(PWMpin5,0);
}

void loop() {
  // speed must be a number between 0 and 255
  pwmsinus(PWMpin1,relaispin1,T,0);
  pwmsinus(PWMpin2,relaispin2,T,offset_2);
  pwmsinus(PWMpin3,relaispin3,T,offset_3);
  pwmsinus(PWMpin4,relaispin4,T,offset_4);
  
  //pwmfix(PWMpin1);

  delay(1000);
}
