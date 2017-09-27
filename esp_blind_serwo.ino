#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <Servo.h>
#include <EEPROM.h>

const char *modName = "SmartDomEsp8266Blind";
const int port = 81;
const int timeout = 20;

#define PIN_PWN 16
#define PIN_RESET 14

ESP8266WebServer server (port);
Servo myservo;

String webServerAddress;

bool prevReset = false;

bool isServerVerified(){
  String clientAddress = server.client().remoteIP().toString();
  return webServerAddress == clientAddress;
}

void handleRoot() {
  if(!isServerVerified()){
    server.send (401, "text/plain", "Unauthorized client");
    return;
  }
    
  server.send(200, "text/plain", "hello from " + String(modName));
}

void getType() {
  if(!isServerVerified()){
    server.send (401, "text/plain", "Unauthorized client");
    return;
  }
  server.send(200, "text/plain", "BLIND_MOTOR_MODULE");
}

void open(){
  Serial.println ("in open");
  myservo.write(82);
}

void close(){
  Serial.println ("in close");                               
  myservo.write(102);
}

void stop(){
  Serial.println ("in stop");
  myservo.write(92);
  
}

void openBlind(){
  if(!isServerVerified()){
    server.send (401, "text/plain", "Unauthorized client");
    return;
  }
  
  StaticJsonBuffer<100> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));

  bool start = root["start"];
  if(start){
    open();
  }
  else{
    stop();
  }
  Serial.println ("finish open");
  server.send ( 200, "text/html", "" );
}

void closeBlind(){
  if(!isServerVerified()){
    server.send (401, "text/plain", "Unauthorized client");
    return;
  }
  
  StaticJsonBuffer<100> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));

  bool start = root["start"];
  if(start){
    close();
  }
  else{
    stop();
  }
  Serial.println ("finish close");
  server.send ( 200, "text/html", "" );
}

void handleNotFound() {

  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
}

void initWiFiServoConnection(){
  Serial.println ("");
  Serial.println("Loading data from memory...");

  char tmpChar;
  int eeprom_counter = 0;
  String ssid = "";
  String password = "";

  if(EEPROM.read(0) != 0){
    while((tmpChar = EEPROM.read(eeprom_counter ++)) != 0 && eeprom_counter < 512){
      ssid += tmpChar;
    }
    while((tmpChar = EEPROM.read(eeprom_counter ++)) != 0 && eeprom_counter < 512){
      password += tmpChar;
    }
    while((tmpChar = EEPROM.read(eeprom_counter ++)) != 0 && eeprom_counter < 512){
      webServerAddress += tmpChar;
    }
  }
  else{
    Serial.print ("Podaj nazwe sieci: ");
    while(Serial.available() <= 0);
 
    ssid += Serial.readString();
    ssid.trim();
    Serial.println (ssid);
    
    Serial.print ("Podaj haslo:");
    while(Serial.available() <= 0);

    password += Serial.readString();
    password.trim();
    Serial.println (password);

    Serial.print ("Podaj adres serwera aplikacji:");
    while(Serial.available() <= 0);

    webServerAddress += Serial.readString();
    webServerAddress.trim();
    Serial.println (webServerAddress);
  }
  
  WiFi.begin (ssid.c_str(), password.c_str());
  Serial.println ("");

  int timeWiFi = 0;
  while (WiFi.status() != WL_CONNECTED && timeWiFi++ < timeout) {
    delay (500);
    Serial.print (".");
  }

  if(timeWiFi >= timeout){
    Serial.println ("");
    Serial.print ("Connection error. Try reseting connection.");
    return;
  }

  for(int i = 0; i< 4; ++i){
    digitalWrite (2, 1);
    delay(200);
    digitalWrite (2, 0);
    delay(200);
    digitalWrite (2, 1);
  }

  Serial.println ("");
  Serial.print ("Connected to ");
  Serial.println (ssid);
  Serial.print ("IP address: ");
  Serial.println (WiFi.localIP());

  if (MDNS.begin (modName)) {
    MDNS.addService("http", "tcp", port);
    Serial.println ("MDNS responder started");
  }

  if(EEPROM.read(0) == 0){
    for(int i = 0; i< ssid.length(); ++i){
      EEPROM.write(i, ssid[i]);
    }
    EEPROM.write(ssid.length(), 0);
    
    int eepromStart = ssid.length() + 1;
    for(int i = 0; i < password.length(); ++i){
      EEPROM.write(i + eepromStart, password[i]);
    }
    EEPROM.write(eepromStart + password.length(), 0);

    eepromStart = eepromStart + password.length() + 1;
    for(int i = 0; i < webServerAddress.length(); ++i){
      EEPROM.write(i + eepromStart, webServerAddress[i]);
    }
    EEPROM.write(eepromStart + webServerAddress.length(), 0);

    EEPROM.commit();
  }

  server.on("/", HTTP_GET, handleRoot );
  server.on("/type", HTTP_GET, getType );
  server.on("/openBlind", HTTP_POST, openBlind);
  server.on("/closeBlind", HTTP_POST, closeBlind);
  server.onNotFound ( handleNotFound );
  server.begin();
  Serial.println ( "HTTP server started" );
  myservo.attach(PIN_PWN);
}

void setup() { 
  pinMode ( 2, OUTPUT );
  pinMode ( PIN_RESET, INPUT);
  Serial.begin ( 115200 );
  EEPROM.begin(512);
  initWiFiServoConnection();
 
} 
 
void loop() { 
  int resetEepromState = digitalRead(PIN_RESET);
  if(resetEepromState > 0 && !prevReset){
    myservo.detach();
    WiFi.disconnect();
    int eeprom_counter = 0;
    for(int i = 0; i < 3; ++i){ // deleting wifi, password, serwer address
      while(EEPROM.read(eeprom_counter ++) != 0 && eeprom_counter < 512){
        EEPROM.write(eeprom_counter - 1, 0);
      }
    }
    webServerAddress = "";
    EEPROM.commit();
    initWiFiServoConnection();
  }
  else{
    server.handleClient();
  }
  prevReset = resetEepromState;
} 

