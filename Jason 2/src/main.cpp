#include <Arduino.h>
#include "WebServerLib.h" // Asumsikan pustaka ini sudah ada dan berfungsi
#include "GravityTDS.h"
// Tidak lagi menggunakan DFRobot_ESP_PH_BY_GREENPONIK secara langsung untuk kalkulasi pH di readPHSensor
// karena kita akan melakukan kalkulasi manual berdasarkan kalibrasi 2 titik.
// Namun, Anda mungkin masih memerlukannya jika ingin menggunakan fitur lain dari library tersebut,
// atau jika Anda ingin membandingkan. Untuk contoh ini, kita akan fokus pada metode kalibrasi manual.

// Define pins
const int trig_pin = 17;  // Ultrasonic 1 trig
const int echo_pin = 18;  // Ultrasonic 1 echo
const int TRIG_PIN = 35;  // Ultrasonic 2 trig (tank)
const int ECHO_PIN = 36;  // Ultrasonic 2 echo (tank)
const int TdsSensorPin = 4;  // TDS sensor
const int adcPin = 5;      // pH sensor ADC pin
const int SoilPin = 6;     // Soil moisture sensor
const int relayPinLaut = 1; // Relay Laut
const int relayPinTDS = 14;  // Relay TDS
const int relayPinTank = 12; // Relay Tank

GravityTDS gravityTds;
WebServerLib webServer;

// Variabel Kalibrasi pH (HARUS DISESUAIKAN berdasarkan kalibrasi Anda)
float neutralVoltage = 1.02; // Tegangan saat di buffer pH 7.0 (Contoh dari video)
float acidVoltage = 2.74;    // Tegangan saat di buffer pH 4.0 (Contoh dari video)

// Untuk DFRobot pH meter, biasanya pH 7 memberikan tegangan di sekitar 1.5V (bisa bervariasi)
// dan pH 4 memberikan tegangan yang lebih tinggi (misalnya 2.0V - 2.5V).
// Video menggunakan pH meter yang mungkin memiliki karakteristik output berbeda (mis. V2 Pro),
// jadi pastikan nilai di atas sesuai dengan sensor Anda setelah kalibrasi.

float temperature = 25, tdsValue = 0;
float phValue = 0;
float distanceRes = 0;
float distanceTank = 0;
int soilMoisture = 0;

unsigned long previousMillis = 0;
const long interval = 5000; // Interval pembacaan sensor

// --- Fungsi Baru untuk Membaca pH dengan Kalibrasi 2 Titik ---
float readPHSensor() {
  int adcValue = analogRead(adcPin);

  float voltage = adcValue * (3.3 / 4095.0);


  float slope = (7.0 - 4.0) / (neutralVoltage - acidVoltage);


  float currentPH = 7.0 + slope * (voltage - neutralVoltage);

  // Debugging tegangan dan pH hasil perhitungan
  Serial.print("pH ADC: "); Serial.print(adcValue);
  Serial.print(", pH Voltage: "); Serial.print(voltage, 3);
  Serial.print("V, Calculated pH: "); Serial.println(currentPH, 2);




  if (isnan(currentPH) || isinf(currentPH)) {
    Serial.println("Gagal membaca pH, periksa nilai kalibrasi atau koneksi sensor.");
    return -1.0; // Mengembalikan nilai error
  }
  return currentPH;
}

// --- Fungsi untuk melakukan proses kalibrasi (placeholder) ---
// Idealnya, ini akan interaktif, meminta pengguna mencelupkan sensor dan menekan tombol/perintah
void performPHCalibration() {
  Serial.println("Memulai Kalibrasi pH...");
  // Proses ini biasanya akan meminta pengguna untuk:
  // 1. Celupkan sensor ke larutan pH 7, tunggu stabil, lalu baca tegangan.
  //    Misalnya: neutralVoltage = bacaTeganganStabil();
  // 2. Celupkan sensor ke larutan pH 4, tunggu stabil, lalu baca tegangan.
  //    Misalnya: acidVoltage = bacaTeganganStabil();
  // 3. Simpan nilai neutralVoltage dan acidVoltage (misalnya ke EEPROM).

  Serial.println("Kalibrasi pH selesai (nilai saat ini di-hardcode).");
  Serial.print("Neutral Voltage (pH 7.0) diatur ke: "); Serial.println(neutralVoltage);
  Serial.print("Acid Voltage (pH 4.0) diatur ke: "); Serial.println(acidVoltage);
  // Untuk saat ini, kita hanya menampilkan nilai yang sudah di-hardcode.
  // Anda perlu mengembangkan fungsi ini lebih lanjut untuk kalibrasi aktual.
}


void setup() {
  Serial.begin(115200);
  pinMode(trig_pin, OUTPUT);
  pinMode(echo_pin, INPUT);
  digitalWrite(trig_pin, LOW);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  gravityTds.setPin(TdsSensorPin);
  gravityTds.setAref(3.3);      // Reference voltage for ESP32
  gravityTds.setAdcRange(4096); // 12-bit ADC
  gravityTds.begin();

  pinMode(relayPinLaut, OUTPUT);
  pinMode(relayPinTDS, OUTPUT);
  pinMode(relayPinTank, OUTPUT);

  // Anda bisa memanggil performPHCalibration() di sini jika ingin
  // "mengkalibrasi" setiap kali startup (dengan nilai hardcode saat ini)
  // atau jika Anda mengembangkan fungsi interaktif.
  // performPHCalibration(); // Komentari jika tidak ingin dijalankan saat startup

  Serial.println("Setup selesai. Menggunakan nilai kalibrasi pH yang ada.");
  Serial.print("Neutral Voltage (pH 7.0): "); Serial.println(neutralVoltage);
  Serial.print("Acid Voltage (pH 4.0): "); Serial.println(acidVoltage);


  // Connect to Wi-Fi
  const char* ssid = "Samsung Galaxy";      // Ganti dengan SSID Wi-Fi Anda
  const char* password = "12345678"; // Ganti dengan kata sandi Wi-Fi Anda
  webServer.begin(ssid, password);
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // --- Read pH using the new function ---
    phValue = readPHSensor();

    // Read soil moisture
    soilMoisture = analogRead(SoilPin);
    soilMoisture = map(soilMoisture, 0, 4095, 225, 0); // Sesuaikan rentang jika perlu

    // Read ultrasonic 1 (Reservoir)
    digitalWrite(trig_pin, LOW);
    delayMicroseconds(2);
    digitalWrite(trig_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig_pin, LOW);
    float timing = pulseIn(echo_pin, HIGH);
    distanceRes = 11 - ((timing * 0.0343) / 2);

    // Read ultrasonic 2 (Tank)
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    unsigned long duration1 = pulseIn(ECHO_PIN, HIGH);
    distanceTank = 14 - ((duration1 * 0.0343) / 2);

    // Read TDS
    gravityTds.setTemperature(temperature);
    gravityTds.update();
    tdsValue = gravityTds.getTdsValue();

    // Control relays
    if (distanceRes < 4) {
      digitalWrite(relayPinLaut, LOW);
    } else {
      digitalWrite(relayPinLaut, HIGH);
    }
    if (distanceTank < 6) {
      digitalWrite(relayPinTank, HIGH);
    } else {
      digitalWrite(relayPinTank, LOW);
    }

    // Update web server with sensor values
    webServer.setSensorValues(phValue, tdsValue, distanceRes, distanceTank, soilMoisture);

    // Print to serial for debugging
    Serial.println("--- Sensor Readings ---");
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
    Serial.println("--------------------");
  }
  webServer.handleClient();
}