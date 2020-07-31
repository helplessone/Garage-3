#define ESP8266;
#define ARDUINO 10808
#include <IotSensors.h>
#include <TimeLib.h>                   

#include <ESP8266WiFi.h>
//#include <ESP8266WiFiMulti.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <FS.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>               //For UDP 
//#include <time.h>                       // time() ctime()
//#include <sys/time.h>                   // struct timeval
//#include <coredecls.h>                  // settimeofday_cb()

//ESP8266WiFiMulti wifiMulti;       // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

ESP8266WebServer server(80);       // create a web server on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81
WiFiManager wifiManager;
WiFiUDP Udp;

File fsUploadFile;                 // a File variable to temporarily store the received file

//const char *ssid = "mynetwork"; // The name of the Wi-Fi network that will be created
//const char *password = "";   // The password required to connect to it, leave blank for an open network

const char *OTAName = "ESP8266";           // A name and a password for the OTA service
const char *OTAPassword = "wade"; //"esp8266";



#ifndef GARAGE_UDP
  #define UDP_PORT 4204
  #define MAX_UDP_SIZE 255
#endif

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

#define LED_RED     15            // specify the pins with an RGB LED connected
#define LASER       12            

#define RED 0
#define GREEN 1
#define YELLOW 2
#define GRAY 3
#define PURPLE 4

//Garage Door Position
#define DOOR_CLOSED 0
#define DOOR_OPEN 1
#define DOOR_PARTIAL 2

#define MAX_DEVICES 30
#define JSON_BUF_SIZE 2048

#define SENSOR_TIMEOUT 20000

#define STRSWITCH(STR)      char _x[16]; strcpy(_x, STR); if (false)
#define STRCASE(STR)        } else if (strcmp(_x, STR)==0){
#define STRDEFAULT          } else {

/******* SAVED GLOBAL VARIABLES **********/
int timeOffset = 0;
Device devices[MAX_DEVICES];
/*****************************************/

byte closeDelayState[MAX_DEVICES];
byte closeTimeState[MAX_DEVICES];
byte closeState[MAX_DEVICES];
unsigned long delayCloseTimer[MAX_DEVICES];
unsigned long timeCloseTimer[MAX_DEVICES];
unsigned long closeTimer[MAX_DEVICES];

const char* mdnsName = "swoop"; // Domain name for the mDNS responder

//For realtime clock
timeval tv;
struct timezone tz;
timespec tp;
time_t tnow;
extern "C" int clock_gettime(clockid_t unused, struct timespec *tp);

bool updateGui = false; //forces gui update on next loop
unsigned long heartbeatMillis = millis();
unsigned long checkCloseDelayMillis = millis();
//unsigned long checkCloseTimeMillis = millis();
unsigned long closeStateTimer = 0;
unsigned long getRealTimeMillis = millis();
unsigned long offlineTimer = millis();
unsigned long clientRefreshTimer = millis();

unsigned long testMillis = millis();
int hue = 0;

/*__________________________________________________________SETUP__________________________________________________________*/

void setup() {
  Serial.begin(115200);        // Start the Serial communication to send messages to the computer
  
  for (int i=0; i<MAX_DEVICES; i++) {  //set auto close state machines to state 0
//    closeDelayState[i] = 0;
//    closeTimeState[i] = 0;
    closeState[i] = 0;
    closeTimer[i] = 0;
  }
  delay(10);
  Serial.println("\r\n");

  Serial.println("*********** SETUP ie. reboot ************");

  clearDevices();
 
  pinMode(LASER, OUTPUT);     // set LASER pin as output
  digitalWrite(LASER, HIGH);  // turn off laser
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);    // turn off on-board LED
  
  startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  startOTA();                  // Start the OTA service
  startSPIFFS();               // Start the SPIFFS and list all contents
  startWebSocket();            // Start a WebSocket server
  startMDNS();                 // Start the mDNS responder
  startServer();               // Start a HTTP server with a file read handler and an upload handler
  Udp.begin(UDP_PORT);         // Start listening for UDP
  delay(300);
  Serial.println("Mac Address = " + WiFi.macAddress());  
  restoreSettings();
  
  configTime(0, 0, "pool.ntp.org");
  // set up TZ string to use a POSIX/gnu TZ string for local timezone
  // TZ string information: https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
  // See Arduino Examples NTP-Clock-MinExample for RealTimeClock code
//  setenv("TZ", "PST+8PDT,M3.2.0/2,M11.1.0/2", 1);
  setenv("TZ", "UTC+0UTC,M3.2.0/2,M11.1.0/2", 1);
  tzset(); // save the TZ variable
  updateGui = true;
}

/*__________________________________________________________LOOP__________________________________________________________*/

void loop() {
  unsigned long loopMillis = millis();
//  SPIFFS.remove("/devices.txt");
//  delay(500);
//  return;
  delay(0);
  
  bool alarm = false;
/*  
  if (millis() - clientRefreshTimer > 60000) {
    clientRefreshTimer = millis();
    pushJson();
    Serial.println("Refresh timer");
  }
*/  
  if (updateGui) {    
    pushJson();
    updateGui = false;
  }
    
  webSocket.loop();                           // constantly check for websocket events
  server.handleClient();                      // run the server
  
  for (int i=0; i<MAX_DEVICES; i++) {
    if (devices[i].deviceType != DEVICE_NONE){
      if (millis() - devices[i].timer > SENSOR_TIMEOUT) {
        Serial.printf("Sensor Timeout device %d\n",i);
        devices[i].online = 0;
        devices[i].timer = millis();// - SENSOR_TIMEOUT;   //keep timer from rolling over
      }
    }
  }
  
  for (int i=0; i<MAX_DEVICES; i++) {  //Determine if LASER need to be turned on
    if (devices[i].deviceType == DEVICE_GARAGE) {
      if (devices[i].sensorSwap == 0) {
        if (devices[i].sensor[0] != 1 || devices[i].sensor[1] != 0 || devices[i].online == 0) alarm = true;
      } else {
        if (devices[i].sensor[0] != 0 || devices[i].sensor[1] != 1 || devices[i].online == 0) alarm = true;        
      }
    }
    if (devices[i].deviceType == DEVICE_LATCH) {
      if (devices[i].latchValue) alarm = true;  
    }
    if (devices[i].deviceType == DEVICE_THERMOMETER) {
      if (devices[i].celcius) {
        if (devices[i].maxTemp != 0 && devices[i].temp > devices[i].maxTemp * 100) alarm = true;
        if (devices[i].minTemp != 0 && devices[i].temp < devices[i].minTemp * 100) alarm = true;
      } else {
        if (devices[i].maxTemp != 0 && devices[i].temp > (int)(round(float(devices[i].maxTemp-32) * 5 / 9 * 100))) alarm = true;
        if (devices[i].minTemp != 0 && devices[i].temp < (int)(round(float(devices[i].minTemp-32) * 5 / 9 * 100))) alarm = true;        
      }
    }  
  } 

  if (millis() - testMillis > 2000) {
    testMillis = millis();
    int mint, maxt;
    for (int i=0; i<MAX_DEVICES; i++) {
      if (devices[i].deviceType == DEVICE_THERMOMETER) {
//        displayDevice(i);
        
        if (devices[i].celcius) {
          mint = devices[i].minTemp * 100;
          maxt = devices[i].maxTemp * 100;

        } else {
          mint = (int)(float(devices[i].minTemp-32) * 5 / 9 * 100); // * (5/9)* 100;
          maxt = (int)(float(devices[i].maxTemp-32) * 5 / 9 * 100); // * (5/9)* 100;      
        }        
//        Serial.printf("Device %d, temp = %d, min = %d, max = %d, devicemin = %d, devicemax = %d\n",i,devices[i].temp,mint,maxt,devices[i].minTemp,devices[i].maxTemp);
        
      }
    }
  }

  if (alarm) digitalWrite (LASER, LOW); else digitalWrite(LASER, HIGH);
  
  if ((millis() - checkCloseDelayMillis) > 2000) {
    for (int i=0; i<MAX_DEVICES; i++) {
      if (devices[i].deviceType == DEVICE_GARAGE) {
        if (garageDoorPosition(devices[i]) == DOOR_CLOSED) {
          if (closeState[i] == 0 && closeTimer[i] != 0) Serial.printf("Device %d: DOOR CLOSED STATE %d -> 0 (Timer Cancelled)\n",i,closeState[i]);
          if (closeState[i] != 0) Serial.printf("Device %d: DOOR CLOSED STATE %d -> 0\n",i,closeState[i]);
          closeState[i] = 0;
          closeTimer[i] = 0;
          continue;     //go to next device
        }
        switch (closeState[i]) {
          case 0: { 
            if (devices[i].closeDelay > 0) {
              if (closeTimer[i] == 0) {
                closeTimer[i] = millis();
                Serial.printf("Device %d: STATE 0 (Starting %d min Timer)\n",i,devices[i].closeDelay);
              }
              if ((millis() - closeTimer[i]) > (devices[i].closeDelay * 60 * 1000)) {
                closeState[i] = 1; //start close procedure 
                closeTimer[i] = millis();
                Serial.printf("Device %d: STATE 0 -> 1 (Delay timer expired)\n",i);
                pressDoorButton(devices[i]); //press the button
                break;
              }
            }
            if (devices[i].closeTime > 0) {
              unsigned long ltime = getLocalTime();
              int localTime = hour(ltime) * 100 + minute(ltime);
              if (devices[i].closeTime == localTime) {
                closeState[i] = 1; //start close procedure 
                closeTimer[i] = millis();
                Serial.printf("Device %d: STATE 0 -> 1 (Time of day hit)\n",i);
                pressDoorButton(devices[i]); //press the button
              }
            }
          }
          break;
          case 1: { 
            if (millis() - closeTimer[i] > 20000) {
              closeState[i] = 2;
              closeTimer[i] = millis();
              Serial.printf("Device %d: STATE 1 -> 2 (Trying again...)\n",i);
              pressDoorButton(devices[i]); //press the button
            }
          }
          break;
          case 2: { //garage was open, waiting for close
            if (millis() - closeTimer[i] > 20000) {
              //if the door didn't close in 20 seconds, abort
              closeState[i] = 0;
              closeTimer[i] = 0;
              Serial.printf("Device %d: DOOR DID NOT CLOSE\n", i);
              Serial.printf("Device %d: STATE 2 -> 0\n",i);
            }                           
          }
          break;                 
        } 
      }
    }
    checkCloseDelayMillis = millis();
  }
  
  if ((millis() - heartbeatMillis) > 15000) {
    digitalWrite (LED_BUILTIN, LOW);
    if ((millis() - heartbeatMillis) > 15000+50) {
      digitalWrite (LED_BUILTIN, HIGH);
      heartbeatMillis = millis();  
    }
  }
  
  ArduinoOTA.handle();                        // listen for OTA events
  handleUDP();
  
  if ((millis() - loopMillis) > 30)
    Serial.printf("Loop took %d millis\n", millis() - loopMillis);
}

void pushJson(){
//  Serial.println("push Json");
  String st = getJsonString();
//  Serial.println (st);
  webSocket.broadcastTXT(st.c_str(), st.length());
}

int garageDoorPosition (Device device) {  //0 = closed, 1 = open, 2 = partially open
  if (device.deviceType == DEVICE_GARAGE) {
    if (device.sensor[0]==0 && device.sensor[1]==1 && device.sensorSwap==0) return 1; //door open w/o sendorSwap
    if (device.sensor[0]==1 && device.sensor[1]==0 && device.sensorSwap==1) return 1; //door open with sensorSwap
    if (device.sensor[0] == device.sensor[1]) return 2;  //if partially open (assume no illegal state)
  }
  return 0;
}

String timeToString(unsigned long longtime) {
  return (String)longtime;
}

unsigned long getTime() { //char *deviceTime) {
  gettimeofday(&tv, &tz);
  return tv.tv_sec;
}

unsigned long getLocalTime() { //char *deviceTime) {
  gettimeofday(&tv, &tz);
  return (tv.tv_sec + (timeOffset * 60 * 60));
}

int getDeviceCount (int deviceType) {
  int cnt = 0;
  for (int i=0; i<MAX_DEVICES; i++) {
    if (devices[i].deviceType == deviceType || ((deviceType == DEVICE_ANY) && (devices[i].deviceType != DEVICE_NONE))) cnt++;
  }
  return cnt;
}

String getJsonString() {
  StaticJsonDocument<JSON_BUF_SIZE> json;
  String st;
  json["global"]["currTime"] = getTime();
  json["global"]["timeOffset"] = timeOffset;
  for (int i=0; i<MAX_DEVICES; i++) {
    if (devices[i].deviceType != DEVICE_NONE) {
      json["devices"][i]["mac"] = macToString(devices[i].mac);      
      json["devices"][i]["deviceType"] = devices[i].deviceType;
      json["devices"][i]["deviceName"] = devices[i].deviceName;
      json["devices"][i]["deviceTime"] = devices[i].deviceTime;
      json["devices"][i]["ip"] = ipToString(devices[i].ip);
      json["devices"][i]["timer"] = devices[i].timer; 
      json["devices"][i]["online"] = devices[i].online;
      if (devices[i].deviceType == DEVICE_GARAGE) {
        json["devices"][i]["sensor0"] = devices[i].sensor[0];   
        json["devices"][i]["sensor1"] = devices[i].sensor[1]; 
        json["devices"][i]["sensorSwap"] = devices[i].sensorSwap;        
        json["devices"][i]["closeDelay"] = devices[i].closeDelay;        
        json["devices"][i]["closeTime"] = devices[i].closeTime;        
      }
      if (devices[i].deviceType == DEVICE_THERMOMETER) {
        json["devices"][i]["temp"] = devices[i].temp;   
        json["devices"][i]["maxTemp"] = devices[i].maxTemp; 
        json["devices"][i]["minTemp"] = devices[i].minTemp;        
        json["devices"][i]["celcius"] = devices[i].celcius;        
      }
      if (devices[i].deviceType == DEVICE_LATCH) {
        json["devices"][i]["switchValue"] = devices[i].switchValue;
        json["devices"][i]["latchValue"] = devices[i].latchValue;           
        json["devices"][i]["switchReverse"] = devices[i].switchReverse?1:0;        
        json["devices"][i]["latchTime"] = devices[i].latchTime;        
      }
    }
  }
  serializeJsonPretty(json,st);
//  Serial.println(st);
//  Serial.print ("timeOffset = ");
//  Serial.println (timeOffset);
  return st;  
}

void deleteSettings(){
  SPIFFS.remove("/devices.txt");
  Serial.println ("/devices.txt deleted");
  clearDevices();
  ESP.reset();
}

void saveSettings(){
  File f = SPIFFS.open("/devices.txt","w");
  if (!f) return;
  f.write((const char *)&(timeOffset),sizeof(timeOffset));
  f.write((const char *)&(devices),sizeof(devices));
  f.close();  
  updateGui = true;
}

void restoreSettings() {
  char buffer[sizeof(Device)];
  clearDevices();
  File f = SPIFFS.open("/devices.txt","r");
  if (!f) return;
  int i = 0;
  f.readBytes((char *)&(timeOffset),sizeof(timeOffset));
  f.readBytes((char *)&(devices),sizeof(devices));
  for (i=0; i<MAX_DEVICES; i++) {
    if (devices[i].deviceType != DEVICE_NONE) displayDevice(i);
  }
  Serial.println("---End Restore"); 
  f.close(); 
  updateGui = true;
}

void clearDevices() {
  memset(&devices, '\0', MAX_DEVICES * sizeof(Device));   //clear devices array to 0's
  for (int i=0; i<MAX_DEVICES; i++) devices[0].deviceType = DEVICE_NONE;  //and set deviceType to NONE 
} 

String deviceToString(Device device) {
  String st;
  char buffer[200];
  sprintf(buffer,"%s|%s|%s|%lu|%d|%d|%d|%d|%u|%d",macToString(device.mac).c_str(),ipToString(device.ip).c_str(),device.deviceName,device.deviceTime,device.deviceType,device.sensor[0],device.sensor[1],device.sensorSwap,device.timer,device.online);
  st = buffer; 
  return st;
}
 
/*__________________________________________________________SETUP_FUNCTIONS__________________________________________________________*/

void startWiFi() { // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  digitalWrite(LED_BUILTIN,LOW);
  wifiManager.autoConnect("Garage Door Server");
  digitalWrite(LED_BUILTIN,HIGH);
  
  if(WiFi.softAPgetStationNum() == 0) {      // If the ESP is connected to an AP
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());             // Tell us what network we're connected to
    Serial.print("IP address:\t");
    Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
  } else {                                   // If a station is connected to the ESP SoftAP
    Serial.print("Station connected to ESP8266 AP");
  }
  Serial.println("\r\n");
}

void startOTA() { // Start the OTA service
  ArduinoOTA.setHostname(OTAName);
  ArduinoOTA.setPassword(OTAPassword);

  ArduinoOTA.onStart([]() {
  Serial.println("Start");
//    digitalWrite(LED_RED, 0);    // turn off the LEDs
//    digitalWrite(LED_GREEN, 0);
//    digitalWrite(LED_BLUE, 0);
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

void startSPIFFS() { // Start the SPIFFS and list all contents
  SPIFFS.begin();                             // Start the SPI Flash File System (SPIFFS)
  Serial.println("SPIFFS started. Contents:");
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {                      // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void startMDNS() { // Start the mDNS responder
  MDNS.begin(mdnsName);                        // start the multicast domain name server
  Serial.print("mDNS responder started: http://");
  Serial.print(mdnsName);
  Serial.println(".local");
}

void updateDevice(String data) {  // This routine invoked by '^' from javascript 
  bool save = false;
  Serial.println(data);
  StaticJsonDocument<JSON_BUF_SIZE/4> doc;
  deserializeJson(doc,data.c_str());
  int i = 0;
  for (JsonPair keyValue : doc.as<JsonObject>()) {
    delay(0);      
    int val = keyValue.value();
    String st = keyValue.key().c_str();
    if (st == "timeOffset") {
      Serial.printf ("timeOffset = %d\n",(int)keyValue.value());
      timeOffset = (int)keyValue.value();
      save = true;
      updateGui = true;      
    }
    i++;
  } 
  if (doc.containsKey("mac")) { //if mac included, update device
    bool newDevice;
    int deviceIndex = getDeviceIndex(doc["mac"], &newDevice);
    devices[deviceIndex].timer = millis();
    devices[deviceIndex].online = 1;
    for (int j=0; j<4;j++) devices[deviceIndex].ip[j] = server.client().remoteIP()[j];
    for (JsonPair keyValue : doc.as<JsonObject>()) {
      delay(0);      
      int val = keyValue.value();
      STRSWITCH(keyValue.key().c_str())
      {
        STRCASE("deviceType")
          if ((int)devices[deviceIndex].deviceType != (int)keyValue.value()) {
            devices[deviceIndex].deviceType = keyValue.value();
            save = true;
            updateGui = true;
          }
        STRCASE("sensor0")
          if ((int)devices[deviceIndex].sensor[0] != (int)keyValue.value()) {
            devices[deviceIndex].sensor[0] = keyValue.value();
            devices[deviceIndex].deviceTime = getTime();
            save = true;
            updateGui = true;
         }
        STRCASE("sensor1")
          if ((int)devices[deviceIndex].sensor[1] != (int)keyValue.value()) {
            devices[deviceIndex].sensor[1] = keyValue.value();
            devices[deviceIndex].deviceTime = getTime(); 
            save = true;
            updateGui = true;
          }
        STRCASE("sensorSwap")
          if ((int)devices[deviceIndex].sensorSwap != (int)keyValue.value()) {
            devices[deviceIndex].sensorSwap = keyValue.value();
            Serial.println("sensorSwap 0");           
            updateGui = true;
          }      
        STRCASE("deviceName")
          String st = keyValue.value();    
          st.toCharArray(devices[deviceIndex].deviceName, sizeof(devices[deviceIndex].deviceName));       
          save = true;
        STRCASE("temp")
          devices[deviceIndex].temp = (int)keyValue.value();
          devices[deviceIndex].deviceTime = getTime();                   
          updateGui = true;
        STRCASE("minTemp")
          devices[deviceIndex].minTemp = (int)keyValue.value();
          save = true;
        STRCASE("maxTemp")
          devices[deviceIndex].maxTemp = (int)keyValue.value();
          save = true;
        STRCASE("celcius")
          devices[deviceIndex].celcius = (int)keyValue.value();
           save = true;
        STRCASE("closeDelay")
           devices[deviceIndex].closeDelay = (int)keyValue.value();
           save = true;
        STRCASE("closeTime")
           devices[deviceIndex].closeTime = (int)keyValue.value();
           save = true;
/*           
        STRCASE("switchValue") {
          Serial.println("updateDevice switchValue");
          if ((int)devices[deviceIndex].switchValue != (int)keyValue.value()) {
            devices[deviceIndex].switchValue = keyValue.value();
            devices[deviceIndex].deviceTime = getTime();
            updateGui = true;
          }
        }   
*/
        STRCASE("switchReverse") {
          devices[deviceIndex].switchReverse = !devices[deviceIndex].switchReverse;
          devices[deviceIndex].switchValue = !devices[deviceIndex].switchValue;
          devices[deviceIndex].latchValue = !devices[deviceIndex].latchValue;
          Serial.printf("switchReverse = %d\n",devices[deviceIndex].switchReverse?1:0);
          updateGui = true;
        }
      }
    }
  }
  if (save) saveSettings();
}

void startServer() { // Start a HTTP server with a file read handler and an upload handler
  
  server.on("/edit.html",  HTTP_POST, []() {  // If a POST request is sent to the /edit.html address,
    server.send(200, "text/plain", ""); 
  }, handleFileUpload);                       // go to 'handleFileUpload'

  server.on("/var.json",  HTTP_GET, []() {  // If a GET request is sent to the /var.json address,
    Serial.println("HTTP_GET /var.json");
    Serial.println(getJsonString());
    server.send(200, "application/json", getJsonString()); 
  });                                     // go to 'handleFileUpload'
  
  //server.on("/set", handleSet);  // GET request "set"
  
  server.on("/var.json", HTTP_POST, []() {
 //   Serial.println("\nPost!!");
    bool save = false;
    if (!server.hasArg("plain")){
      server.send(200, "text/plain", "No body");
      Serial.print("Bad Request");
      return;
    }
    StaticJsonDocument<JSON_BUF_SIZE/4> doc;
    {
      String data = server.arg("plain");
      deserializeJson(doc,data);
      Serial.print ("Data = ");
      Serial.println(data);
    }
    if (!doc.containsKey("mac")) {
      server.send(400,"text/plain", "Malformed Request, No MAC Address");
      return;
    } 
    bool newDevice;
    int deviceIndex = getDeviceIndex(doc["mac"], &newDevice);
    devices[deviceIndex].timer = millis();
//    Serial.println("Online set to 1");
    devices[deviceIndex].online = 1;
    for (int j=0; j<4;j++)
      devices[deviceIndex].ip[j] = server.client().remoteIP()[j];      // **** LOGAN - what's this for????
//    Serial.println("Json");
    for (JsonPair keyValue : doc.as<JsonObject>()) {
      delay(0);
      
      int val = keyValue.value();
      STRSWITCH(keyValue.key().c_str())
      {
        STRCASE("deviceType")
        if ((int)devices[deviceIndex].deviceType != (int)keyValue.value()) {
           devices[deviceIndex].deviceType = keyValue.value();
           Serial.println("deviceType 1");           
           updateGui = true;
        }
        STRCASE("sensor0")
        if ((int)devices[deviceIndex].sensor[0] != (int)keyValue.value()) {
          devices[deviceIndex].sensor[0] = keyValue.value();
          devices[deviceIndex].deviceTime = getTime();
          save = true;         
          updateGui = true;
        }
        STRCASE("sensor1")
        if ((int)devices[deviceIndex].sensor[1] != (int)keyValue.value()) {
          devices[deviceIndex].sensor[1] = keyValue.value();
          devices[deviceIndex].deviceTime = getTime(); 
          save = true;         
          updateGui = true;
        }   
        STRCASE("sensorSwap")
        if ((int)devices[deviceIndex].sensorSwap != (int)keyValue.value()) {
          devices[deviceIndex].sensorSwap = keyValue.value();
          Serial.println("sensorSwap 0");           
          updateGui = true;
        }  
        STRCASE("temp") {
          devices[deviceIndex].temp = (int)keyValue.value();
          devices[deviceIndex].deviceTime = getTime();
          updateGui = true;
        }
        STRCASE("switchValue") {
          bool switchVal = (bool)keyValue.value();
          if (devices[deviceIndex].switchReverse) switchVal = !switchVal;          
//          Serial.printf("switchVal %d, switchReverse %d\n", switchVal?1:0, devices[deviceIndex].switchReverse?1:0);
          if (devices[deviceIndex].switchValue != switchVal) devices[deviceIndex].deviceTime = getTime();
          devices[deviceIndex].switchValue = switchVal;
          if (devices[deviceIndex].switchValue && !devices[deviceIndex].latchValue) {
            Serial.printf ("Latch %d set\n",deviceIndex);
            devices[deviceIndex].latchValue = true;
            devices[deviceIndex].latchTime = getTime();
          }
          save = true;
          updateGui = true;
        }
      }     
    }
    if (save) saveSettings();
    server.send(200,"text/plain","ok");
//    Serial.println("POST Ok");
  });
    
  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
                                              // and check if the file exists

  server.begin();                             // start the HTTP server
  Serial.println("HTTP server started.");
}

/*__________________________________________________________SERVER_HANDLERS__________________________________________________________*/

void parseString(const char* str, char sep, byte* bytes, int maxBytes, int base) {
  for (int i = 0; i < maxBytes; i++) {
    bytes[i] = strtoul(str, NULL, base);  // Convert byte
    str = strchr(str, sep);               // Find next separator
    if (str == NULL || *str == '\0') {
      break;                            // No more separators, exit
    }
    str++;                                // Point to next character after separator
  }
}

int getAvailableDevice() {
  for (int i = 0; i<MAX_DEVICES; i++) {
    if (devices[i].deviceType == DEVICE_NONE) return i;
  }
  return -1;  
}

int deviceCount () {
  int count = 0;
  for (int i=0; i<MAX_DEVICES; i++) if (devices[i].deviceType != DEVICE_NONE) count++;
  return count;
}

String macToString (byte macID[]) {
  String st;
  char buf[20];
  sprintf(buf,"%x:%x:%x:%x:%x:%x", macID[0], macID[1], macID[2], macID[3], macID[4], macID[5]);
  st = buf;
  return st;
}

String ipToString(byte ip[])
{
  String st;
  char buf[20];
  sprintf(buf,"%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  st = buf;
  return st;
}

int getDeviceIndex (String macString, bool *newDevice) {
  int deviceIndex;
  int DEVICE;
  byte macID[6];
  char buf[25];
  bool found = false;
  
  *newDevice = false;
  
  if (macString == "") return -1;
  parseString(macString.c_str(), ':', macID, 6, 16);
  for (int deviceIndex=0; deviceIndex<MAX_DEVICES; deviceIndex++) {
    if (memcmp(devices[deviceIndex].mac,macID,sizeof(macID)) == 0) {
      DEVICE = deviceIndex;
      found = true;
      break;
    } 
  }
  if (!found) {
    DEVICE = getAvailableDevice();
    if (DEVICE < 0) {
      for (int i=0; i<10; i++) Serial.println ("******** EXCEEDED MAX DEVICES ********");
      delay(5000);
      return -1;      
    }
    memcpy(devices[DEVICE].mac,macID,sizeof(macID));
    *newDevice = true;
    Serial.print("******* NEW DEVICE ADDED ******** deviceIndex = ");
    Serial.println (DEVICE);
  } 
  return DEVICE; 
}


void displayDevice (int deviceIndex) {
  if (deviceIndex < 0 || deviceIndex >= MAX_DEVICES) {
    Serial.print("**** Invalid deviceIndex **** deviceIndex = ");
    Serial.println (deviceIndex);
    return;
  }
  if (devices[deviceIndex].deviceType == DEVICE_NONE) return;
  Serial.print("deviceIndex: ");
  Serial.println (deviceIndex);
  Serial.println("mac: " + macToString(devices[deviceIndex].mac));
  Serial.print("deviceName: ");
  Serial.println(devices[deviceIndex].deviceName);
  Serial.print("deviceTime: ");
  Serial.println(timeToString(devices[deviceIndex].deviceTime));
  Serial.println("IP: " + ipToString(devices[deviceIndex].ip));
  Serial.print("deviceType: ");
  Serial.println(devices[deviceIndex].deviceType);
  Serial.print("timer: ");
  Serial.println(devices[deviceIndex].timer);  
  Serial.print("online: ");
  Serial.println(devices[deviceIndex].online);  
  if (devices[deviceIndex].deviceType == DEVICE_GARAGE){
    Serial.print("sensor[0]: ");
    Serial.println(devices[deviceIndex].sensor[0]);
    Serial.print("sensor[1]: ");
    Serial.println(devices[deviceIndex].sensor[1]);
    Serial.print("sensorSwap: ");
    Serial.println(devices[deviceIndex].sensorSwap);    
    Serial.print ("closeDelay: ");
    Serial.println (devices[deviceIndex].closeDelay); 
    Serial.print ("closeTime: ");
    Serial.println (devices[deviceIndex].closeTime);  
  }
  if (devices[deviceIndex].deviceType == DEVICE_THERMOMETER){
    Serial.print ("temp: ");
    Serial.println (devices[deviceIndex].temp);    
    Serial.print ("minTemp: ");
    Serial.println (devices[deviceIndex].minTemp); 
    Serial.print ("maxTemp: ");
    Serial.println (devices[deviceIndex].maxTemp);  
    Serial.print ("celcius: ");
    Serial.println (devices[deviceIndex].celcius);   
  }
  Serial.println("****************************");
}

void swapDevices(int dev1, int dev2) {
  Device tempDevice;
  memcpy(&tempDevice,&devices[dev1],sizeof(Device));
  memcpy(&devices[dev1], &devices[dev2], sizeof(Device));
  memcpy(&devices[dev2], &tempDevice, sizeof(Device));
  saveSettings();
}

void deleteDevice(int index) {
  Device device;
  for (int i=index; i<MAX_DEVICES-2; i++) {
    memcpy(&(devices[i]),&(devices[i+1]),sizeof(Device));  
  }
  devices[MAX_DEVICES-1].deviceType = DEVICE_NONE;
  saveSettings();
}

int setDevice (String mac, String var, String val) {
//Set variable (var) in device array for sensor (mac) to (val)
//if (mac) doesn't exist in devices, it is added and (var) is set to (val)  
  bool newDevice;
  int deviceIndex = getDeviceIndex(mac, &newDevice);
  if (deviceIndex > -1) {
    if (var == "deviceName") {
      int str_len = val.length() + 1;
      if (str_len > 15) str_len = 15;
      char char_array[16];
      val.toCharArray(char_array, str_len);
      memcpy(devices[deviceIndex].deviceName, char_array, str_len);
      return 0;        
    }
    if (var == "sensorSwap") {
      Serial.print("setDevice:sensorSwap = ");
      Serial.println(val);
      if (val == "1") devices[deviceIndex].sensorSwap = 1; else devices[deviceIndex].sensorSwap = 0;
      saveSettings();
      return 0;        
    }
    if (var == "moveDevice") {
      if (val == "up") {
        if (deviceIndex != 0) swapDevices(deviceIndex, deviceIndex-1);
      }
      if (val == "down") {
        if (deviceIndex < getDeviceCount(DEVICE_ANY)-1) {
          swapDevices(deviceIndex, deviceIndex+1);
        }
      } 
    }
    if (var == "deleteDevice") { 
      if (devices[deviceIndex].online == 0) deleteDevice(deviceIndex);
    }    
  }
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
    updateGui = true;               // and force Gui update
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

void handleFileUpload(){ // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  String path;
  if(upload.status == UPLOAD_FILE_START){
    path = upload.filename;
    if(!path.startsWith("/")) path = "/"+path;
    if(!path.endsWith(".gz")) {                          // The file server always prefers a compressed version of a file 
      String pathWithGz = path+".gz";                    // So if an uploaded file is not compressed, the existing compressed
      if(SPIFFS.exists(pathWithGz))                      // version of that file must be deleted (if it exists)
         SPIFFS.remove(pathWithGz);
    }
    Serial.print("handleFileUpload Name: "); Serial.println(path);
    fsUploadFile = SPIFFS.open(path, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    path = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println((int)upload.totalSize);
      server.sendHeader("Location","/success.html");      // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

int parseCommand(String command, String *mac, String *var, String *val) {
  //command format: "*mac?var=val" - the * can be any character, but not the ? and = characters
  *mac = command.substring(1,command.indexOf('?'));
  *var = command.substring(command.indexOf('?')+1,command.indexOf('='));
  *val = command.substring(command.indexOf('=')+1,command.length());
}

void pressDoorButton(Device device){
  Serial.println ("Sending press door button");
  IPAddress IP = device.ip;
  HTTPClient http;
  http.begin("http://"+IP.toString()+"/");
  http.POST(ACTION_MESSAGE);    //Action message to device  
}

// When a webSocketEvent message is from javascript
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { 
  Serial.printf("WSevent [%d] : %d\n",num,type);
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        updateGui = true;
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT: {                    // if new text data is received
//      Serial.printf("[%u] get Text: %s\n", num, payload);
      switch(payload[0]) {
        case '#': {     // button was pressed sent by javascript       
          Serial.print ("Key Pressed: ");
          String st = (char *)payload;
          bool newDevice;
          Serial.println(st.substring(1,st.length()));
          int index = getDeviceIndex(st.substring(1,st.length()),&newDevice);
          if (!newDevice) {
            pressDoorButton(devices[index]);
            if (devices[index].deviceType == DEVICE_LATCH) {
              devices[index].latchValue = 0;
              Serial.printf ("Latch %d cleared\n",index);
              updateGui = true;
            }
          }
        }
        break;
        // used for misc commands: deleteDevice, moveDevice   
        case '*': {     //update variable, command format: http://url/mac:var=val
          String mac="", var="", val="";
          String command = (char *)payload;
          parseCommand(command, &mac, &var, &val);
          setDevice(mac, var, val);                        
        }
        break;              
        case '@': {     //used to increment & decrement time zone
          switch (payload[1]) {
            case '+': { timeOffset++; Serial.println("time inc"); } break;
            case '-': { timeOffset--; Serial.println("time dec"); } break;
          }                       
        }
        break;                
        case '^': {     //used to update device with json string
          String json = (char *)payload;
          json.remove(0,1);                   
          updateDevice(json);
        }
        break; 
        case '!': {  //request json update
          updateGui = true;        
        }
        break;
        default: {
          switch (payload[0]) {
            case 'S': {
              saveSettings();
            }
            break;
            case 'D': {
              deleteSettings();
            }
            break;
          } //end switch payload[0]
        } //default
      } //end switch(payload[0]
    }  //end WStype_TEXT
  }  //end switch(Type)
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
  
  if (strcmp(incomingPacket, LINK_MESSAGE)==0) { // Form a link to sendHeader
    //TODO register device
    Udp.beginPacket(Udp.remoteIP(),UDP_PORT);
    Udp.write(LINKED_MESSAGE);
    Udp.endPacket();
  }

}
/*__________________________________________________________HELPER_FUNCTIONS__________________________________________________________*/

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String((long)bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}
