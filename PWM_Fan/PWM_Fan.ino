#include "RTClib.h" //Time

unsigned int baudrate = 38400;  //baudrate
// with RTC
//RTC_DS1307 rtc; //Defines the real Time Object
//without RTC
RTC_Millis rtc; //Defines the real Time Object


const int PWMpin1 = 3;

const int relaispin1 = 4;
const int relaispin2 = 5;

float Amp = 255;
float T = 120;//s
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
    Serial.print("Time: ");
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
  pinMode(relaispin1, OUTPUT);
  //pinMode(relaispin2, OUTPUT);

  digitalWrite(relaispin1,LOW);
  //digitalWrite(relaispin2,LOW);
  
  Serial.begin(baudrate);
}

void loop() {
  // speed must be a number between 0 and 255
  pwmsinus(PWMpin1,relaispin1,T,0);
  //pwmfix(PWMpin1);

  delay(1000);
}
