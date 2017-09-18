#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

const char *modName = "SmartDomEsp8266Light";
const int port = 80;
const int timeout = 20;

#define PIN 12
#define PIN_MAN 13
#define PIN_RESET 14

ESP8266WebServer server (port);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, PIN, NEO_GRB + NEO_KHZ800);
bool prevOn = false;
bool prevReset = false;

void handleRoot() {
server.send(200, "text/plain", "hello from " + String(modName));
}

void getType() {
  server.send(200, "text/plain", "LIGHT_MODULE");
}

void turnOnLight() {
  Serial.println("inside turn on light");

  for (auto i = 0; i < 8; ++i)
  {
    strip.setPixelColor(i, strip.Color(100, 100, 50));
  }
  strip.show();
  server.send (200, "text/html", "");
}

void turnOffLight() {
  Serial.println("inside turn off light");

  for (auto i = 0; i < 8; ++i)
  {
    strip.setPixelColor(i, 0);
  }
  strip.show();
  server.send (200, "text/html", "");
}

void setStripColor() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));

  int r = root["red"];
  int g = root["green"];
  int b = root["blue"];

  for (auto i = 0; i < 8; ++i)
  {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
  server.send (200, "text/html", "");
}

void setStripBrightness(){
  StaticJsonBuffer<100> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));
  
  int brightness = root["brightness"];
  brightness = map(brightness, 0, 100, 1, 255);

  strip.setBrightness(brightness);

  strip.show();
  server.send (200, "text/html", "");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName (i) + ": " + server.arg (i) + "\n";
  }

  server.send (404, "text/plain", message);
}

void initWiFiConnection(){
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
  
  strip.clear();
  strip.show();

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

    EEPROM.commit();
  }

  server.on("/", HTTP_GET, handleRoot );
  server.on("/type", HTTP_GET, getType );
  server.on("/turnOnLight", HTTP_POST, turnOnLight);
  server.on("/turnOffLight", HTTP_POST, turnOffLight);
  server.on("/setStripColor", HTTP_POST, setStripColor);
  server.on("/setStripBrightness", HTTP_POST, setStripBrightness);
  server.onNotFound ( handleNotFound );
  server.begin();
  Serial.println ( "HTTP server started" );
}

void setup ( void ) {
  pinMode ( 2, OUTPUT );
  pinMode ( PIN_MAN, INPUT);
  pinMode ( PIN_RESET, INPUT);
   
  strip.begin();
  strip.clear();
  strip.show();
  Serial.begin ( 115200 );
  EEPROM.begin(512);

  initWiFiConnection();
}

void loop ( void ) {
  int changeState = digitalRead(PIN_MAN);
  int resetEepromState = digitalRead(PIN_RESET);
  if(changeState > 0){
    int stripColor = strip.getPixelColor(0);
    if(stripColor == 0 && !prevOn){
      turnOnLight();
    }
    else if(!prevOn){
      turnOffLight();
    }
    delay(100);
  }
  else if(resetEepromState > 0 && !prevReset){
    WiFi.disconnect();
    int eeprom_counter = 0;
    while(EEPROM.read(eeprom_counter ++) != 0 && eeprom_counter < 512){
      EEPROM.write(eeprom_counter - 1, 0);
    }
    while(EEPROM.read(eeprom_counter ++) != 0 && eeprom_counter < 512){
      EEPROM.write(eeprom_counter - 1, 0);
    }
    EEPROM.commit();
    initWiFiConnection();
  }
  else{
    server.handleClient();
  }
  prevOn = changeState;
  prevReset = resetEepromState;
}
