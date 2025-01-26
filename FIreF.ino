#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <SoftwareSerial.h>


#define FLAME_SENSOR_PIN 5
#define DHT_PIN 4
#define DHT_TYPE DHT22
#define RELAY_PIN 25
#define BUZZER_PIN 26


DHT dht(DHT_PIN, DHT_TYPE);


SoftwareSerial gsmSerial(17, 16); 

bool smsSent = false;


unsigned long previousMillis = 0;
int frequency = 500; 
bool increasing = true;

void setup() {

  Serial.begin(115200);

  gsmSerial.begin(9600);


  dht.begin();

  // Configure pin modes
  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); 
  digitalWrite(BUZZER_PIN, LOW); 


  delay(10000);
}

void loop() {
  int val = digitalRead(FLAME_SENSOR_PIN);


  Serial.print("Flame Sensor Value: ");
  Serial.println(val);

  if (val == HIGH) { 

    Serial.println("Fire detected!");


    digitalWrite(RELAY_PIN, LOW);


    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 10) { 
      previousMillis = currentMillis;
      if (increasing) {
        frequency += 10;
        if (frequency >= 1000) { 
          increasing = false;
        }
      } else {
        frequency -= 10;
        if (frequency <= 500) { 
          increasing = true;
        }
      }
      tone(BUZZER_PIN, frequency);
    }


    if (!smsSent) {

      float temperature = dht.readTemperature();
      float humidity = dht.readHumidity();

 
      String message = "Warning! Fire detected! Temp: " + String(temperature) + "C, Humidity: " + String(humidity) + "%";

      sendSMS("09999", message);

      smsSent = true;
    }

  } else { 

    Serial.println("No fire detected.");


    digitalWrite(RELAY_PIN, HIGH);
    noTone(BUZZER_PIN);


    smsSent = false;
  }

  delay(100);
}

void sendSMS(String phoneNumber, String message) {
  gsmSerial.println("AT+CMGF=1"); 
  delay(1000);
  gsmSerial.println("AT+CMGS=\"" + phoneNumber + "\""); 
  delay(1000);
  gsmSerial.print(message);
  delay(1000);
  gsmSerial.write(26); 
  delay(1000);
}
