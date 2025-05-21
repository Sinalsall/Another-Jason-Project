#include "WebServerLib.h"

WebServerLib::WebServerLib() : server(80) {}

void WebServerLib::begin(const char* ssid, const char* password) {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    server.on("/", std::bind(&WebServerLib::handleRoot, this));
    server.begin();
}

void WebServerLib::handleClient() {
    server.handleClient();
}

void WebServerLib::setSensorValues(float ph, float tds, float distanceRes, float distanceTank, int soilMoisture) {
    phValue = ph;
    tdsValue = tds;
    distanceResValue = distanceRes;
    distanceTankValue = distanceTank;
    soilMoistureValue = soilMoisture;
}

void WebServerLib::handleRoot() {
    String html = "<html><head><meta http-equiv='refresh' content='5'></head><body>";
    html += "<h1>Sensor Values</h1>";
    html += "<p>Ph: " + String(phValue, 3) + "</p>";
    html += "<p>TDS: " + String(tdsValue, 0) + " ppm</p>";
    html += "<p>Distance Reservoir: " + String(distanceResValue) + " cm</p>";
    html += "<p>Distance Tank: " + String(distanceTankValue) + " cm</p>";
    html += "<p>Soil Moisture: " + String(soilMoistureValue) + "</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}