#include <LiquidCrystal.h>

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

#define HEARTBEAT 10000

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
  int currentPosition;
  bool motorRunning;
  bool manualOverride = false;
  unsigned long interruptTimer;
  int interruptCounter;
  long int stallTimer;
  word stallCounter;

  int test = 0;
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
  deleteLog(); 
  
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
  attachInterrupt(digitalPinToInterrupt(IR), interrupt, RISING);
  pinMode(SWITCH,INPUT_PULLUP);
  pinMode(MOTOR, OUTPUT);
  digitalWrite(MOTOR,LOW);
  delay(50);
  pinMode(REVERSE, OUTPUT);
  motorOff();
  irValue = digitalRead(IR);
  if (device.currentPosition < 0) { 
    device.currentPosition = 0; //reset currentPosition
    saveDevice();
  }
  if (device.errorCode != 0) { 
    device.errorCode = 0; //clear error
    saveDevice();
  }
  currentPosition = device.currentPosition;
  interruptTimer = millis();
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
  if (!(startup || (millis() - heartbeat > HEARTBEAT) || 
                   (doorDown != digitalRead(DOORDOWN)) || 
                   (doorUp != digitalRead(DOORUP)))  ) return;
#endif
#if SENSOR_TYPE == DEVICE_THERMOMETER 
    sensors.requestTemperatures();
    float f = sensors.getTempCByIndex(0);
    if ((f < -120) || (f > 200)) f = currentTemp;
    if (!(startup || forceUpdate || (millis() - heartbeat > HEARTBEAT) || 
          (abs(currentTemp - f) > 1.0 ))) return;
    if (abs(currentTemp - f) > 1.0 ) Serial.println("Forced temp update"); 
//    if (tempRequest) Serial.println("**** TEMP REQUESTED ****");
    currentTemp = f; 
#endif
#if SENSOR_TYPE == DEVICE_LATCH 
  if (!(startup || (millis() - heartbeat > HEARTBEAT) || forceUpdate ||
                   (switchValue != digitalRead(SWITCH)))) return;
  switchValue = digitalRead(SWITCH);                   
#endif
#if SENSOR_TYPE == DEVICE_BAT
  processBatAlarm(); 
  if (!(startup || (millis() - heartbeat > HEARTBEAT) || forceUpdate)) return;
#endif 
 
#if SENSOR_TYPE == DEVICE_CURTAIN
  processCurtain(); 
  if (motorRunning && !forceUpdate) return;
  if (!(startup || (millis() - heartbeat > HEARTBEAT) || forceUpdate)) return;
#endif
  
#if SENSOR_TYPE == DEVICE_GARAGE  
  if (!(startup || (millis() - heartbeat > HEARTBEAT) || forceUpdate ||
      doorUp != digitalRead(DOORUP) || doorDown != digitalRead(DOORDOWN))) return;
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
    json["errorCode"] = device.errorCode;
    json["deviceColor"] = device.deviceColor;
  #endif   
  serializeJsonPretty(json,st);
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

ICACHE_RAM_ATTR void interrupt() {
  if (millis() - interruptTimer < 50) return;
  if (currentDirection == DOWN) interruptCounter++; else interruptCounter--;
  interruptTimer = millis();
  return;
/*  
  if (currentDirection == UP && digitalRead(SWITCH) == 0) {
    motorOff();
    device.currentPosition = 0;
    manualOverride = false;
    saveDevice();
  }  
*/  

  if (millis() - interruptTimer < 100) return;
  if (currentDirection == DOWN) device.currentPosition++; else device.currentPosition--;  
  Serial.printf("count: %d\n",device.currentPosition);
  interruptTimer = millis();
  
}

void motorOn(){
  if (device.errorCode != 0) {
    Serial.printf("Error code: %d\n",device.errorCode);
    return;
  }
  Serial.printf("Motor ON: Direction = %s, Reverse = %d, Position = %d\n",(currentDirection==UP)?"UP":"DOWN", device.motorReverse, device.currentPosition);
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
  device.deviceColor = YELLOW;
  forceUpdate = true;
  motorRunning = true;
  stallCounter = device.currentPosition;
  interruptCounter = device.currentPosition;
  digitalWrite(MOTOR,HIGH);
  stallTimer = millis();
}

void motorOff() {
  Serial.printf("Motor OFF: Direction = %s, Reverse = %d, Position = %d\n",(currentDirection==UP)?"UP":"DOWN", device.motorReverse, device.currentPosition);
  digitalWrite(MOTOR,LOW);
  delay(50);
  digitalWrite(REVERSE,LOW);
  motorRunning = false;
  if (device.currentPosition >= device.rotationCount) device.deviceColor = BLUE;
  else if (device.currentPosition <= 0) device.deviceColor = GREEN;
  else device.deviceColor = YELLOW;
  forceUpdate = true;
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

void processCurtain(){
  int cnt = 0;  
  if (device.errorCode != 0) return;
 
  if (motorRunning && millis() - stallTimer > STALLTIME) {
    Serial.printf("[STALL]interrupt: %d currentPosition: %d\n",interruptCounter, device.currentPosition);    
    if (interruptCounter == device.currentPosition) {
      motorOff();
      saveDevice();
      delay(50);
      device.deviceColor = RED;
      device.errorCode = CURTAIN_STALL_ERROR;   
      forceUpdate = true;
      return;
    }
  } 

  if (currentDirection == UP && digitalRead(SWITCH) == 0) {
    motorOff();
    device.currentPosition = 0;
    interruptCounter = 0;
    manualOverride = false;
    saveDevice();
    return;
  }

  if (interruptCounter != device.currentPosition) {
    device.currentPosition = interruptCounter;
    stallTimer = millis();
    if (currentDirection == DOWN) {
      if (device.currentPosition >= device.rotationCount && !manualOverride) {
        motorOff();
        saveDevice();
        device.deviceColor = BLUE; 
        forceUpdate = true;
      }
    } else {
      if (!manualOverride) {
        if (device.currentPosition <= -12) { // This shouldn't happen, switch should cut it off
          Serial.println("Error, switch not hit");
          motorOff(); 
          saveDevice();
          device.errorCode = CURTAIN_SWITCH_ERROR;
          device.deviceColor = RED;
          forceUpdate = true;
        }
      }
    }
    Serial.printf("count: %d\n",device.currentPosition);    
  }

/**
  if (irValue != digitalRead(IR)) {
    for (int i=0; i<30; i++) {
      if (irValue != digitalRead(IR)) cnt++;
      delay(0);
    }
    if (cnt > 20) {
      irValue = digitalRead(IR);
      if (irValue == 1) {
        if (currentDirection == DOWN) {
          device.currentPosition++;
          if (device.currentPosition >= device.rotationCount && !manualOverride) {
            motorOff(); 
            forceUpdate = true;
          }
          saveDevice();
        } else {
          device.currentPosition--; 
          if (!manualOverride) {
            if (device.currentPosition <= -24) { // This shouldn't happen, switch should cut it off
              Serial.println("Error, switch not hit");
              motorOff(); 
              device.errorCode = CURTAIN_SWITCH_ERROR;
              device.deviceColor = RED;
              forceUpdate = true;
            }
          }
          saveDevice();          
        }
        Serial.printf("count: %d test: %d\n",device.currentPosition, test);
      }
    }
  }
*/  
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
          } 
          if (device.currentPosition > 0) {
            currentDirection = UP;
            motorOn();
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

#if SENSOR_TYPE == DEVICE_CURTAIN
        if (Name.equals("rotationCount")) {
          device.rotationCount = Value.toInt();
          String s = "rotationCount = " + String(device.rotationCount) + "\n";
          logString(s);
          saveDevice();
          forceUpdate = true;
        }
        if (Name.equals("motorReverse")) {
          device.motorReverse = !device.motorReverse;
          saveDevice();
          forceUpdate = true;
        }
        if (Name.equals("lowerCurtain")) lowerCurtain();
        if (Name.equals("raiseCurtain")) raiseCurtain();
        
        if (Name.equals("downButton")) {
          manualOverride = true;
          currentDirection = DOWN;
          device.errorCode = 0;
          motorOn();          
        }
        if (Name.equals("upButton")) {
          manualOverride = true;
          currentDirection = UP;
          device.errorCode = 0;
          motorOn();          
        }
        if (Name.equals("stopButton")) motorOff();
        if (Name.equals("setBottom")) {
          device.rotationCount = device.currentPosition;
          saveDevice();
          forceUpdate = true;
        }      
//        Serial.printf("rotationCount = %d\n",device.rotationCount);        
#endif        
      }
    }
    server.send(HTTP_CODE_OK);
    saveDevice();
  });
  
  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
                                              // and check if the file exists  
  server.begin();
}

void handleNotFound(){ // if the requested file or page doesn't exist, return a 404 not found error
  if(!handleFileRead(server.uri())){          // check if the file exists in the flash memory (SPIFFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found");
  }
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) {
    path += "index.html";          // If a folder is requested, send the index file
  }
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  if (path == "/var.json") {
    Serial.println("var.json requested");
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
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
  SPIFFS.remove("/device.txt");
  Serial.println ("/device.txt deleted");
  clearDevice();
  ESP.reset();
}

void saveDevice(){
  File f = SPIFFS.open("/device.txt","w");
  if (!f) return;
  f.write((const char *)&(device),sizeof(device));
  f.close();  
}

void restoreDevice() {
  char buffer[sizeof(Device)];
  clearDevice();
  File f = SPIFFS.open("/device.txt","r");
  if (!f) return;
  f.readBytes((char *)&(device),sizeof(device));
  f.close(); 
}

void logString(String st){
  File f = SPIFFS.open("/log.txt","a");
  if (!f) return;
  f.write(st.c_str());
  f.close(); 
}

void deleteLog(){
  SPIFFS.remove("/log.txt");
}
