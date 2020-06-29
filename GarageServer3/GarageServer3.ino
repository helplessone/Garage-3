#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <FS.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

//ESP8266WiFiMulti wifiMulti;       // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

ESP8266WebServer server(80);       // create a web server on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81
WiFiManager wifiManager;

File fsUploadFile;                 // a File variable to temporarily store the received file

const char *ssid = "mynetwork"; // The name of the Wi-Fi network that will be created
const char *password = "";   // The password required to connect to it, leave blank for an open network

const char *OTAName = "ESP8266";           // A name and a password for the OTA service
const char *OTAPassword = "esp8266";

#define LED_RED     15            // specify the pins with an RGB LED connected
#define LED_GREEN   12
#define LED_BLUE    13

#define RED 0
#define GREEN 1
#define YELLOW 2
#define GRAY 3
#define PURPLE 4

#define MAX_DEVICES 30
#define JSON_BUF_SIZE 2048

#define DEVICE_NONE 0
#define DEVICE_GARAGE 1
#define DEVICE_THERMOMETOR 2
#define DEVICE_RELAY 3
#define DEVICE_IRSENSOR 4

#define SENSOR_TIMEOUT 20000

typedef struct {
  byte mac[6];
  int deviceType;
  char deviceName[16];
  int devicePosition;
  IPAddress ip;
  int sensor[2];
  unsigned int timer;
  int online;
} device;

device devices[MAX_DEVICES];

const char* mdnsName = "swoop"; // Domain name for the mDNS responder

/*__________________________________________________________SETUP__________________________________________________________*/

void setup() {
  Serial.begin(115200);        // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println("\r\n");

  memset(&devices, '\0', MAX_DEVICES * sizeof(device));   //clear devices
  for (int i=0; i<MAX_DEVICES; i++) devices[0].deviceType = DEVICE_NONE;  //and set deviceType to NONE  
 
  pinMode(LED_RED, OUTPUT);    // the pins with LEDs connected are outputs
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  startOTA();                  // Start the OTA service
  startSPIFFS();               // Start the SPIFFS and list all contents
  startWebSocket();            // Start a WebSocket server
  startMDNS();                 // Start the mDNS responder
  startServer();               // Start a HTTP server with a file read handler and an upload handler

  Serial.println("Mac Address = " + WiFi.macAddress());  
}

/*__________________________________________________________LOOP__________________________________________________________*/

bool rainbow = false;             // The rainbow effect is turned off on startup

unsigned long prevMillis = millis();
int hue = 0;

void loop() {
  webSocket.loop();                           // constantly check for websocket events
  server.handleClient();                      // run the server

  for (int i=0; i<MAX_DEVICES; i++) {
    if (devices[i].deviceType != DEVICE_NONE){
      if (millis() - devices[i].timer > SENSOR_TIMEOUT) {
        devices[i].online = 0;
        devices[i].timer = millis() - SENSOR_TIMEOUT;   //keep timer from rolling over
      }
    }
  }
  ArduinoOTA.handle();                        // listen for OTA events
}

int getDeviceCount (int deviceType) {
  int cnt = 0;
  for (int i=0; i<MAX_DEVICES; i++) {
    if (devices[i].deviceType == deviceType) cnt++;
  }
  return cnt;
}

String getJSONString() {
  StaticJsonDocument<JSON_BUF_SIZE> doc;
  String st;
  for (int i=0; i<MAX_DEVICES; i++) {
    if (devices[i].deviceType != DEVICE_NONE) {
      doc["devices"][i]["mac"] = macIDToString(devices[i].mac);      
      doc["devices"][i]["deviceType"] = devices[i].deviceType;
      doc["devices"][i]["deviceName"] = devices[i].deviceName;
      doc["devices"][i]["devicePosition"] = devices[i].devicePosition;
      doc["devices"][i]["ip"] = ipAddressToString(devices[i].ip);
      doc["devices"][i]["sensor0"] = devices[i].sensor[0];   
      doc["devices"][i]["sensor1"] = devices[i].sensor[1]; 
      doc["devices"][i]["timer"] = devices[i].timer; 
      doc["devices"][i]["online"] = devices[i].online; 
    } 
  }
  serializeJson(doc,st);
  return st;  
}

void saveSettings() {
  for (int i=0; i<MAX_DEVICES; i++) {
    deviceToString(devices[i]);
  }
}

void restoreSettings() {

}

String deviceToString(device Device) {
  char buffer[200];
  sprintf(buffer,"%s|%c|%s|%d|%s|%d|%d|%u|%d",macIDToString(Device.mac).c_str(),Device.deviceType,Device.deviceName,Device.devicePosition,ipAddressToString(Device.ip).c_str(),Device.sensor[0],Device.sensor[1],Device.timer,Device.online);
  Serial.print("|");
  Serial.print(buffer);
  Serial.println("|");  
}

void stringToDevice (String st, device *Device) {
  
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
    digitalWrite(LED_RED, 0);    // turn off the LEDs
    digitalWrite(LED_GREEN, 0);
    digitalWrite(LED_BLUE, 0);
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

String macIDToString (byte macID[]) {
  String st;
  char buf[20];
  sprintf(buf,"%x:%x:%x:%x:%x:%x", macID[0], macID[1], macID[2], macID[3], macID[4], macID[5]);
  st = buf;
  return st;
}

int deviceCount () {
  int count = 0;
  for (int i=0; i<MAX_DEVICES; i++) if (devices[i].deviceType != DEVICE_NONE) count++;
  return count;
}

String ipAddressToString(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ;
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
  Serial.println("mac: " + macIDToString(devices[deviceIndex].mac));
  Serial.print("Device Name: ");
  Serial.println(devices[deviceIndex].deviceName);
  Serial.println("IP: How do I get this?" + devices[deviceIndex].ip);
  Serial.print("deviceType: ");
  Serial.println(devices[deviceIndex].deviceType);
  Serial.print("sensor 1: ");
  Serial.println(devices[deviceIndex].sensor[0]);
  Serial.print("sensor 2: ");
  Serial.println(devices[deviceIndex].sensor[1]);
  Serial.print("timer: ");
  Serial.println(devices[deviceIndex].timer);  
  Serial.print("online: ");
  Serial.println(devices[deviceIndex].online);  
  Serial.println("****************************");
  
}

void handleSet () {
  String st;
  String Up = "", Down = "";
  String macString = "";
  int deviceIndex = -1;
  bool newDevice;
  char deviceName[16];

  String message = "Number of args received:";
  message += server.args();
  message += "\n";
  Serial.println(message);
  Serial.println(server.uri());
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
    }
    
    if (server.argName(i) == "save") {
      saveSettings();
    }
    if (server.argName(i) == "restore") {
      restoreSettings();
    }
   
    if (deviceIndex > -1) {
      if (server.argName(i) == "deviceType") {
        String st = server.arg(i);
        devices[deviceIndex].deviceType = st.toInt();
      }
      if (server.argName(i) == "sensor0") devices[deviceIndex].sensor[0] = trueFalse(server.arg(i));
      if (server.argName(i) == "sensor1") devices[deviceIndex].sensor[1] = trueFalse(server.arg(i));
      if (server.argName(i) == "devicePosition") {
        String st = server.arg(i);
        devices[deviceIndex].devicePosition = st.toInt();
      }
      if (server.argName(i) == "deviceName") {
        Serial.println("********************* deviceName ****************** =");
        int str_len = server.arg(i).length() + 1;
        if (str_len > 15) str_len = 15;
        char char_array[16];
        server.arg(i).toCharArray(char_array, str_len);
        memcpy(devices[deviceIndex].deviceName, char_array, str_len);
        Serial.println (devices[deviceIndex].deviceName);
      }
    }
  } //end for
  Serial.println(message);

  Serial.print ("----- Device Count = ");
  Serial.println (deviceCount());
  
  server.send(200, "text/plain", getJSONString());
  
  if (deviceIndex > -1) displayDevice(deviceIndex);
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
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location","/success.html");      // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        rainbow = false;                  // Turn rainbow off when a new connection is established
      }
      break;
    case WStype_TEXT:                     // if new text data is received
//      Serial.printf("[%u] get Text: %s\n", num, payload);
      if (payload[0] == '#') {            // button was pressed
        Serial.print ("Key Pressed: ");
        String st = (char *)payload;
        Serial.println(st.substring(1,st.length()));               
      } else {
        switch (payload[0]) {
          case '1': {                      // Door 1 pressed
              Serial.println("Door 1 pressed");
           
//              webSocket.send(JSON.ToString());
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
        } //end switch payload[0]
      }   //end else
  }  //end switch(type)
}

/*__________________________________________________________HELPER_FUNCTIONS__________________________________________________________*/

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
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
