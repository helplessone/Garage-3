#define SENSOR_TYPE DEVICE_GARAGE
//#define SENSOR_TYPE DEVICE_THERMOMETER
//#define SENSOR_TYPE DEVICE_LATCH

#define ESP8266;
#include <IotSensors.h>
#define ARDUINO 10808

#include <ESP8266WiFi.h>
//#include <ESP8266WiFiMulti.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>              //For UDP 
#include <ArduinoJson.h>


#if SENSOR_TYPE == DEVICE_THERMOMETER
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

  bool forceUpdate = false;

//ESP8266WiFiMulti WiFiMulti;
WiFiManager wifiManager;
WiFiUDP Udp;
ESP8266WebServer server(80); 
const char *OTAName = "ESP8266";  // A name and a password for the OTA service
const char *OTAPassword = "";     //"esp8266";


#ifndef GARAGE_UDP
  #define UDP_PORT 4204
  #define MAX_UDP_SIZE 255
#endif

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

#ifndef D0
  #define D0 16
#endif

#if SENSOR_TYPE == DEVICE_GARAGE
  #define DOORUP 5      //Input: 0 if door is up (NodeMCU D1)
  #define DOORDOWN 4    //Input: 0 if door is down (NodeMCU D2)
  #define DOORBUTTON 14 //Output: set HIGH to activate garage door
#endif

#if SENSOR_TYPE == DEVICE_LATCH
  #define SWITCH 4      //Input: 0 if door is up (NodeMCU D2)
#endif

#define LED D0

#if SENSOR_TYPE == DEVICE_GARAGE
  int doorUp, doorDown;
#endif

#if SENSOR_TYPE == DEVICE_LATCH
  bool switchValue;
#endif

bool startup = true;
String macID = "";
IPAddress serverIP;
unsigned long heartbeat = 0;

/************************  SETUP ************************/
void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED,HIGH);
  SPIFFS.begin();              // Start the SPI Flash File System (SPIFFS)
  
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
  startOTA();                  // Start the OTA service
  displayInfo();
  Udp.begin(UDP_PORT);         // Start listening for UDP
  startServer();

}

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
    if (tempRequest) Serial.println("**** TEMP REQUESTED ****");
    currentTemp = f; 
#endif
#if SENSOR_TYPE == DEVICE_LATCH 
  if (!(startup || (millis() - heartbeat > 10000) || forceUpdate ||
                   (switchValue != digitalRead(SWITCH)))) return;
#endif
  forceUpdate = false;                                 
  displayInfo();
  heartbeat = millis();
#if SENSOR_TYPE == DEVICE_GARAGE  
  doorUp = digitalRead(DOORUP);
  doorDown = digitalRead(DOORDOWN);
#endif  
#if SENSOR_TYPE == DEVICE_LATCH
  switchValue = digitalRead(SWITCH);
#endif   
  startup = false;
  
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
//    sensors.requestTemperatures();
    int temp =  (int)(currentTemp * 100);
    json["temp"] = temp;
    Serial.print("temp = ");
    Serial.println(temp);
    Serial.println(sensors.getTempCByIndex(0));
  #endif
  #if SENSOR_TYPE == DEVICE_LATCH  
    json["switchValue"] = switchValue?1:0;
  #endif
  
  serializeJsonPretty(json,st);
  Serial.println(st);
  int httpCode = http.POST(st);
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] POST code: %d\n", httpCode);

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

void startWiFi() {
  digitalWrite(LED,LOW);
#if SENSOR_TYPE == DEVICE_GARAGE   
  wifiManager.autoConnect("Garage Door Sensor");
#endif
#if SENSOR_TYPE == DEVICE_THERMOMETER   
  wifiManager.autoConnect("Temperature Sensor");
#endif
#if SENSOR_TYPE == DEVICE_LATCH   
  wifiManager.autoConnect("Latch Sensor");
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
      Serial.println(server.arg(i));
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
        digitalWrite(LED,HIGH);  //turn LED Off
        return;
      }
      server.send(HTTP_CODE_BAD_REQUEST);
    }
  });
  server.begin();
}
