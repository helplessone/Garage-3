#define ESP8266;
#define ARDUINO 10808
#include <IotSensors.h>

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
#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
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
const char *OTAPassword = "esp8266";
const char *month[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

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

#define MAX_DEVICES 30
#define JSON_BUF_SIZE 2048

#define SENSOR_TIMEOUT 20000



Device devices[MAX_DEVICES];

const char* mdnsName = "swoop"; // Domain name for the mDNS responder

//For realtime clock
timeval tv;
struct timezone tz;
timespec tp;
time_t tnow;
extern "C" int clock_gettime(clockid_t unused, struct timespec *tp);

unsigned long heartbeatMillis = millis();
unsigned long getRealTimeMillis = millis();
unsigned long offlineTimer = millis();
int hue = 0;

/*__________________________________________________________SETUP__________________________________________________________*/

void setup() {
  Serial.begin(115200);        // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println("\r\n");

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
  setenv("TZ", "PST+8PDT,M3.2.0/2,M11.1.0/2", 1);
  tzset(); // save the TZ variable
}

/*__________________________________________________________LOOP__________________________________________________________*/



void loop() {
  
  bool alarm = false;
   
  webSocket.loop();                           // constantly check for websocket events
  server.handleClient();                      // run the server
  
//  char deviceTime[13];

  unsigned long deviceTime;
  if (millis() - getRealTimeMillis > 2000) {
    getRealTimeMillis = millis();
    deviceTime = getTime(); //((char *)&deviceTime);
    Serial.print ("Device Time = ");
    Serial.println(deviceTime);
  }
  
  for (int i=0; i<MAX_DEVICES; i++) {
    if (devices[i].deviceType != DEVICE_NONE){
      if (millis() - devices[i].timer > SENSOR_TIMEOUT) {
        devices[i].online = 0;
        devices[i].timer = millis() - SENSOR_TIMEOUT;   //keep timer from rolling over
      }
    }
  }
  for (int i=0; i<MAX_DEVICES; i++) {
    if (devices[i].deviceType == DEVICE_GARAGE) {
      if (devices[i].sensorSwap == 0) {
        if (devices[i].sensor[0] != 1 || devices[i].sensor[1] != 0 || devices[i].online == 0) alarm = true;
      } else {
        if (devices[i].sensor[0] != 0 || devices[i].sensor[1] != 1 || devices[i].online == 0) alarm = true;        
      }
    }
  } 
  if (alarm) digitalWrite (LASER, LOW); else digitalWrite(LASER, HIGH);
  
  if (millis() - heartbeatMillis > 20000) {
    digitalWrite (LED_BUILTIN, LOW);
    delay (50);
    digitalWrite (LED_BUILTIN, HIGH);
    heartbeatMillis = millis();  
  }
  
  ArduinoOTA.handle();                        // listen for OTA events
  handleUDP();
}

String timeToString(unsigned long longtime) {
  char buf[20];
  String am = "am";

  int hour = longtime >> 6 & 0x1f;
  if (hour > 12) {
    hour = hour - 12;
    am = "pm";
  }

  sprintf (buf,"%d/%d/%d %d:%02d",(longtime >> 16) & 0xf,(longtime >> 11) & 0x1f,(longtime >> 20 & 0x1f),hour,longtime & 0x3f);
  String st = buf + am;
  return st;
/* 
   this code needs fixing...
  String st = month[(longtime >> 16) & 0xf]
  Serial.print ("year = ");
  Serial.print ((longtime >> 20) + 2000);
  Serial.print (" month = ");
  Serial.print ((longtime >> 16) & 0xf); 
  Serial.print (" day = ");
  Serial.print ((longtime >> 11) & 0x1f);  
  Serial.print (" hour = ");
  Serial.print (longtime >> 6 & 0x1f);  
  Serial.print (" min = ");
  Serial.println (longtime & 0x3f);  
*/
}

unsigned long timeToInt(struct tm *currtime){
  return (((currtime->tm_year)-100) << 20) + ((currtime->tm_mon+1) << 16) + (currtime->tm_mday << 11) + (currtime->tm_hour << 6) + currtime->tm_min; 
}

unsigned long getTime() { //char *deviceTime) {
  unsigned long longtime;
  struct tm *currtime;
  
  gettimeofday(&tv, &tz);
  clock_gettime(0, &tp); // also supported by esp8266 code
  tnow = time(nullptr);
  currtime = localtime(&tnow);
//  sprintf (deviceTime,"%s %d %d:%02d",month[currtime->tm_mon],currtime->tm_mday,currtime->tm_hour,currtime->tm_min);
  longtime = (((currtime->tm_year)-100) << 20) + ((currtime->tm_mon+1) << 16) + (currtime->tm_mday << 11) + (currtime->tm_hour << 6) + currtime->tm_min;

  Serial.print ("longtime = ");
  Serial.println (longtime);
  
  Serial.print ("year = ");
  Serial.print (currtime->tm_year);
  Serial.print (" month = ");
  Serial.print (currtime->tm_mon+1); 
  Serial.print (" day = ");
  Serial.print (currtime->tm_mday);  
  Serial.print (" hour = ");
  Serial.print (currtime->tm_hour);
  Serial.print (" min = ");
  Serial.println (currtime->tm_min); 

  
  Serial.print ("year = ");
  Serial.print ((longtime >> 20) & 0x1f + 2000);
  Serial.print (" month = ");
  Serial.print ((longtime >> 16) & 0xf); 
  Serial.print (" day = ");
  Serial.print ((longtime >> 11) & 0x1f);  
  Serial.print (" hour = ");
  Serial.print (longtime >> 6 & 0x1f);  
  Serial.print (" min = ");
  Serial.println (longtime & 0x3f);

  return longtime;
}


int getDeviceCount (int deviceType) {
  int cnt = 0;
  for (int i=0; i<MAX_DEVICES; i++) {
    if (devices[i].deviceType == deviceType || ((deviceType == DEVICE_ANY) && (devices[i].deviceType != DEVICE_NONE))) cnt++;
  }
  return cnt;
}

String getJSONString() {
  StaticJsonDocument<JSON_BUF_SIZE> doc;
  String st;
  for (int i=0; i<MAX_DEVICES; i++) {
    if (devices[i].deviceType != DEVICE_NONE) {
      doc["devices"][i]["mac"] = macToString(devices[i].mac);      
      doc["devices"][i]["deviceType"] = devices[i].deviceType;
      doc["devices"][i]["deviceName"] = devices[i].deviceName;
      doc["devices"][i]["deviceTime"] = devices[i].deviceTime;
      doc["devices"][i]["ip"] = ipToString(devices[i].ip);
      doc["devices"][i]["sensor0"] = devices[i].sensor[0];   
      doc["devices"][i]["sensor1"] = devices[i].sensor[1]; 
      doc["devices"][i]["sensorSwap"] = devices[i].sensorSwap;
      doc["devices"][i]["timer"] = devices[i].timer; 
      doc["devices"][i]["online"] = devices[i].online; 
    } 
  }
  serializeJson(doc,st);
  return st;  
}

void clearDevices() {
  memset(&devices, '\0', MAX_DEVICES * sizeof(Device));   //clear devices array to 0's
  for (int i=0; i<MAX_DEVICES; i++) devices[0].deviceType = DEVICE_NONE;  //and set deviceType to NONE 
} 

void saveSettings() {
  String st;
  File f = SPIFFS.open("/devices.txt","w");
  if (!f) return;
  for (int i=0; i<MAX_DEVICES; i++) {
    if (devices[i].deviceType != DEVICE_NONE) {
      st = deviceToString(devices[i]);
      Serial.print (i);
      Serial.println (": " + st);
      f.println(st);
    }
  }
  f.close();
}

void restoreSettings() {
  Device device;
  String field;
  int deviceIndex = 0;
  byte mac[6];
  char buffer[200];
  String st;
  File f = SPIFFS.open("/devices.txt","r");
  if (!f) return;
  clearDevices();
  Serial.println("\n--------- restoreSettings() ------------");
  while (f.available() && (deviceIndex < MAX_DEVICES)) {
    int cnt = f.readBytesUntil('\n',buffer,sizeof(buffer));
    buffer[cnt] = 0;
    st = buffer;

    Serial.println(buffer);
    
    field = st.substring(0,st.indexOf('|'));
    Serial.println ("mac field = " + field);  
    parseString(field.c_str(), ':', device.mac, 6, 16); // Get mac address
    Serial.printf("%x:%x:%x:%x:%x:%x\n",device.mac[0],device.mac[1],device.mac[2],device.mac[3],device.mac[4],device.mac[5]);
    st.remove(0,st.indexOf('|')+1); //remove mac

    Serial.println("after remove: " + st);

    field = st.substring(0,st.indexOf('|'));  //get ip    
    Serial.println ("ip field = " + field);  
    parseString(field.c_str(), '.', device.ip, 4, 10); // Get mac address
    Serial.printf("Device ip: %d.%d.%d.%d\n",device.ip[0],device.ip[1],device.ip[2],device.ip[3]);
    st.remove(0,st.indexOf('|')+1); //remove ip

    field = st.substring(0,st.indexOf('|'));  //get name
    Serial.println ("name field = |" + field + "|" + " length = " + field.length());
    if (field.length() == 0) memset(device.deviceName,'\0',sizeof(device.deviceName));
    else sscanf(field.c_str(),"%15s",device.deviceName);
    Serial.printf("deviceName = |%s|\n",device.deviceName);
    Serial.print ("Name length = ");
    Serial.println ((int)strlen(device.deviceName));
    st.remove(0,st.indexOf('|')+1); //remove name
    Serial.println("After Remove Name: " + st);

    sscanf(st.c_str(),"%lu|%d|%d|%d|%d|%lu|%d",&device.deviceTime,&device.deviceType,&device.sensor[0],&device.sensor[1],&device.sensorSwap,&device.timer,&device.online);
    Serial.printf("Restored Device: |%s|%s|%s|%lu|%d|%d|%d|%d|%lu|%d|\n",macToString(device.mac).c_str(),ipToString(device.ip).c_str(),device.deviceName,device.deviceTime,device.deviceType,device.sensor[0],device.sensor[1],device.sensorSwap,device.timer,device.online);
    memcpy(&(devices[deviceIndex]),&device,sizeof(Device));
    deviceIndex++;    
    Serial.println("---------------------");    
  }
  Serial.println("\n----------- end restoreSettings() ----------");    
f.close();
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
  wifiManager.autoConnect("Garage Door Server");
  
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

void startServer() { // Start a HTTP server with a file read handler and an upload handler

  server.on("/edit.html",  HTTP_POST, []() {  // If a POST request is sent to the /edit.html address,
    server.send(200, "text/plain", ""); 
  }, handleFileUpload);                       // go to 'handleFileUpload'

  server.on("/var.json",  HTTP_GET, []() {  // If a GET request is sent to the /var.json address,
    server.send(200, "application/json", getJSONString()); 
  });                                     // go to 'handleFileUpload'
  
  server.on("/set", handleSet);  // GET request "set"
    
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

int trueFalse(String st) {
  if (st == "TRUE") return 1;
  if (st == "FALSE") return 0;
  Serial.println ("**** Error: trueFALSE neither TRUE or FALSE ******");
  return 0;
}

void displayDevice (int deviceIndex) {
  if (deviceIndex < 0 || deviceIndex >= MAX_DEVICES) {
    Serial.print("**** Invalid deviceIndex **** deviceIndex = ");
    Serial.println (deviceIndex);
    return;
  }
  if (devices[deviceIndex].deviceType == DEVICE_NONE) return;
  Serial.println("****************************");
  Serial.print("Device Index: ");
  Serial.println (deviceIndex);
  Serial.println("mac: " + macToString(devices[deviceIndex].mac));
  Serial.print("Device Name: ");
  Serial.println(devices[deviceIndex].deviceName);
  Serial.print("Device Time: ");
  Serial.println(timeToString(devices[deviceIndex].deviceTime));
  Serial.println("IP: " + ipToString(devices[deviceIndex].ip));
  Serial.print("deviceType: ");
  Serial.println(devices[deviceIndex].deviceType);
  Serial.print("sensor 1: ");
  Serial.println(devices[deviceIndex].sensor[0]);
  Serial.print("sensor 2: ");
  Serial.println(devices[deviceIndex].sensor[1]);
  Serial.print("sensorSwap: ");
  Serial.println(devices[deviceIndex].sensorSwap);
  Serial.print("timer: ");
  Serial.println(devices[deviceIndex].timer);  
  Serial.print("online: ");
  Serial.println(devices[deviceIndex].online);  
  Serial.println("****************************");  
}

void swapDevices(int dev1, int dev2) {
  Device tempDevice;
  memcpy(&tempDevice,&devices[dev1],sizeof(Device));
  memcpy(&devices[dev1], &devices[dev2], sizeof(Device));
  memcpy(&devices[dev2], &tempDevice, sizeof(Device));
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
      if (val == "TRUE") devices[deviceIndex].sensorSwap = 1; else devices[deviceIndex].sensorSwap = 0;
      saveSettings();
      return 0;        
    }
    if (var == "moveDevice") {
      if (val == "up") {

        if (deviceIndex != 0) {
          Serial.print("Move up: ");
          Serial.print(deviceIndex);
          Serial.print(" ");
          Serial.println(deviceIndex-1);        
          swapDevices(deviceIndex, deviceIndex-1);
        }
      }
      if (val == "down") {
        if (deviceIndex < getDeviceCount(DEVICE_ANY)-1) {
          Serial.print("Move down: ");
          Serial.print(deviceIndex);
          Serial.print(" ");
          Serial.println(deviceIndex+1);         
          Serial.println(getDeviceCount(DEVICE_ANY));
          swapDevices(deviceIndex, deviceIndex+1);
        }
      } 
    }
    if (var == "deleteDevice") {
      Serial.print ("deleteDevice online =");
      Serial.println (devices[deviceIndex].online);
   
      if (devices[deviceIndex].online == 0) devices[deviceIndex].deviceType = DEVICE_NONE;
      saveSettings();
      restoreSettings();     
    }    
  }
}

void handleSet () {
  String st;
  String Up = "", Down = "";
  String macString = "";
  String ipString = "";
  int deviceIndex = -1;
  bool newDevice;
  char deviceName[16];

  String message = "Number of args received:";
  message += server.args();
  message += "\n";
//  Serial.println(message);
//  Serial.println(server.uri());
  if (server.argName(0) == "save") {
    saveSettings();
    server.send(200, "text/plain", "");
    return;

  }
  if (server.argName(0) == "restore") {
    restoreSettings();
    server.send(200, "text/plain", "");
    return;    
  }
  for (int i=0; i<server.args(); i++) {
    message += "Arg " + (String)i + " -> ";
    message += server.argName(i) + ": ";
    message += server.arg(i) + "\n"; 

    if (server.argName(i) == "mac") {
      macString = server.arg(i); 
      deviceIndex = getDeviceIndex(macString, &newDevice);
      if (newDevice) {
        Serial.println ("NEW DEVICE!!!!!");
      }
      devices[deviceIndex].timer = millis();
      devices[deviceIndex].online = 1;
      for (int j=0; j<4;j++)
        devices[deviceIndex].ip[j] = server.client().remoteIP()[j];
    }  
    if (deviceIndex > -1) {
      if (server.argName(i) == "deviceType") {
        String st = server.arg(i);
        devices[deviceIndex].deviceType = st.toInt();
      }
      if (server.argName(i) == "sensor0") {
        if (devices[deviceIndex].sensor[0] != trueFalse(server.arg(i))) {
          devices[deviceIndex].deviceTime = getTime();         
          devices[deviceIndex].sensor[0] = trueFalse(server.arg(i));
          saveSettings();
        }
      }
      if (server.argName(i) == "sensor1") {
        if (devices[deviceIndex].sensor[1] != trueFalse(server.arg(i))) {
          devices[deviceIndex].deviceTime = getTime();         
          devices[deviceIndex].sensor[1] = trueFalse(server.arg(i));
          saveSettings();
        }       
      }
      if (server.argName(i) == "deviceTime") {
        Serial.println ("****** ERROR handleSet 'deviceTime' ************");
/*        
        int str_len = server.arg(i).length() + 1;
        if (str_len > 12) str_len = 12;
        char char_array[13];
        server.arg(i).toCharArray(char_array, str_len);
        memcpy(devices[deviceIndex].deviceTime, char_array, str_len);
*/       
      }
      if (server.argName(i) == "deviceName") {
        int str_len = server.arg(i).length() + 1;
        if (str_len > 15) str_len = 15;
        char char_array[16];
        server.arg(i).toCharArray(char_array, str_len);
        memcpy(devices[deviceIndex].deviceName, char_array, str_len);
      }
      if (server.argName(i) == "ip") {
        String ipstr = server.arg(i);
        byte ip[4];
        parseString(ipstr.c_str(), '.', ip, 4, 10); // Get mac address
        memcpy(devices[deviceIndex].ip,ip,4);
      }
    }
  } //end for
 
  server.send(200, "text/plain", getJSONString());
  
  if (newDevice && deviceIndex > -1) displayDevice(deviceIndex);
  for (int i = 0; i<getDeviceCount(DEVICE_ANY); i++) displayDevice(i);
}

void handleNotFound(){ // if the requested file or page doesn't exist, return a 404 not found error
  if(!handleFileRead(server.uri())){          // check if the file exists in the flash memory (SPIFFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found");
  }
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
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


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT: {                    // if new text data is received
//      Serial.printf("[%u] get Text: %s\n", num, payload);
      switch(payload[0]) {
        case '#': {     // button was pressed        
          Serial.print ("Key Pressed: ");
          String st = (char *)payload;
          bool newDevice;
          Serial.println(st.substring(1,st.length()));
          int index = getDeviceIndex(st.substring(1,st.length()),&newDevice);
          if (!newDevice) {
            IPAddress IP = devices[index].ip;
            HTTPClient http;
            http.begin("http://"+IP.toString()+"/");
            http.POST(ACTION_MESSAGE);
          }
        }
        break;
        case '*': {     //update variable
          String mac="", var="", val="";
          String command = (char *)payload;
          parseCommand(command, &mac, &var, &val);
          Serial.print("parseCommand:");
          Serial.println((char *)payload);
          Serial.println (mac + " " + var + " " + val);
          setDevice(mac, var, val);                        
        }
        break;
        default: {
          switch (payload[0]) {
            case '1': {                      // Door 1 pressed
                Serial.println("Door 1 pressed");             
  //            webSocket.send(JSON.ToString());
            }
            break;
            case '2': {                      // Door 2 pressed
              Serial.println("Door 2 pressed");
            }          
            break;
            case '3': {                      // Door 3 pressed
              Serial.println("Door 3 pressed");
            }
            break;
            case 'S': {
                saveSettings();
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
