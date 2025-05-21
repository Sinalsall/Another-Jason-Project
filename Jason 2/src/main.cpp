#include <Arduino.h>
#include "WebServerLib.h"
#include "GravityTDS.h"

// Define pins
const int trig_pin = 17;  // Ultrasonic 1 trig
const int echo_pin = 18;  // Ultrasonic 1 echo
const int TRIG_PIN = 35;  // Ultrasonic 2 trig
const int ECHO_PIN = 36;  // Ultrasonic 2 echo
const int TdsSensorPin = 4;  // TDS sensor
const int adcPin = 5;     // pH sensor
const int SoilPin = 6;    // Soil moisture sensor
const int relayPinLaut = 1; // Relay Laut
const int relayPinTDS = 14;  // Relay TDS
const int relayPinTank = 12; // Relay Tank

GravityTDS gravityTds;
WebServerLib webServer;

float temperature = 25, tdsValue = 0;
float phValue = 0;
float distanceRes = 0;
float distanceTank = 0;
int soilMoisture = 0;

unsigned long previousMillis = 0;
const long interval = 5000;

void setup() {
  Serial.begin(115200);
  pinMode(trig_pin, OUTPUT);
  pinMode(echo_pin, INPUT);
  digitalWrite(trig_pin, LOW);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  gravityTds.setPin(TdsSensorPin);
  gravityTds.setAref(3.3);     // Reference voltage for ESP32
  gravityTds.setAdcRange(4096); // 12-bit ADC
  gravityTds.begin();
  pinMode(relayPinLaut, OUTPUT);
  pinMode(relayPinTDS, OUTPUT);
  pinMode(relayPinTank, OUTPUT);

  // Connect to Wi-Fi
  const char* ssid = "Samsung Galaxy";     // Ganti dengan SSID Wi-Fi Anda
  const char* password = "12345678"; // Ganti dengan kata sandi Wi-Fi Anda
  webServer.begin(ssid, password);
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Read pH
    float adcValue = analogRead(adcPin);
    // Catatan: Perhitungan pH ini adalah placeholder, sesuaikan dengan sensor Anda
    phValue = (2.5 -((adcValue / 4095.0) * 3.3))/0.18 -3; // Mengubah ke tegangan (perlu kalibrasi)

    // Read soil moisture
    soilMoisture = analogRead(SoilPin);
    soilMoisture = map(soilMoisture, 0, 4095, 255, 0);

    // Read ultrasonic 1 (Reservoir)
    digitalWrite(trig_pin, LOW);
    delayMicroseconds(2);
    digitalWrite(trig_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig_pin, LOW);
    float timing = pulseIn(echo_pin, HIGH);
    distanceRes = (timing * 0.0343) / 2; // Kecepatan suara 343 m/s

    // Read ultrasonic 2 (Tank)
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    unsigned long duration1 = pulseIn(ECHO_PIN, HIGH);
    distanceTank = duration1 / 58.2; // Sederhana: 29 us/cm untuk round trip

    // Read TDS
    gravityTds.setTemperature(temperature);
    gravityTds.update();
    tdsValue = gravityTds.getTdsValue();

    // Control relays
    if (distanceRes < 8) {
      digitalWrite(relayPinLaut, LOW); // Relay aktif LOW (sesuaikan dengan modul)
    } else {
      digitalWrite(relayPinLaut, HIGH);
    }
    if (tdsValue < 500 && distanceRes < 8) {
      digitalWrite(relayPinTDS, HIGH);
    } else {
      digitalWrite(relayPinTDS, LOW);
    }
    if (tdsValue > 500 && distanceTank > 8) {
      digitalWrite(relayPinTank, HIGH);
    } else {
      digitalWrite(relayPinTank, LOW);
    }

    // Update web server with sensor values
    webServer.setSensorValues(phValue, tdsValue, distanceRes, distanceTank, soilMoisture);

    // Print to serial for debugging
    Serial.print("Ph: ");
    Serial.println(phValue, 3);
    Serial.print("TDS: ");
    Serial.print(tdsValue, 0);
    Serial.println(" ppm");
    Serial.print("DistanceRes: ");
    Serial.print(distanceRes);
    Serial.println(" cm");
    Serial.print("DistanceTank: ");
    Serial.print(distanceTank);
    Serial.println(" cm");
    Serial.print("SoilMoisture: ");
    Serial.println(soilMoisture);
  }
  webServer.handleClient();
}