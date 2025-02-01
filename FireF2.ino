#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <SoftwareSerial.h>

//Sheila*082574
#define FLAME_SENSOR_PIN 5
#define DHT_PIN 4
#define DHT_TYPE DHT22
#define RELAY_PIN 25
#define BUZZER_PIN 26

// Initialize DHT sensor
DHT dht(DHT_PIN, DHT_TYPE);

// Initialize GSM module (RX, TX)
SoftwareSerial gsmSerial(17, 16); // RX, TX

// Variable to track if SMS has been sent for the current fire detection
bool smsSent = false;

// Variables for siren effect
unsigned long previousMillis = 0;
int frequency = 500; // Starting frequency
bool increasing = true;

void setup() {
  // Start serial communication for debugging
  Serial.begin(115200);
  // Start GSM serial communication
  gsmSerial.begin(9600);

  Serial.println("Initializing...");
  
  // Initialize DHT sensor
  dht.begin();

  // Configure pin modes
  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Ensure relay is off initially
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is off initially

  // Give time for GSM module to initialize
  delay(10000);

  // Test GSM module communication
  Serial.println("Testing GSM module...");
  gsmSerial.println("AT"); // Send AT command
  delay(1000);
  while (gsmSerial.available()) {
    Serial.write(gsmSerial.read()); // Print GSM response
  }

  // Check SIM card presence
  Serial.println("Checking SIM card presence...");
  gsmSerial.println("AT+CSMINS?");
  delay(1000);
  while (gsmSerial.available()) {
    Serial.write(gsmSerial.read()); // Print GSM response
  }

  // Check network registration
  Serial.println("Checking network registration...");
  gsmSerial.println("AT+CREG?");
  delay(1000);
  while (gsmSerial.available()) {
    Serial.write(gsmSerial.read()); // Print GSM response
  }

  // Check signal strength
  Serial.println("Checking signal strength...");
  gsmSerial.println("AT+CSQ");
  delay(1000);
  while (gsmSerial.available()) {
    Serial.write(gsmSerial.read()); // Print GSM response
  }

  Serial.println("GSM initialization complete.");
}

void loop() {
  int val = digitalRead(FLAME_SENSOR_PIN);

  // Print the sensor value for debugging
  Serial.print("Flame Sensor Value: ");
  Serial.println(val);

  if (val == HIGH) { // Flame detected
    // Print debug message
    Serial.println("Fire detected!");

    // Turn on relay
    digitalWrite(RELAY_PIN, LOW);

    // Implement siren sound for buzzer
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 10) { // Change frequency every 10 ms
      previousMillis = currentMillis;
      if (increasing) {
        frequency += 10;
        if (frequency >= 1000) { // Max frequency
          increasing = false;
        }
      } else {
        frequency -= 10;
        if (frequency <= 500) { // Min frequency
          increasing = true;
        }
      }
      tone(BUZZER_PIN, frequency);
    }

    // Send SMS message if not already sent
    if (!smsSent) {
      // Read temperature and humidity
      float temperature = dht.readTemperature();
      float humidity = dht.readHumidity();

      // Compose SMS message
      String message = "Warning! Fire detected! Temp: " + String(temperature) + "C, Humidity: " + String(humidity) + "%";

      // Send SMS message
      sendSMS("09751969408", message); // Replace with the recipient's phone number

      // Set flag to indicate SMS has been sent
      smsSent = true;
    }

  } else { // No fire detected
    // Print debug message
    Serial.println("No fire detected.");

    // Turn off relay and buzzer
    digitalWrite(RELAY_PIN, HIGH);
    noTone(BUZZER_PIN);

    // Reset SMS sent flag
    smsSent = false;
  }

  // Small delay before next loop iteration
  delay(100);
}

void sendSMS(String phoneNumber, String message) {
  Serial.println("Sending SMS...");
  
  // Set SMS mode to text
  gsmSerial.println("AT+CMGF=1");
  delay(1000);
  while (gsmSerial.available()) {
    Serial.write(gsmSerial.read()); // Print GSM response
  }

  // Set recipient phone number
  gsmSerial.println("AT+CMGS=\"" + phoneNumber + "\"");
  delay(1000);
  while (gsmSerial.available()) {
    Serial.write(gsmSerial.read()); // Print GSM response
  }

  // Send message content
  gsmSerial.print(message);
  delay(1000);

  // Send Ctrl+Z (ASCII 26) to indicate the end of the message
  gsmSerial.write(26);
  delay(3000);
  while (gsmSerial.available()) {
    Serial.write(gsmSerial.read()); // Print GSM response
  }

  Serial.println("SMS sent.");
}
