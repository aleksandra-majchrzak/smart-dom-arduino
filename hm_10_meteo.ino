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

void setup(){
  pinMode(2, INPUT);
  Serial.begin(115200);
  softSerial.begin(9600);
  mq2.begin();
}

void loop(){
  int c;
  
  if (softSerial.available()) {
    c = softSerial.read();  
    Serial.println("Got input:");
    if (c != 0){
      lpg = mq2.readLPG();
      co = mq2.readCO();
      smoke = mq2.readSmoke();
      char out[50] = "";
      String output = "{lpg: " + String(lpg) + " CO: " + String(co) + " smoke: " + String(smoke)+"}";
      // z pinu 2 mozesz jeszcze odczytac alarm
      output.toCharArray(out, 50);
      Serial.println(output);
       
      softSerial.write(out);
    }
    else{
      dht11.read(bHumi, bTemp);
      char out[30] = "";
      String output = "{temp: " + String(bTemp)+ " wilg: " + String(bHumi)+"}";
      output.toCharArray(out, 30);
      Serial.println(output);
       
      softSerial.write(out);
    }
  }
}
