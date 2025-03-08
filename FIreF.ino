#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <SoftwareSerial.h>

// Pin Definitions
#define FLAME_SENSOR_PIN 5
#define DHT_PIN 4
#define DHT_TYPE DHT22
#define RELAY_PIN 25
#define BUZZER_PIN 26
#define RELAY_ON LOW    // Assuming active-low relay
#define RELAY_OFF HIGH

// Global Variables
DHT dht(DHT_PIN, DHT_TYPE);
SoftwareSerial gsmSerial(17, 16);  // TX, RX
bool smsSent = false;
unsigned long previousMillis = 0;
int frequency = 500;
bool increasing = true;

void setup() {
  Serial.begin(115200);
  gsmSerial.begin(9600);
  dht.begin();
  
  // Pin Configuration
  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initial States
  digitalWrite(RELAY_PIN, RELAY_OFF);
  digitalWrite(BUZZER_PIN, LOW);
  
  // GSM Initialization
  delay(10000);  // Wait for GSM module to initialize
  gsmSerial.println("AT");
  delay(1000);
  gsmSerial.println("AT+CSMINS?");
  delay(1000);
  gsmSerial.println("AT+CREG?");
  delay(1000);
  gsmSerial.println("AT+CSQ");
  delay(1000);
}

void loop() {
  int val = digitalRead(FLAME_SENSOR_PIN);
  Serial.print("Flame Sensor Value: ");
  Serial.println(val);

  unsigned long currentMillis = millis();
  
  if (val == HIGH) {
    Serial.println("Fire detected!");
    digitalWrite(RELAY_PIN, RELAY_ON);
    
    // Buzzer control with 100ms interval
    if (currentMillis - previousMillis >= 100) {
      previousMillis = currentMillis;
      if (increasing) {
        frequency += 20;
        if (frequency >= 1000) increasing = false;
      } else {
        frequency -= 20;
        if (frequency <= 500) increasing = true;
      }
      tone(BUZZER_PIN, frequency);
    }
    
    // SMS handling
    if (!smsSent) {
      float temperature = dht.readTemperature();
      float humidity = dht.readHumidity();
      String message;
      if (isnan(temperature) || isnan(humidity)) {
        message = "Warning! Fire detected! Sensor error";
      } else {
        message = "Warning! Fire detected! Temp: " + String(temperature) + "C, Humidity: " + String(humidity) + "%";
      }
      sendSMS("09751969408", message);
      smsSent = true;
    }
  } else {
    Serial.println("No fire detected.");
    digitalWrite(RELAY_PIN, RELAY_OFF);
    noTone(BUZZER_PIN);
    smsSent = false;
  }
  delay(100);  // Main loop delay
}

void sendSMS(String phoneNumber, String message) {
  Serial.println("Sending SMS...");
  
  // Set SMS text mode
  gsmSerial.println("AT+CMGF=1");
  delay(1000);
  
  // Check if command was successful
  if (waitForResponse("OK", 2000)) {
    gsmSerial.println("AT+CMGS=\"" + phoneNumber + "\"");
    delay(1000);
    
    gsmSerial.print(message);
    delay(1000);
    gsmSerial.write(26);  // Ctrl+Z to send
    delay(3000);
    
    if (waitForResponse("OK", 5000)) {
      Serial.println("SMS sent successfully");
    } else {
      Serial.println("SMS sending failed");
    }
  } else {
    Serial.println("Failed to set SMS mode");
  }
}

// Helper function to wait for GSM response
bool waitForResponse(String expected, unsigned long timeout) {
  unsigned long startTime = millis();
  String response = "";
  
  while (millis() - startTime < timeout) {
    if (gsmSerial.available()) {
      char c = gsmSerial.read();
      response += c;
      if (response.indexOf(expected) != -1) {
        return true;
      }
    }
  }
  Serial.println("GSM Response: " + response);
  return false;
}
