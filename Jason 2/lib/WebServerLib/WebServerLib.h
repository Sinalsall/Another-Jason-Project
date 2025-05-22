#ifndef WEBSERVERLIB_H
#define WEBSERVERLIB_H

#include <WiFi.h>
#include <WebServer.h> // Pustaka WebServer standar ESP32

// Define maximum values for sensor gauges
#define MAX_DISTANCE_ULTRASONIC_GAUGE 15.0f // Max distance for Reservoir and Tank gauges (cm)
#define MAX_PH_GAUGE 14.0f
#define MAX_TDS_GAUGE 1000.0f // ppm
#define MAX_SOIL_MOISTURE_GAUGE 255.0f // Mapped value from main.cpp (0-255)

class WebServerLib { // Nama kelas sekarang WebServerLib
public:
    WebServerLib(); // Constructor
    void begin(const char* ssid, const char* password); // Method untuk memulai WiFi dan server
    void handleClient();
    void setSensorValues(float ph, float tds, float distanceRes, float distanceTank, int soilMoisture);

private:
    WebServer server; // Instance dari WebServer standar ESP32, dikelola oleh kelas ini

    // Variables to store sensor values directly from main.cpp
    float currentPhValue;
    float currentTdsValue;
    float currentDistanceRes;  // Raw distance from ultrasonic 1
    float currentDistanceTank; // Raw distance from ultrasonic 2
    int currentSoilMoisture;   // Raw value from soil moisture sensor

    // Helper methods
    String generateHTML();
    String getSensorDataJSON();

    // Handler methods
    void handleRoot();
    void handleSensorData();
};

#endif // WEBSERVERLIB_H