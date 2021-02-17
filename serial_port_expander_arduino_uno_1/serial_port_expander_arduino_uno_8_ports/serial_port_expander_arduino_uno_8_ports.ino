//Modify this code as you see fit.
//This code will output data to the Arduino serial monitor.
//This code will allow you to control up to 8 Atlas Scientific devices through 1 soft serial RX/TX line.
//This code was written in the Arduino 1.8.9 IDE
//An Arduino UNO was used to test this code.
//This code was last tested 5/2019


//To open a channel (marked on the board as P1 to P8) send the number of the channel followed by a colon and the command (if any) that you want to send. End the string with a carriage return.
//1:r<CR>
//2:i<CR>
//3:c<CR>
//4:r<CR>

//To open a channel and not send a command just send the channel number followed by a colon.
//1:<CR>
//3:<CR>


#include <SoftwareSerial.h>                           //we have to include the SoftwareSerial library, or else we can't use it
#define rx 2                                          //define what pin rx is going to be
#define tx 3                                          //define what pin tx is going to be
SoftwareSerial myserial(rx, tx);                      //define how the soft serial port is going to work

String inputstring = "";                              //a string to hold incoming data from the PC
String sensorstring = "";                             //a string to hold the data from the Atlas Scientific product
boolean input_string_complete = false;                //have we received all the data from the PC
boolean sensor_string_complete = false;               //have we received all the data from the Atlas Scientific product

int s1 = 6;                                           //Arduino pin 6 to control pin S1
int s2 = 5;                                           //Arduino pin 5 to control pin S2
int s3 = 4;                                           //Arduino pin 4 to control pin S3
int port = 0;                                         //what port to open


void setup() {
  Serial.begin(9600);                                 //Set the hardware serial port to 9600
  myserial.begin(9600);                               //set baud rate for the software serial port to 9600
  inputstring.reserve(10);                            //set aside some bytes for receiving data from the PC
  sensorstring.reserve(30);                           //set aside some bytes for receiving data from Atlas Scientific product

  pinMode(s1, OUTPUT);                                //Set the digital pin as output
  pinMode(s2, OUTPUT);                                //Set the digital pin as output
  pinMode(s3, OUTPUT);                                //Set the digital pin as output
}


void serialEvent() {                                  //This interrupt will trigger when the data coming from the serial monitor(pc/mac/other) is received
  inputstring = Serial.readStringUntil(13);           //read the string until we see a <CR>
  input_string_complete = true;                       //set the flag used to tell if we have received a completed string from the PC
}



void loop() {

  if (input_string_complete) {                        //if a string from the PC has been received in its entirety
    pars();                                           //call the pars function to decode the string 
    open_port();                                      //call this function to open the correct port (p1-p8) 

    if (inputstring != "") {                          //if there is a message in the string (example 4:cal,1413) then send that message (cal,1413) through the open port 
      myserial.print(inputstring);                    //TX the message (cal,1413)
      myserial.print("\r");                           //and a craage return (cal,1413<CR>)
      inputstring = "";                               //clear the sent message so we don't send it again by mistake
    }
  }

  if (myserial.available() > 0) {                     //if we see that the Atlas Scientific product has sent a character
    char inchar = (char)myserial.read();              //get the char we just received
    sensorstring += inchar;                           //add the char to the var called sensorstring
    if (inchar == '\r') {                             //if the incoming character is a <CR>
      sensor_string_complete = true;                  //set the flag
    }
  }

  if (sensor_string_complete == true) {               //if a string from the Atlas Scientific product has been received in its entirety
    Serial.println(sensorstring);                     //send that string to the PC's serial monitor
    sensorstring = "";                                //clear the string
    sensor_string_complete = false;                   //reset the flag used to tell if we have received a completed string from the Atlas Scientific product
  }
}

void pars() {                                         //this function will decode the string (example 4:cal,1413)

  int colon = 0;                                      //where we will store the location of the colon in the string 
  String port_as_string = "";                         //we will extract the port number from the string and store it here (example 4)
  String command_string = "";                         //we will extract the message from the string and store it here (example cal,1413)

  colon = inputstring.indexOf(':');                  //find the location of the colon in the string 
  port_as_string = inputstring.substring(0, colon);  //extract the port number from the string and store it here  
  port = port_as_string.toInt();                     //convert the port number from a string to an int  
  inputstring = inputstring.substring(colon + 1);    //extract the message from the string and store it here
  input_string_complete = false;                     //reset the flag used to tell if we have received a completed string from the PC

}


void open_port() {                                  //this function controls what UART port is opened.

  if (port < 1 || port > 8)port = 1;                //if the value of the port is within range (1-8) then open that port. If it’s not in range set it to port 1
  port -= 1;                                        //the multiplexer used on the serial port expander refers to its ports as 0-15, but we have them labeled 1-16 by subtracting one from the port to be opened we correct for this.

  digitalWrite(s1, bitRead(port, 0));               //Here we have two commands combined into one.
  digitalWrite(s2, bitRead(port, 1));               //The digitalWrite command sets a pin to 1/0 (high or low)
  digitalWrite(s3, bitRead(port, 2));               //The bitRead command tells us what the bit value is for a specific bit location of a number
  delay(2);                                         //this is needed to make sure the channel switching event has completed
}
