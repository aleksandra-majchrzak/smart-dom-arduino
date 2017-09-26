#include <MQ2.h>
#include <DHT11.h>
#include <SoftwareSerial.h>

#define MQ_PIN A0
#define DTH11_DIN 4
#define SOFT_TX 7
#define SOFT_RX 8

DHT11 dht11(DTH11_DIN);
MQ2 mq2(MQ_PIN);
SoftwareSerial softSerial(SOFT_TX, SOFT_RX);

float bTemp, bHumi;
float lpg, co, smoke;

void setup() {
  pinMode(2, INPUT);
  Serial.begin(115200);
  softSerial.begin(9600);
  mq2.begin();
}

void getGases(){
  lpg = mq2.readLPG();
  co = mq2.readCO();
  smoke = mq2.readSmoke();
  char out[15] = "";
  String output = "lpg:" + String(lpg);
  // z pinu 2 mozesz jeszcze odczytac alarm
  output.toCharArray(out, 15);
  Serial.println(output);
  softSerial.write(out);
  
  delay(100);
  output = "CO:" + String(co);
  output.toCharArray(out, 15);
  Serial.println(output);
  softSerial.write(out);
  delay(100);
  
  output = "smoke:" + String(smoke);
  output.toCharArray(out, 15);
  Serial.println(output);
  softSerial.write(out);
  delay(100);
}

void getTemperatureHumidity(){
  dht11.read(bHumi, bTemp);
  char out[15] = "";
  String output = "temp:" + String(bTemp);
  output.toCharArray(out, 15);
  Serial.println(output);
  softSerial.write(out);
  delay(100);
  
  output = "humi:" + String(bHumi);
  output.toCharArray(out, 15);
  Serial.println(output);

  softSerial.write(out);
}

void loop() {
  int c;
  if (softSerial.available()) {
    c = softSerial.read();
    Serial.println("Got input:");
    if (c != 0) {
      getGases();
    }
    else {
      getTemperatureHumidity();
    }
  }
}
