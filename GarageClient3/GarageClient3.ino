//#define SENSOR_TYPE DEVICE_GARAGE
//#define SENSOR_TYPE DEVICE_THERMOMETER
//#define SENSOR_TYPE DEVICE_LATCH
//#define SENSOR_TYPE DEVICE_BAT
#define SENSOR_TYPE DEVICE_CURTAIN

#define ESP8266;
#include <IotSensors.h>
#define ARDUINO 10808

#include <ESP8266WiFi.h>
//#include <ESP8266WiFiMulti.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <FS.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>              //For UDP 
#include <ArduinoJson.h>
             
WiFiManager wifiManager;
WiFiUDP Udp;
ESP8266WebServer server(80);

/*
#ifndef GARAGE_UDP
  #define UDP_PORT 4204
  #define MAX_UDP_SIZE 255
#endif
*/

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

#ifndef D0
  #define D0 16
#endif

Device device;

#if SENSOR_TYPE == DEVICE_THERMOMETER
  #define deviceTypeString "THERMO"
  #include <OneWire.h>
  #include <DallasTemperature.h>
  // Data wire is plugged into digital pin 2 on the Arduino
  #define ONE_WIRE_BUS 5 //D1 on NodeMCU
  // Setup a oneWire instance to communicate with any OneWire device
  OneWire oneWire(ONE_WIRE_BUS);
  // Pass oneWire reference to DallasTemperature library
  DallasTemperature sensors(&oneWire);
  float currentTemp = 0.0;
#endif

#if SENSOR_TYPE == DEVICE_GARAGE
  #define deviceTypeString "GARAGE"
  #define DOORUP 5      // (D1) Input: 0 if door is up
  #define DOORDOWN 4    // (D2) Input: 0 if door is down
  #define DOORBUTTON 14 // (D5) Output: set HIGH to activate garage door
  int doorUp, doorDown;
#endif

#if SENSOR_TYPE == DEVICE_LATCH
  #define deviceTypeString "LATCH"
  #define SWITCH 4      // (D2) Input: 0 if door is up
  bool switchValue;
#endif

#if SENSOR_TYPE == DEVICE_BAT
  #define deviceTypeString "BAT"
  #define PIR 4         // (D2) Input from PIR sensor
  #define BAT_ALARM 14  // (D5) Output: set HIGH to activate external alarm
  bool pirValue;
  unsigned long batTimer;
  byte alarmState;
  byte alarmRepeat;
#endif

#if SENSOR_TYPE == DEVICE_CURTAIN
  #define deviceTypeString "CURTAIN"
  #define SWITCH 5      // (D1) Input from SWITCH (curtain up)
  #define IR 4          // (D2) Input from IR sensor
  #define MOTOR 14      // (D5) Output: HIGH=Motor On, LOW=Motor Off
  #define REVERSE 12    // (D6) Output - Reverses motor direction
  #define UP 1          //currentDirection UP
  #define DOWN 0        //currentDirection DOWN
  #define STALLTIME 1500    
  int irValue;
  int currentDirection = 0; //UP or DOWN
  bool motorRunning;
  bool manualOverride = false;
  long int stallTimer;
  bool motorFault = false;
#endif

#define LED D0

bool startup = true;
bool forceUpdate = false;
char OTAName[15];  // A name and a password for the OTA service
const char *OTAPassword = "";     //"esp8266";
String macID = "";
IPAddress serverIP;
IPAddress myIP;
unsigned long heartbeat = 0;

/************************  SETUP ************************/
void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED,HIGH);
  SPIFFS.begin();              // Start the SPI Flash File System (SPIFFS)
  
  restoreDevice();  
  
#if SENSOR_TYPE == DEVICE_GARAGE
  pinMode(DOORUP, INPUT);
  pinMode(DOORDOWN, INPUT);
  pinMode(DOORBUTTON, OUTPUT);
  digitalWrite (DOORBUTTON, LOW);
  doorUp = digitalRead(DOORUP);
  doorDown = digitalRead(DOORDOWN);
#endif 

#if SENSOR_TYPE == SENSOR_THERMOMETER
  sensors.begin();  // Start up the temperature library
#endif

#if SENSOR_TYPE == DEVICE_LATCH
  pinMode(SWITCH, INPUT);
  switchValue = digitalRead(SWITCH);
#endif 

#if SENSOR_TYPE == DEVICE_BAT
  pinMode(PIR, INPUT);
  pinMode(BAT_ALARM, OUTPUT);
  digitalWrite (BAT_ALARM, LOW);  //Turn off alarm  
  pirValue = 0; //digitalRead(PIR);
  alarmState = 0;
#endif 

#if SENSOR_TYPE == DEVICE_CURTAIN
  pinMode(IR, INPUT);
  pinMode(SWITCH,INPUT_PULLUP);
  pinMode(MOTOR, OUTPUT);
  digitalWrite(MOTOR,LOW);
  delay(50);
  pinMode(REVERSE, OUTPUT);
  motorOff();
  motorFault = false;
  irValue = digitalRead(IR);
#endif

  macID = WiFi.macAddress();

  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(200);
  } 

  startWiFi(); 
  myIP = WiFi.localIP();
  sprintf(OTAName, "%s_%03d",deviceTypeString,myIP[3]);
  
  startOTA();                  // Start the OTA service
  displayInfo();
  Udp.begin(UDP_PORT);         // Start listening for UDP
  startServer();
}

//unsigned long MDNStimer = millis();

void loop() {
  handleStatusUpdate();  
  ArduinoOTA.handle();                        // listen for OTA events
  handleUDP();
  server.handleClient();
  delay(1);
}

void handleStatusUpdate() {
  String st;
#if SENSOR_TYPE == DEVICE_GARAGE 
  if (!(startup || (millis() - heartbeat > 10000) || 
                   (doorDown != digitalRead(DOORDOWN)) || 
                   (doorUp != digitalRead(DOORUP)))  ) return;
#endif
#if SENSOR_TYPE == DEVICE_THERMOMETER 
    sensors.requestTemperatures();
    float f = sensors.getTempCByIndex(0);
    if ((f < -120) || (f > 200)) f = currentTemp;
    if (!(startup || forceUpdate || (millis() - heartbeat > 10000) || 
          (abs(currentTemp - f) > 1.0 ))) return;
    if (abs(currentTemp - f) > 1.0 ) Serial.println("Forced temp update"); 
//    if (tempRequest) Serial.println("**** TEMP REQUESTED ****");
    currentTemp = f; 
#endif
#if SENSOR_TYPE == DEVICE_LATCH 
  if (!(startup || (millis() - heartbeat > 10000) || forceUpdate ||
                   (switchValue != digitalRead(SWITCH)))) return;
  switchValue = digitalRead(SWITCH);                   
#endif
#if SENSOR_TYPE == DEVICE_BAT
  processBatAlarm(); 
  if (!(startup || (millis() - heartbeat > 10000) || forceUpdate)) return;
#endif 
 
#if SENSOR_TYPE == DEVICE_CURTAIN
  processCurtain(); 
  if (motorRunning) return;
  if (!(startup || (millis() - heartbeat > 10000) || forceUpdate)) return;
#endif
  
#if SENSOR_TYPE == DEVICE_GARAGE  
  doorUp = digitalRead(DOORUP);
  doorDown = digitalRead(DOORDOWN);
#endif  

  startup = false;
  forceUpdate = false;                           
  displayInfo();
  heartbeat = millis(); 
  
  HTTPClient http;
  http.begin("http://"+serverIP.toString()+"/var.json"); 
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> json;
  json["mac"] = macID;
  json["deviceType"] = SENSOR_TYPE;   

  #if SENSOR_TYPE == DEVICE_GARAGE  
    json["sensor0"] = doorUp;
    json["sensor1"] = doorDown;
  #endif
  #if SENSOR_TYPE == DEVICE_THERMOMETER  
    int temp =  (int)(currentTemp * 100);
    json["temp"] = temp;
    Serial.print("temp = ");
    Serial.println(temp);
    Serial.println(sensors.getTempCByIndex(0));
  #endif
  #if SENSOR_TYPE == DEVICE_LATCH  
    json["switchValue"] = switchValue?1:0;
  #endif
  #if SENSOR_TYPE == DEVICE_BAT
    json["pirValue"] = pirValue?1:0;
    json["alarmState"] = alarmState;
  #endif  
  #if SENSOR_TYPE == DEVICE_CURTAIN
    json["currentPosition"] = device.currentPosition;
    json["rotationCount"] = device.rotationCount;
    json["motorReverse"] = device.motorReverse;
  #endif   
  serializeJsonPretty(json,st);
//  Serial.println(st);
  int httpCode = http.POST(st);
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    // Serial.printf("[HTTP] POST code: %d\n", httpCode);
    // file found at server
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      //String payload = http.getString();
      //Serial.println(payload);
    }
  } else {
    Serial.printf("[HTTP] POST failed, error: %d %s\n", httpCode, http.errorToString(httpCode).c_str());
    findServer();
  }
}

#if SENSOR_TYPE == DEVICE_CURTAIN
void motorOn(){
  Serial.printf("Motor ON: Direction = %d, Reverse = %d\n",currentDirection, device.motorReverse);
  if (motorFault) return;
  if (device.motorReverse == 0) {
    if (currentDirection == DOWN) {
      digitalWrite(REVERSE,LOW); 
    } else {
      digitalWrite(REVERSE,HIGH);
    }
  } else {
    if (currentDirection == DOWN) {
      digitalWrite(REVERSE,HIGH); 
    } else {
      digitalWrite(REVERSE,LOW);
    }   
  }
  delay(50);
  digitalWrite(MOTOR,HIGH);
  motorRunning = true;
  stallTimer = millis();
}

void motorOff() {
  Serial.println("Motor OFF");
  digitalWrite(MOTOR,LOW);
  delay(50);
  digitalWrite(REVERSE,LOW);
  motorRunning = false;
  forceUpdate = true;
}

void processCurtain(){
  int cnt = 0;  
  if (motorFault) return;
   
  if (motorRunning && millis() - stallTimer > STALLTIME) {
    motorOff();
    motorFault = true;
    delay(50);
    device.currentPosition = 1000;
    forceUpdate = true;
    return; 
  }  
 
  if (currentDirection == UP && digitalRead(SWITCH) == 0) {
    motorOff();
    device.currentPosition = 0;
    manualOverride = false;
    return;
  }
  if (irValue != digitalRead(IR)) {
    for (int i=0; i<30; i++) {
      if (irValue != digitalRead(IR)) cnt++;
      delay(1);
    }
    if (cnt > 25) {
      stallTimer = millis();
      irValue = digitalRead(IR);
      if (irValue == 1) {
        if (currentDirection == DOWN) {
          device.currentPosition++;
//          if (device.currentPosition % 16 == 0) forceUpdate = true; //update server every gear turn
          if (device.currentPosition >= device.rotationCount && !manualOverride) {
            motorOff(); 
            forceUpdate = true;
          }
          saveDevice();
        } else {
          device.currentPosition--;
//          if (device.currentPosition % 16 == 0) forceUpdate = true; //update server every gear turn
          if (!manualOverride) {
            if (device.currentPosition <= -16) { // This shouldn't happen, switch should cut it off
              Serial.println("Error, switch not hit");
              motorOff(); 
              device.currentPosition = 0;
              forceUpdate = true;
            }
          }
          saveDevice();          
        }
//        Serial.printf("  Position = %d\n",device.currentPosition);
      }
    }
  }
}
#endif

#if SENSOR_TYPE == DEVICE_BAT
void processBatAlarm(){
  if (pirValue != digitalRead(PIR)) forceUpdate = true;
  pirValue = digitalRead(PIR);
  switch (alarmState) {

/*    
    case 0: {
      if (pirValue == 1) {
        digitalWrite(BAT_ALARM,HIGH); //turn on alarm
        digitalWrite(LED,LOW); //turn on on-board LED        
        Serial.println("State 0 -> 1");
        alarmState = 1;
        alarmRepeat = 0;
        batTimer = millis();
      }
    }
    break;
*/    
    case 1: {
      if (millis() - batTimer > 10800000) {     // 10800000 = 3 hours   
        Serial.println("State 1 -> 0");
        alarmState = 0;
        batTimer = millis();
        digitalWrite(BAT_ALARM,LOW); //turn off alarm
        digitalWrite(LED,HIGH); //turn off on-board LED        
      }
    }
    break;
    case 2: {
      if (millis() - batTimer > 100) {
        if (alarmRepeat++ < 3) {
          Serial.println("State 2 -> 1");
          batTimer = millis();
          alarmState = 1;
          digitalWrite(BAT_ALARM,HIGH);  
          digitalWrite(LED,LOW);    //turn on on-board LED        
        } else {
          Serial.println("State 2 -> 3");
          batTimer = millis();
          alarmState = 3;
        }
      }
    }
    break;
    case 3: {
      if (millis() - batTimer > 100) {
        Serial.println("State 3 -> 0");
        alarmState = 0;
      }
    }
  }
}
#endif

void startWiFi() {
  digitalWrite(LED,LOW);
#if SENSOR_TYPE == DEVICE_GARAGE   
  wifiManager.autoConnect("Garage Door Sensor");
#endif
#if SENSOR_TYPE == DEVICE_THERMOMETER   
  wifiManager.autoConnect("Temperature Sensor");
#endif
#if SENSOR_TYPE == DEVICE_LATCH   
  wifiManager.autoConnect("Switch Sensor");
#endif
#if SENSOR_TYPE == DEVICE_BAT   
  wifiManager.autoConnect("Motion Sensor");
#endif
#if SENSOR_TYPE == DEVICE_CURTAIN   
  wifiManager.autoConnect("Curtain");
#endif
digitalWrite(LED,HIGH);
}

void displayInfo() {
#if SENSOR_TYPE == DEVICE_GARAGE  
  String st;
  st = "Door Up = ";
  st += digitalRead(DOORUP);
  st += "\nDoor Down = ";
  st += digitalRead(DOORDOWN);
  st += "\nMAC ID: ";
  st += macID + "\n";
  Serial.println(st);
#endif
#if SENSOR_TYPE == DEVICE_THERMOMETER
  // Send the command to get temperatures
  sensors.requestTemperatures(); 
  //print the temperature in Celsius
  Serial.print("Temperature: ");
  Serial.print(sensors.getTempCByIndex(0));
  Serial.print(" "); //(char)0xb0);//shows degrees character
  Serial.print("C  |  ");
  
  //print the temperature in Fahrenheit
  Serial.print((sensors.getTempCByIndex(0) * 9.0) / 5.0 + 32.0);
  Serial.print(" "); //(char)0xb0);//shows degrees character
  Serial.println("F");
#endif  
}

void handleUDP() {
  int packetSize = Udp.parsePacket();
  if (!packetSize) return;
  
  // receive incoming UDP packets
  Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
  char incomingPacket[MAX_UDP_SIZE];
  int len = Udp.read(incomingPacket, MAX_UDP_SIZE);
  if (len > 0)
  {
    incomingPacket[len] = 0;
  }
  Serial.printf("UDP packet contents: %s\n", incomingPacket);
  
  if (strcmp(incomingPacket, SEARCH_MESSAGE)==0) { // Someone is looking for me
    Udp.beginPacket(Udp.remoteIP(),UDP_PORT);
    Udp.write(LINK_MESSAGE);
    Udp.endPacket();
  }
  
  if (strcmp(incomingPacket, LINKED_MESSAGE)==0) { // I have been linked to server 
    serverIP = Udp.remoteIP();
  }
  if (strcmp(incomingPacket, ACTION_MESSAGE)==0) { //TODO handle potential actions here

  }
}

void startOTA() { // Start the OTA service
  ArduinoOTA.setHostname(OTAName);
  ArduinoOTA.setPassword(OTAPassword);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\r\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready\r\n");
}

void findServer() {
  IPAddress scanAddress = WiFi.localIP();
  scanAddress[3] = 255;
  Serial.printf("Now broadcasting at IP %s, UDP port %d\n", scanAddress.toString().c_str(), UDP_PORT);
  Udp.beginPacket(scanAddress, UDP_PORT);
  Udp.write(LINK_MESSAGE);
  Udp.endPacket();
}

void startServer() {
  server.on("/", HTTP_POST, []() {
    for (int i = 0; i < server.args(); i++) {
      Serial.printf("server.on : %s\n", server.arg(i).c_str());
      if (server.arg(i).equals(ACTION_MESSAGE)) {
        digitalWrite(LED,LOW);
        server.send(HTTP_CODE_OK);
        
#if SENSOR_TYPE == DEVICE_THERMOMETER
        delay (200); //let the LED blink
        forceUpdate = true;
#endif        
                
#if SENSOR_TYPE == DEVICE_GARAGE        
        digitalWrite (DOORBUTTON, HIGH); //push the door opener button
        delay(200);   
        digitalWrite (DOORBUTTON, LOW);  //and release it
#endif
#if SENSOR_TYPE == DEVICE_LATCH
        delay(200);   //just blink the LED
        forceUpdate = true;
#endif
#if SENSOR_TYPE == DEVICE_BAT
        if (alarmState == 0) { //toggle alarm
          digitalWrite(BAT_ALARM,HIGH);   //force a bat alarm
          Serial.println("Forced State 0 -> 1");
          batTimer = millis();        
          alarmState = 1;
        } else {
          digitalWrite(BAT_ALARM,LOW);   //turn off bat alarm
          Serial.println("Forced State to 0");
          alarmState = 0;
         
        }
        alarmRepeat = 0;
        delay(200);   //just blink the LED
        forceUpdate = true; 
#endif
#if SENSOR_TYPE == DEVICE_CURTAIN
        Serial.printf("motorRunning = %d\n",motorRunning);
        if (motorRunning) motorOff();
        else {
          if (device.currentPosition <= 0) { //if up, put it down
            currentDirection = DOWN; 
            motorOn();
            Serial.printf("<=0 Direction: %d, Position: %d, Running: %d\n",currentDirection,device.currentPosition, motorRunning);
          } 
          if (device.currentPosition > 0) {
            currentDirection = UP;
            motorOn();
            Serial.printf(">0 Direction: %d, Position: %d, Running: %d\n",currentDirection,device.currentPosition, motorRunning);
          }
        }
#endif       
        digitalWrite(LED,HIGH);  //turn LED Off
        return;
      }
      server.send(HTTP_CODE_BAD_REQUEST);
    }
  });

    server.on("/alarm", HTTP_POST, []() {
    for (int i = 0; i < server.args(); i++) {
      Serial.println(server.arg(i));
      if (server.arg(i).equals(ACTION_MESSAGE)) {
        digitalWrite(LED,LOW);
        server.send(HTTP_CODE_OK);
        
#if SENSOR_TYPE == DEVICE_BAT
        if (alarmState == 0) { // if alarm is off, turn it on
          digitalWrite(BAT_ALARM,HIGH);   //force a bat alarm
          Serial.println("Forced State 0 -> 1");
          batTimer = millis();        
          alarmState = 1;  
          alarmRepeat = 0;
          delay(200);   //just blink the LED
          forceUpdate = true;
        } 
 #endif
        digitalWrite(LED,HIGH);  //turn LED Off
        return;
      }
      server.send(HTTP_CODE_BAD_REQUEST);
    }
  });

  server.on("/set", HTTP_POST, []() {
    String Name, Value;
    for (int i = 0; i < server.args(); i++) {
      if (server.argName(i).equals("plain")) {
        Name = getArgName(server.arg(i));
        Value = getArgValue(server.arg(i));
        Serial.printf("Name = |%s|, Value = |%s|\n",Name.c_str(),Value.c_str());

        if (Name.equals("rotationCount")) device.rotationCount = Value.toInt();
        if (Name.equals("motorReverse")) {
          device.motorReverse = !device.motorReverse;
          forceUpdate = true;
        }
        if (Name.equals("lowerCurtain")) lowerCurtain();
        if (Name.equals("raiseCurtain")) raiseCurtain();
        
        if (Name.equals("downButton")) {
          manualOverride = true;
          currentDirection = DOWN;
          motorOn();          
        }
        if (Name.equals("upButton")) {
          manualOverride = true;
          currentDirection = UP;
          motorOn();          
        }
        if (Name.equals("stopButton")) motorOff();
        if (Name.equals("setBottom")) {
          device.rotationCount = device.currentPosition;
          saveDevice();
        }
        

        Serial.printf("rotationCount = %d\n",device.rotationCount);        
      }
    }
    server.send(HTTP_CODE_OK);
    saveDevice();
  });
  
  server.begin();
}

void lowerCurtain(){
  if (device.currentPosition <= 4) { //if up (or just slightln down), put it down
    currentDirection = DOWN; 
    motorOn(); 
  }  
}

void raiseCurtain(){
  if (device.currentPosition > 0) {
    currentDirection = UP;
    motorOn();     
  }   
}

String getArgName (String st) {
  return (st.substring(0,st.indexOf('=')));  
}

String getArgValue (String st) {
  return (st.substring(st.indexOf('=')+1,st.length()));    
}

/*
void startMDNS() { // Start the mDNS responder
  MDNS.begin(mdnsName);                        // start the multicast domain name server
  Serial.print("mDNS responder started: http://");
  Serial.print(mdnsName);
  Serial.println(".local");
}
*/

void clearDevice() {
  memset(&device, '\0', sizeof(Device));   //clear devices array to 0's
  device.deviceType = SENSOR_TYPE;
  device.online = 0;
  device.timer = millis();
#if SENSOR_TYPE == DEVICE_CURTAIN  
  device.currentPosition = 0;
  device.rotationCount = 0;
  device.upTime = 0;
  device.downTime = 0;
#endif  
} 

void deleteSettings(){
  SPIFFS.remove("/devices.txt");
  Serial.println ("/devices.txt deleted");
  clearDevice();
  ESP.reset();
}

void saveDevice(){
  File f = SPIFFS.open("/devices.txt","w");
  if (!f) return;
  f.write((const char *)&(device),sizeof(device));
  f.close();  
}

void restoreDevice() {
  char buffer[sizeof(Device)];
  clearDevice();
  File f = SPIFFS.open("/devices.txt","r");
  if (!f) return;
  f.readBytes((char *)&(device),sizeof(device));
  f.close(); 
}
