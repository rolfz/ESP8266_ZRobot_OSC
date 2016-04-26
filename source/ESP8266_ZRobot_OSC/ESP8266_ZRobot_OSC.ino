/*---------------------------------------------------------------------------------------------

  Open Sound Control (OSC) library for the ESP8266

  Example for receiving open sound control (OSC) bundles on the ESP8266
  Send integers '0' or '1' to the address "/led" to turn on/off the built-in LED of the esp8266.

  This example code is in the public domain.
http://42bots.com/tutorials/28byj-48-stepper-motor-with-uln2003-driver-and-arduino-uno/
--------------------------------------------------------------------------------------------- */
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>

#include <AccelStepper.h>

#ifdef SOFTAP
char ssid[] = "ZROBOT";                  // your network SSID (name)
char pass[] = "pass";               // your network password
#else
char ssid[] = "SSID";          // your network SSID (name)
char pass[] = "PWD";               // your network password
#endif

// UDP and communication defs  
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

const IPAddress outIp(10,40,10,105);        // remote IP (not needed for receive)
const unsigned int outPort = 9999;          // remote port (not needed for receive)
const unsigned int localPort = 8888;        // local port to listen for UDP packets (here's where we send the packets)

int LED_PIN =  15;       //pin 13 on Arduino Uno. Pin 6 on a Teensy++2

// analog input for Disance sensor  
int SENSOR_PIN = A0;

OSCErrorCode error;
unsigned int ledState = LOW;              // LOW means led is *off*
unsigned int runit = 0;
int pwrval=0;
int dirval=0;
int revval=0; // reverse motors = 1

// Step motor defs
// change this to your motor if not NEMA-17 200 step
#define STEPS_PER_MREV 32  // Max steps for one revolution
#define STEPS_PER_OREV 32*64  // 2048 steps per rev
#define RPM 60     // Max RPM
#define DELAY 1    // Delay to allow Wifi to work
#define MAX_SPEED 1000.0

#define HALFSTEP 8
// Motor pin definitions
#define mPin11  2    //D4  IN1 on the ULN2003 driver 1
#define mPin12  4    //D3  IN2 on the ULN2003 driver 1
#define mPin13  0    //D2  IN3 on the ULN2003 driver 1
#define mPin14  5    //D1  IN4 on the ULN2003 driver 1

#define mPin21  13   //D0   IN1 on the ULN2003 driver 1
#define mPin22  14   //D5   IN2 on the ULN2003 driver 1
#define mPin23  12   //D6   IN3 on the ULN2003 driver 1
#define mPin24  16   //D8   IN4 on the ULN2003 driver 1

// GPIO Pins for Motor Driver board
AccelStepper Lstepper(HALFSTEP, mPin21,mPin22,mPin23,mPin24);
AccelStepper Rstepper(HALFSTEP, mPin11,mPin12,mPin13,mPin14);

// commands
// stepLeft.step(nr of steps)
// stepLeft.setSpeed(Long whatspeed)

//---------------------- Init Code --------------------------------------------
void setup() {
  
//  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
//  digitalWrite(BUILTIN_LED, ledState);    // turn *on* led

// setup ADC
//  ESP.ADC_MODE(ADC_VCC);

// setup UART
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  // Connect to WiFi network
  #ifdef SOFTAP
   Serial.print("Please connect to Robot on AP ");
   Serial.println(ssid);
   Serial.println("Password = " "pass""");
   
   WiFi.softAP(ssid, pass);
   WiFi.mode(WIFI_AP); 
  //WiFi.mode(WIFI_AP_STA);
  #else
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  #endif
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());

// init stepper
  // Set default speed to Max (doesn't move motor)
  Lstepper.setMaxSpeed(MAX_SPEED);
  Lstepper.move(1);  // I found this necessary
  Lstepper.setSpeed(0);

  Rstepper.setMaxSpeed(MAX_SPEED);
  Rstepper.move(-1);  // I found this necessary
  Rstepper.setSpeed(0);

}

//------------------------ Functions --------------------------------------

void toggleOnOff(OSCMessage &msg, int addrOffset){
  ledState = (boolean) msg.getFloat(0);
  OSCMessage msgOUT("/1/Lig");

  digitalWrite(LED_PIN, ledState);

  msgOUT.add(ledState);
  if (ledState) {
    Serial.println("LED on");
  }
  else {
    Serial.println("LED off");
  }

  digitalWrite(BUILTIN_LED, !ledState);    // turn *on* led

  ledState = !ledState;     // toggle the state from HIGH to LOW to HIGH to LOW ...

  //send osc message back to controll object in TouchOSC
  //Local feedback is turned off in the TouchOSC interface.
  //The button is turned on in TouchOSC interface whe the conrol receives this message.
  Udp.beginPacket(Udp.remoteIP(), outPort);
  msgOUT.send(Udp); // send the bytes
  Udp.endPacket(); // mark the end of the OSC Packet
  msgOUT.empty(); // free space occupied by message

}


void runCmd(OSCMessage &msg, int addrOffset ){

  runit = msg.getFloat(0);
  OSCBundle msgOUT;

  Serial.print("Robot ");

  if(runit==1)
  {  Serial.println("ON");
     msgOUT.add("/1/Logo").add("Z-Robot ON");
     digitalWrite(LED_PIN, 1);

  }
  else
  {
    Serial.println("OFF");
     msgOUT.add("/1/Logo").add("Z-Robot OFF");
     // put power cursor and value to 0
     msgOUT.add("/1/Pwr").add(0);
    // switch off motor power
     msgOUT.add("/1/PwrVal").add(0);
     pwrval=0;
     // and center the motor direction
     msgOUT.add("/1/Dir").add(0);
     dirval=0;
     digitalWrite(LED_PIN, 0);

  }
//  msgOUT.add(value);
//  msgOUT.add("Hello World");

  Udp.beginPacket(Udp.remoteIP(), outPort);
  msgOUT.send(Udp); // send the bytes
  Udp.endPacket(); // mark the end of the OSC Packet
  msgOUT.empty(); // free space occupied by message
}

void dirCmd(OSCMessage &msg, int addrOffset){
  
  OSCBundle msgOUT;
  
  if(runit)
  {
  dirval = (int) msg.getFloat(0);
  msgOUT.add("/1/Msg").add("OK"); 
  Serial.print("Dir : ");
  Serial.println(dirval);
  }
  else
  {
 // and we bring back the cursor to central place
  Serial.println("Robot OFF !!");
  dirval=0;

  msgOUT.add("/1/Dir").add(dirval);
  msgOUT.add("/1/Msg").add("Start Robot first"); 
     
  Udp.beginPacket(Udp.remoteIP(), outPort);
  msgOUT.send(Udp); // send the bytes
  Udp.endPacket(); // mark the end of the OSC Packet
  msgOUT.empty(); // free space occupied by message
  }
}

void cenCmd(OSCMessage &msg, int addrOffset){
  
  dirval = 0;
  OSCMessage msgOUT("/1/Dir");
  msgOUT.add(dirval);
  Serial.print("Center Dir = ");
  Serial.println(dirval);
  
  Udp.beginPacket(Udp.remoteIP(), outPort);
  msgOUT.send(Udp); // send the bytes
  Udp.endPacket(); // mark the end of the OSC Packet
  msgOUT.empty(); // free space occupied by message

}

void revCmd(OSCMessage &msg, int addrOffset){
  
  OSCMessage msgOUT("/1/RevLed");
  revval = (int) msg.getFloat(0);
  msgOUT.add(revval);
  
  Serial.print("Pwr : ");
  Serial.println(pwrval);
  
  Udp.beginPacket(Udp.remoteIP(), outPort);
  msgOUT.send(Udp); // send the bytes
  Udp.endPacket(); // mark the end of the OSC Packet
  msgOUT.empty(); // free space occupied by message

}

// add Tleft Tright

void buzCmd(OSCMessage &msg, int addrOffset){
  
  int ledval= (boolean) msg.getFloat(0);
  
  OSCMessage msgOUT("/led");
  if(ledval)
  msgOUT.add(0);
  else
  msgOUT.add(1);
    
  Serial.print("Led = ");
  Serial.println(ledval);
  
  Udp.beginPacket(Udp.remoteIP(), outPort);
  msgOUT.send(Udp); // send the bytes
  Udp.endPacket(); // mark the end of the OSC Packet
  msgOUT.empty(); // free space occupied by message
}

void pwrCmd(OSCMessage &msg, int addrOffset){
 
  OSCBundle msgOUT;
  if(runit)
  {
  pwrval = (int) msg.getFloat(0);
  msgOUT.add("/1/PwrVal").add(pwrval);
  msgOUT.add("/1/Msg").add("OK");

  Serial.print("Pwr : ");
  Serial.println(pwrval);

  }
  else
  {
    Serial.println("Robot OFF !!");
    pwrval=0;
    msgOUT.add("/1/Pwr").add(pwrval);
    msgOUT.add("/1/PwrVal").add(pwrval);
    msgOUT.add("/1/Msg").add("Start Robot first");
  }
  
  Udp.beginPacket(Udp.remoteIP(), outPort);
  msgOUT.send(Udp); // send the bytes
  Udp.endPacket(); // mark the end of the OSC Packet
  msgOUT.empty(); // free space occupied by message
}

void updateRadar(int radarDist){
  
  OSCMessage msgOUT("/1/Rfront"); // Rback
  msgOUT.add(int(radarDist));

  Udp.beginPacket(Udp.remoteIP(), outPort);
  msgOUT.send(Udp); // send the bytes
  Udp.endPacket(); // mark the end of the OSC Packet
  msgOUT.empty(); // free space occupied by message
}

void accelMsg(OSCMessage &msg, int addrOffset){
  float xval =  msg.getFloat(0);
  float yval =  msg.getFloat(1);
  float zval =  msg.getFloat(2);

  OSCBundle msgOUT;
//  OSCMessage msgOUT("/1/AX");

//  msgOUT.add(xval);
  msgOUT.add("/1/AX").add(xval);
  msgOUT.add("/1/AY").add(yval);
  msgOUT.add("/1/AZ").add(zval);

  Serial.print("AX : ");
  Serial.print(xval);
  Serial.print(" AY : ");
  Serial.print(yval);
  Serial.print(" AZ : ");
  Serial.println(zval);
  
  Udp.beginPacket(Udp.remoteIP(), outPort);
  msgOUT.send(Udp); // send the bytes
  Udp.endPacket(); // mark the end of the OSC Packet
  msgOUT.empty(); // free space occupied by message
}

void OSCMsgReceive(void){
  OSCMessage msgIN;
  int size;
  if((size = Udp.parsePacket())>0){

//#define TRACE_OSC

    // dump date to terminal for debugging
    #ifdef TRACE_OSC
    Udp.read(packetBuffer,UDP_TX_PACKET_MAX_SIZE);
    Serial.print("Contents: ");
    Serial.println(packetBuffer);
    #endif
    
    // read and interprete data
    while(size--)
      msgIN.fill(Udp.read());
     
    if(!msgIN.hasError()){
//      msgIN.route("/1/Lig",toggleOnOff);
      msgIN.route("/1/Run",runCmd);
      msgIN.route("/1/Dir",dirCmd);
      msgIN.route("/1/Buz",buzCmd);
      msgIN.route("/1/Cen",cenCmd);
      msgIN.route("/1/Rev",revCmd);
      msgIN.route("/1/Pwr",pwrCmd);
      msgIN.route("/1/accxyz",accelMsg);
    }
  }
}

//---------------------- Main code stats here----------------------------------
unsigned long lastMillis2;

void loop(void){
  //process received messages
  OSCMsgReceive();

// read sensor
unsigned long CurrentMillis = millis();
// slow down ADC to keep wifi alive, we mesure every 10ms
  if (CurrentMillis - lastMillis2 > 10)
  {
    int rDistance=analogRead(SENSOR_PIN);
    updateRadar(rDistance);
    lastMillis2 = CurrentMillis;
  }

// we will use dirvalue and pwrvalue to run the robot
/* variables used from iPhone, smartphone
 *  dirval,direction 
 *  pwrval,power 
 *  
 */ 
 int mixL,mixR;
 
   mixL=pwrval+dirval;
   if(mixL>MAX_SPEED)mixL=MAX_SPEED;
   mixR=-(pwrval-dirval);
   if(mixR>MAX_SPEED)mixR=MAX_SPEED;
 
  if(revval==1) // we reverse the motor sense
    {
     mixL*=-1;
     mixR*=-1;
    }
     
  
  Lstepper.setSpeed(mixL);
  Rstepper.setSpeed(-mixR);

  if(runit){
   Lstepper.runSpeed();
   Rstepper.runSpeed();
  }
  else //we switched off the robot
  {
    Lstepper.stop();
    Rstepper.stop();
  }
 }

// end of code
