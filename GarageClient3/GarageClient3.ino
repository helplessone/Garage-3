#define SENSOR_TYPE DEVICE_GARAGE
//#define SENSOR_TYPE DEVICE_THERMOMETER

#define ESP8266;
#define ARDUINO 10808

#include "garage.h"

#include <ESP8266WiFi.h>
//#include <ESP8266WiFiMulti.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>              //For UDP 
#include <ArduinoJson.h>

typedef struct {
  int month;
  int day;
  int year;
  int hour;
  int minute;  
} sensorTime;

typedef struct {
  bool sensor0;
  bool sensor1;
  bool swapSensors;
  sensorTime deviceTime;
} garageSensor;

typedef struct {
  int degreesC;
  int degreesF;
  int alarmHigh;
  int alarmLow;
  int minTemp;
  int maxTemp;
  sensorTime deviceTime;
} temperatureSensor;

#if SENSOR_TYPE == DEVICE_THERMOMETER
  #include <OneWire.h>
  #include <DallasTemperature.h>
  // Data wire is plugged into digital pin 2 on the Arduino
  #define ONE_WIRE_BUS 5 //D1 on NodeMCU
  // Setup a oneWire instance to communicate with any OneWire device
  OneWire oneWire(ONE_WIRE_BUS);
  // Pass oneWire reference to DallasTemperature library
  DallasTemperature sensors(&oneWire);
#endif

//ESP8266WiFiMulti WiFiMulti;
WiFiManager wifiManager;
WiFiUDP Udp;
ESP8266WebServer server(80); 


#ifndef GARAGE_UDP
  #define UDP_PORT 4204
  #define MAX_UDP_SIZE 255
#endif

#ifndef DEVICE_TYPES
  #define DEVICE_NONE 0
  #define DEVICE_ANY 99
  #define DEVICE_GARAGE 1
  #define DEVICE_THERMOMETER 2
  #define DEVICE_RELAY 3
  #define DEVICE_IRSENSOR 4
  #define DEVICE_WATER 5
#endif

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

#ifndef D0
  #define D0 16
#endif

#define DOORUP 5      //Input: 0 if door is up (NodeMCU D1)
#define DOORDOWN 4    //Input: 0 if door is down (NodeMCU D2)
#define DOORBUTTON 14 //Output: set HIGH to activate garage door
#define LED D0

bool doorUp, doorDown, startup = true;
String macID = "";
IPAddress serverIP;
unsigned long heartbeat = 0;

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED,HIGH);

#if SENSOR_TYPE == DEVICE_GARAGE
  pinMode(DOORUP, INPUT);
  pinMode(DOORDOWN, INPUT);
  pinMode(DOORBUTTON, OUTPUT);
  digitalWrite (DOORBUTTON, LOW);

  doorUp = digitalRead(DOORUP);
  doorDown = digitalRead(DOORDOWN);
#endif 

#if SENSOR_TYPE == SENSOR_THERMOMETER
  sensors.begin();  // Start up the library
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
  displayInfo();
  Udp.begin(UDP_PORT);         // Start listening for UDP
  startServer();

}

void loop() {
  handleStatusUpdate();  
  handleUDP();
  server.handleClient();
  delay(1);
}

void handleStatusUpdate() {
  String st;
  if (!(startup || (millis() - heartbeat > 10000) || (doorDown != digitalRead(DOORDOWN)) || (doorUp != digitalRead(DOORUP)))  ) return;
  displayInfo();
  heartbeat = millis();
  doorUp = digitalRead(DOORUP);
  doorDown = digitalRead(DOORDOWN);
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
    sensors.requestTemperatures(); 
    json["temperature"] = sensors.getTempCByIndex(0);
  #endif

  serializeJsonPretty(json,st);
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
                
#if SENSOR_TYPE == DEVICE_GARAGE        
        digitalWrite (DOORBUTTON, HIGH); //activate door
        delay(200);
        digitalWrite (DOORBUTTON, LOW);
#endif        
        digitalWrite(LED,HIGH);

        server.send(HTTP_CODE_OK);
        return;
      }
      server.send(HTTP_CODE_BAD_REQUEST);
    }
  });
  server.begin();
}
