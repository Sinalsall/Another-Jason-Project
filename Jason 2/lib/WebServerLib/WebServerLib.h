#ifndef WEBSERVERLIB_H
#define WEBSERVERLIB_H

#include <WiFi.h>
#include <WebServer.h>

class WebServerLib {
public:
    WebServerLib();
    void begin(const char* ssid, const char* password);
    void handleClient();
    void setSensorValues(float ph, float tds, float distanceRes, float distanceTank, int soilMoisture);
private:
    WebServer server;
    float phValue;
    float tdsValue;
    float distanceResValue;
    float distanceTankValue;
    int soilMoistureValue;
    void handleRoot();
};

#endif