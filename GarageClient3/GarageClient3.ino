/**
   BasicHTTPClient.ino

    Created on: 24.05.2015

*/
#define ESP8266;
#include "../GarageServer3/garage.h"
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>

#include <WiFiUdp.h>              //For UDP 

ESP8266WiFiMulti WiFiMulti;
WiFiManager wifiManager;
WiFiUDP Udp;

#ifndef GARAGE_UDP
  #define UDP_PORT 4204
  #define MAX_UDP_SIZE 255
#endif

#ifndef DEVICE_TYPES
  #define DEVICE_NONE 0
  #define DEVICE_ANY 99
  #define DEVICE_GARAGE 1
  #define DEVICE_THERMOMETOR 2
  #define DEVICE_RELAY 3
  #define DEVICE_IRSENSOR 4
  #define DEVICE_WATER 5
#endif

#define DOORUP 4
#define DOORDOWN 5
#define SENSOR_TYPE DEVICE_GARAGE

bool doorUp, doorDown, startup = true;
String macID = "";
IPAddress serverIP;
unsigned long heartbeat = 0;

void setup() {

  pinMode(DOORUP, INPUT);
  pinMode(DOORDOWN, INPUT);

  doorUp = digitalRead(DOORUP);
  doorDown = digitalRead(DOORDOWN);

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
  

//  startWiFi();
  
  WiFi.mode(WIFI_STA);

  WiFiMulti.addAP("Peterson", "7758499666");
  displayInfo();

  Udp.begin(UDP_PORT);         // Start listening for UDP

}

void loop() {
  String st;
  // wait for WiFi connection

  if (!(startup || (millis() - heartbeat > 10000) || (doorDown != digitalRead(DOORDOWN)) || (doorUp != digitalRead(DOORUP)))  ) return;
  displayInfo();
  heartbeat = millis();
  doorUp = digitalRead(DOORUP);
  doorDown = digitalRead(DOORDOWN);
  startup = false;
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    WiFiClient client;
    HTTPClient http;

    Serial.print("[HTTP] begin...\n");

    st = "http://";
    st += serverIP.toString();
    st += "/set?mac=" + macID;
    st += '&';
    st += "deviceType=";
    st += SENSOR_TYPE;
    st += '&';
    st += "sensor0=";
    if (!doorUp) st+= "TRUE"; else st += "FALSE";   //The sensors are reverse logic
    st += '&';
    st += "sensor1=";
    if (!doorDown) st+= "TRUE"; else st += "FALSE";   //The sensors are reverse logic
    Serial.println(st);
    if (http.begin(client, st)) {  // HTTP
      Serial.print("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
          Serial.println(payload);
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
      findServer();
      delay(1000);
    }
  }
  handleUDP();
  delay(200);
}

void startWiFi() {
  wifiManager.autoConnect("Garage Door Sensor");
}

void displayInfo() {
  String st;
  st = "Door Up = ";
  st += digitalRead(DOORUP);
  st += "\nDoor Down = ";
  st += digitalRead(DOORDOWN);
  st += "\nMAC ID: ";
  st += macID + "\n";
  Serial.println(st);
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