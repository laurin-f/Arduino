#include "RTClib.h" //Time

unsigned int baudrate = 38400;  //baudrate
// with RTC
//RTC_DS1307 rtc; //Defines the real Time Object
//without RTC
RTC_Millis rtc; //Defines the real Time Object

////////////////////////////////////////////////////////////////////
// hier den Offset und Amplitude und Periodendauer T einstellen
float Amp_rel = 60;//100%
float Amp_change_rel = 20; // 20% weniger pro Stufe
int step_hours = 6; // Anzahl Stunden pro Amp Stufe ACHTUNG mindestens 2 h sonst funktioniert der Code nicht
int n_steps = 3; //Anzahl Stufen 
int break_hours = 6; //Pause zwischen Versuchen
int init_hours = 0; //Stunden bis zum Start des Programms
int WS_soil = 0;
float T = 60;//s
float offset_2 = 10;//s
float offset_3 = 20;//s
float offset_4 = 30;//s
float amp_offset1 = 1;
float amp_offset2 = 1;
float amp_offset3 = 1;
float amp_offset4 = 1;
/////////////////////////////////////////////////////////////////////
//umrechnung

float Amp = Amp_rel/100 * 255;
float Amp_change = Amp_change_rel/100 * 255;

//andere Variablen
int marker = 1;
int counter = 0;
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
  
}

void loop() {
}
