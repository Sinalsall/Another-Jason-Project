#include "WebServerLib.h" // Pastikan ini merujuk ke file header yang benar
#include <Arduino.h>

// Constructor untuk WebServerLib
WebServerLib::WebServerLib() : server(80) { // Inisialisasi WebServer standar pada port 80
    currentPhValue = 0.0;
    currentTdsValue = 0.0;
    currentDistanceRes = 0.0;
    currentDistanceTank = 0.0;
    currentSoilMoisture = 0;
}

// Method untuk memulai koneksi WiFi dan server web
void WebServerLib::begin(const char* ssid, const char* password) {
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Mengatur rute server
    // std::bind digunakan untuk mengikat method dari kelas ini ke handler server
    server.on("/", HTTP_GET, std::bind(&WebServerLib::handleRoot, this));
    server.on("/sensordata", HTTP_GET, std::bind(&WebServerLib::handleSensorData, this));
    
    server.begin(); // Memulai WebServer standar ESP32
    Serial.println("Web server started on port 80.");
}

void WebServerLib::handleClient() {
    server.handleClient(); // Menangani permintaan klien yang masuk
}

void WebServerLib::setSensorValues(float ph, float tds, float distanceRes, float distanceTank, int soilMoisture) {
    currentPhValue = ph;
    currentTdsValue = tds;
    currentDistanceRes = distanceRes;
    currentDistanceTank = distanceTank;
    currentSoilMoisture = soilMoisture;
}

String WebServerLib::getSensorDataJSON() {
    String tdsStatus = (currentTdsValue < 500) ? "Bagus" : "Buruk";
    String phStatus;
    if (currentPhValue < 6.5) phStatus = "Asam";
    else if (currentPhValue <= 7.5) phStatus = "Netral";
    else phStatus = "Basa";
    
    String soilStatus = (currentSoilMoisture < 30) ? "Siram (Kering)" : "Cukup Air (Lembab)"; 

    String json = "{";
    json += "\"distance_res\":" + String(currentDistanceRes, 1) + ",";
    json += "\"distance_tank\":" + String(currentDistanceTank, 1) + ",";
    json += "\"tds\":" + String(currentTdsValue, 2) + ",";
    json += "\"tds_status\":\"" + tdsStatus + "\",";
    json += "\"ph\":" + String(currentPhValue, 2) + ",";
    json += "\"ph_status\":\"" + phStatus + "\",";
    json += "\"soil_moisture\":" + String(currentSoilMoisture) + ",";
    json += "\"soil_moisture_status\":\"" + soilStatus + "\"";
    json += "}";
    return json;
}

void WebServerLib::handleRoot() {
    server.send(200, "text/html", generateHTML());
}

void WebServerLib::handleSensorData() {
    server.send(200, "application/json", getSensorDataJSON());
}

String WebServerLib::generateHTML() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Sistem Monitoring Air Cerdas</title>
  <style>
    body {
      background-color: #1e1e1e;
      font-family: 'Roboto', sans-serif;
      color: #e0e0e0;
      margin: 0;
      padding: 20px;
    }
    .container {
      max-width: 800px;
      margin: 0 auto;
    }
    .card {
      background-color: #2c2c2c;
      border-radius: 8px;
      padding: 20px;
      margin-bottom: 20px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.3);
      text-align: center;
    }
    .header {
      font-size: 1.5em;
      margin-bottom: 15px;
      color: #76ff03; /* Warna hijau utama untuk header */
    }
    .data {
      font-size: 1.2em;
      margin: 10px 0;
    }
    .sub-data {
      font-size: 0.9em;
      color: #b0b0b0;
    }
    svg.arc-gauge { /* Kelas khusus untuk gauge SVG arc */
      width: 100%;
      max-width: 250px; /* Batasi lebar maksimum gauge arc */
      height: auto;
      margin: 0 auto; /* Pusatkan gauge */
    }
    .gauge-text {
      font-size: 1.5em; /* Perbesar teks nilai di tengah gauge */
      fill: #fff;
      dominant-baseline: middle;
      text-anchor: middle;
      font-weight: bold;
    }

    /* Styling untuk linear gauge baru */
    .linear-gauge-container {
      width: 90%;
      margin: 20px auto; /* Beri jarak lebih */
      height: 25px;
      background-color: #444; /* Warna track/latar belakang gauge */
      border-radius: 12px;
      overflow: hidden;
      position: relative;
    }
    .gauge-fill {
      height: 100%;
      background-color: #76ff03; /* Warna hijau untuk isian */
      border-radius: 12px; /* Samakan dengan container atau sedikit lebih kecil */
      transition: width 0.4s ease-out; /* Transisi halus */
      width: 0%; /* Mulai dari 0% */
    }
    .gauge-fill-ph-acid { background-color: #ff5252; } /* Merah untuk pH Asam */
    .gauge-fill-ph-neutral { background-color: #76ff03; } /* Hijau untuk pH Netral */
    .gauge-fill-ph-base { background-color: #448aff; } /* Biru untuk pH Basa */

    .status-good { color: #76ff03; font-weight: bold; }
    .status-bad { color: #ff5252; font-weight: bold; } /* Merah lebih terang */
    .status-acid { color: #ff5252; font-weight: bold; }
    .status-neutral { color: #76ff03; font-weight: bold; }
    .status-base { color: #448aff; font-weight: bold; } /* Biru lebih terang */
    
    .status-irrigation-needed { /* Menggantikan status-on */
      color: #ffa726; /* Oranye untuk butuh siram */
      font-weight: bold;
      animation: blink 1.2s infinite;
    }
    .status-irrigation-ok { /* Menggantikan status-off */
      color: #76ff03;
      font-weight: bold;
    }
    @keyframes blink {
      0% { opacity: 1; }
      50% { opacity: 0.4; }
      100% { opacity: 1; }
    }
  </style>
  <script>
    var MAX_DISTANCE_ULTRASONIC_GAUGE = %MAX_DISTANCE_ULTRASONIC_GAUGE_VAL%;
    var MAX_TDS_GAUGE = %MAX_TDS_GAUGE_VAL%;
    var MAX_PH_GAUGE = %MAX_PH_GAUGE_VAL%;
    var MAX_SOIL_MOISTURE_GAUGE = %MAX_SOIL_MOISTURE_GAUGE_VAL%;
    // Panjang busur untuk setengah lingkaran dengan radius 60 (pi * r)
    // Path: M20,80 A60,60 0 0,1 180,80. Jarak x dari 20 ke 180 adalah 160 (diameter). Radius 80.
    // A rx ry x-axis-rotation large-arc-flag sweep-flag x y
    // Untuk path d="M30,100 A70,70 0 0,1 170,100" (radius 70)
    // Panjang busur adalah pi * 70 = 219.91
    var ARC_LENGTH = 219.91; // Disesuaikan dengan radius baru (misal 70)

    function fetchData() {
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var data = JSON.parse(this.responseText);

          // Water Level Reservoir
          var resLevel = MAX_DISTANCE_ULTRASONIC_GAUGE - data.distance_res;
          resLevel = Math.max(0, Math.min(resLevel, MAX_DISTANCE_ULTRASONIC_GAUGE)); // Clamp value
          var reservoirFraction = resLevel / MAX_DISTANCE_ULTRASONIC_GAUGE;
          // Pastikan fraction antara 0 dan 1
          reservoirFraction = Math.min(Math.max(reservoirFraction, 0), 1);
          document.getElementById("reservoirArc").setAttribute("stroke-dashoffset", ARC_LENGTH * (1 - reservoirFraction));
          document.getElementById("reservoirValue").textContent = resLevel.toFixed(1) + " cm";

          // Water Level Tank
          var tankLevel = MAX_DISTANCE_ULTRASONIC_GAUGE - data.distance_tank;
          tankLevel = Math.max(0, Math.min(tankLevel, MAX_DISTANCE_ULTRASONIC_GAUGE)); // Clamp value
          var tankFraction = tankLevel / MAX_DISTANCE_ULTRASONIC_GAUGE;
          tankFraction = Math.min(Math.max(tankFraction, 0), 1);
          document.getElementById("tankArc").setAttribute("stroke-dashoffset", ARC_LENGTH * (1 - tankFraction));
          document.getElementById("tankValue").textContent = tankLevel.toFixed(1) + " cm";

          // TDS
          document.getElementById("tdsValueDisplay").textContent = data.tds.toFixed(2) + " ppm";
          document.getElementById("tdsStatus").textContent = "Status: " + data.tds_status;
          document.getElementById("tdsStatus").className = "data " + (data.tds_status === "Bagus" ? "status-good" : "status-bad");
          var tdsValueMapped = Math.min(Math.max(data.tds, 0), MAX_TDS_GAUGE);
          var tdsPercentage = (tdsValueMapped / MAX_TDS_GAUGE) * 100;
          document.getElementById("tdsGaugeFill").style.width = tdsPercentage + "%";

          // pH
          document.getElementById("phValueDisplay").textContent = data.ph.toFixed(2);
          document.getElementById("phStatus").textContent = "Klasifikasi: " + data.ph_status;
          var phGaugeFill = document.getElementById("phGaugeFill");
          var phClass = "gauge-fill-ph-neutral"; // Default
          if (data.ph_status === "Asam") {
            document.getElementById("phStatus").className = "data status-acid";
            phClass = "gauge-fill-ph-acid";
          } else if (data.ph_status === "Netral") {
            document.getElementById("phStatus").className = "data status-neutral";
          } else { // Basa
            document.getElementById("phStatus").className = "data status-base";
            phClass = "gauge-fill-ph-base";
          }
          phGaugeFill.className = "gauge-fill " + phClass; // Set kelas warna
          var phValueMapped = Math.min(Math.max(data.ph, 0), MAX_PH_GAUGE);
          var phPercentage = (phValueMapped / MAX_PH_GAUGE) * 100;
          phGaugeFill.style.width = phPercentage + "%";
          

          // Soil Moisture
          document.getElementById("soilMoistureValueDisplay").textContent = data.soil_moisture;
          document.getElementById("soilMoistureStatus").textContent = "Status Irigasi: " + data.soil_moisture_status;
          // Status class untuk teks: Siram (Kering) -> .status-irrigation-needed, Cukup Air (Lembab) -> .status-irrigation-ok
          document.getElementById("soilMoistureStatus").className = "data " + (data.soil_moisture_status.includes("Siram") ? "status-irrigation-needed" : "status-irrigation-ok");
          var soilValueMapped = Math.min(Math.max(data.soil_moisture, 0), MAX_SOIL_MOISTURE_GAUGE);
          // Untuk kelembaban tanah, nilai tinggi berarti kering. Jika ingin bar hijau penuh saat lembab (nilai rendah):
          // var soilPercentage = (1 - (soilValueMapped / MAX_SOIL_MOISTURE_GAUGE)) * 100;
          // Namun, sesuai permintaan "hijaunya akan mengisi gaugemeter perlahan dari kiri sampai ke kanan sampai maksimalnya",
          // maka kita tetap memetakan nilai langsung. Hijau penuh berarti nilai sensor maksimal (kering).
          var soilPercentage = (soilValueMapped / MAX_SOIL_MOISTURE_GAUGE) * 100;
          document.getElementById("soilGaugeFill").style.width = soilPercentage + "%";
        }
      };
      xhr.open("GET", "/sensordata", true);
      xhr.send();
    }
    setInterval(fetchData, 2000);
    window.onload = fetchData;
  </script>
</head>
<body>
  <div class="container">
    <div class="card">
      <div class="header">Tinggi Muka Air Reservoir</div>
      <svg class="arc-gauge" viewBox="0 0 200 110"> <path d="M30,100 A70,70 0 0,1 170,100" stroke="#555" stroke-width="18" fill="none" stroke-linecap="round"/>
        <path id="reservoirArc" d="M30,100 A70,70 0 0,1 170,100" stroke="#76ff03" stroke-width="18" fill="none"
              stroke-dasharray="219.91" stroke-dashoffset="219.91" stroke-linecap="round"/>
        <text id="reservoirValue" x="100" y="85" class="gauge-text">-- cm</text> </svg>
      <div class="data sub-data">Maksimal: )rawliteral" + String(MAX_DISTANCE_ULTRASONIC_GAUGE,0) + R"rawliteral( cm</div>
    </div>

    <div class="card">
      <div class="header">Tinggi Muka Air Tangki</div>
      <svg class="arc-gauge" viewBox="0 0 200 110">
        <path d="M30,100 A70,70 0 0,1 170,100" stroke="#555" stroke-width="18" fill="none" stroke-linecap="round"/>
        <path id="tankArc" d="M30,100 A70,70 0 0,1 170,100" stroke="#4caf50" stroke-width="18" fill="none"
              stroke-dasharray="219.91" stroke-dashoffset="219.91" stroke-linecap="round"/>
        <text id="tankValue" x="100" y="85" class="gauge-text">-- cm</text>
      </svg>
      <div class="data sub-data">Maksimal: )rawliteral" + String(MAX_DISTANCE_ULTRASONIC_GAUGE,0) + R"rawliteral( cm</div>
    </div>

    <div class="card">
      <div class="header">Nilai TDS</div>
      <div id="tdsValueDisplay" class="data">- ppm</div>
      <div id="tdsStatus" class="data">Status: -</div>
      <div class="linear-gauge-container">
        <div id="tdsGaugeFill" class="gauge-fill"></div>
      </div>
      <div class="data sub-data">Batas Ideal: < 500 ppm</div>
    </div>

    <div class="card">
      <div class="header">Tingkat Keasaman pH</div>
      <div id="phValueDisplay" class="data">-</div>
      <div id="phStatus" class="data">Klasifikasi: -</div>
      <div class="linear-gauge-container">
        <div id="phGaugeFill" class="gauge-fill"></div> </div>
      <div class="data sub-data">Ideal: 6.5 - 7.5</div>
    </div>

    <div class="card">
      <div class="header">Kelembaban Tanah</div>
      <div id="soilMoistureValueDisplay" class="data">-</div>
      <div id="soilMoistureStatus" class="data">Status Irigasi: -</div>
      <div class="linear-gauge-container">
        <div id="soilGaugeFill" class="gauge-fill"></div>
      </div>
      <div class="data sub-data">(Nilai Sensor: 0 = Lembab Maks, )rawliteral" + String(MAX_SOIL_MOISTURE_GAUGE,0) + R"rawliteral( = Kering Maks)</div>
    </div>
  </div>
</body>
</html>
)rawliteral";

    html.replace("%MAX_DISTANCE_ULTRASONIC_GAUGE_VAL%", String(MAX_DISTANCE_ULTRASONIC_GAUGE, 1));
    html.replace("%MAX_TDS_GAUGE_VAL%", String(MAX_TDS_GAUGE, 1));
    html.replace("%MAX_PH_GAUGE_VAL%", String(MAX_PH_GAUGE, 1));
    html.replace("%MAX_SOIL_MOISTURE_GAUGE_VAL%", String(MAX_SOIL_MOISTURE_GAUGE, 0));
    return html;
}