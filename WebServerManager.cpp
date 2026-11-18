#include "WebServerManager.h"
#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <time.h>

WebServerManager::WebServerManager(uint16_t port,
                                   const char* csvFilePath,
                                   const SystemTelemetry& telemetry)
    : server_(port),
      csvFilePath_(csvFilePath),
      telemetry_(telemetry) {}

bool WebServerManager::connectWiFiAndSyncTime(const char* ssid,
                                              const char* password,
                                              const char* ntpServer,
                                              long gmtOffsetSec,
                                              int daylightOffsetSec,
                                              unsigned long timeoutMs) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("[WiFi] Connecting to Wi-Fi");

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startAttempt) < timeoutMs) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n[WiFi] Warning: Connection timed out. Running in offline edge mode.");
    return false;
  }

  Serial.println(" Connected!");
  Serial.print("[WiFi] IP Address: ");
  Serial.println(WiFi.localIP());

  configTime(gmtOffsetSec, daylightOffsetSec, ntpServer);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("[NTP] Warning: Initial NTP sync failed; using uptime fallback until synced.");
    return false;
  }

  Serial.println("[NTP] Time synchronized successfully.");
  return true;
}

void WebServerManager::begin() {
  server_.on("/", HTTP_GET, [this]() { this->handleRoot(); });
  server_.on("/json", HTTP_GET, [this]() { this->handleJson(); });
  server_.on("/data", HTTP_GET, [this]() { this->handleDataCsv(); });

  server_.begin();
  Serial.println("[WebServer] HTTP server started on port 80.");
}

void WebServerManager::handleClient() {
  server_.handleClient();
}

void WebServerManager::handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>IoT-Based Predictive Analysis System Using ESP32</title>";
  html += "<style>body{font-family:sans-serif;max-width:640px;margin:2rem auto;padding:0 1rem;color:#1e293b;background:#f8fafc;}";
  html += ".card{background:#fff;border-radius:8px;padding:1.25rem;margin-bottom:1rem;box-shadow:0 1px 3px rgba(0,0,0,0.1);}";
  html += "h2{margin-top:0;color:#0f172a;}p{margin:0.5rem 0;}.meta{color:#64748b;font-size:0.9rem;}</style></head><body>";

  html += "<div class='card'><h2>ESP32 Edge Predictive Analytics</h2>";
  html += "<p class='meta'>Timestamp: " + getFormattedDateTime() + "</p>";
  html += "<p><strong>Current Temperature:</strong> " + String(telemetry_.currentTemperature, 2) + " &deg;C</p>";
  html += "<p><strong>Current Humidity:</strong> " + String(telemetry_.currentHumidity, 2) + " %</p>";
  html += "<p class='meta'>Active Sampling Interval: " + String(telemetry_.samplingIntervalMs / 1000UL) + " s</p></div>";

  html += "<div class='card'><h2>Linear Regression Forecast (Next 24 Steps)</h2>";
  html += "<p><strong>Predicted Temperature:</strong> " + String(telemetry_.predictedTemperature, 2) + " &deg;C</p>";
  html += "<p><strong>Predicted Humidity:</strong> " + String(telemetry_.predictedHumidity, 2) + " %</p>";
  html += "<p class='meta'>Temp OLS Model: y = " + String(telemetry_.tempSlope, 4) + "x + " +
          String(telemetry_.tempIntercept, 2) + " (R&sup2; = " + String(telemetry_.tempRSquared, 3) + ")</p>";
  html += "<p class='meta'>Humidity OLS Model: y = " + String(telemetry_.humSlope, 4) + "x + " +
          String(telemetry_.humIntercept, 2) + " (R&sup2; = " + String(telemetry_.humRSquared, 3) + ")</p>";
  html += "<p class='meta'>Training Window Samples: " + String(telemetry_.sampleCount) + "</p></div>";

  html += "<p><a href='/json'>JSON API (/json)</a> | <a href='/data'>Download CSV Log (/data)</a></p>";
  html += "</body></html>";

  server_.send(200, "text/html", html);
}

void WebServerManager::handleJson() {
  String json = "{";
  json += "\"date\":\"" + getFormattedDateTime() + "\",";
  json += "\"currentTemperature\":" + String(telemetry_.currentTemperature, 2) + ",";
  json += "\"currentHumidity\":" + String(telemetry_.currentHumidity, 2) + ",";
  json += "\"predictedTemperature\":" + String(telemetry_.predictedTemperature, 2) + ",";
  json += "\"predictedHumidity\":" + String(telemetry_.predictedHumidity, 2) + ",";
  json += "\"modelMetrics\":{";
  json += "\"sampleCount\":" + String(telemetry_.sampleCount) + ",";
  json += "\"samplingIntervalMs\":" + String(telemetry_.samplingIntervalMs) + ",";
  json += "\"temperatureSlope\":" + String(telemetry_.tempSlope, 4) + ",";
  json += "\"temperatureIntercept\":" + String(telemetry_.tempIntercept, 2) + ",";
  json += "\"temperatureRSquared\":" + String(telemetry_.tempRSquared, 3) + ",";
  json += "\"humiditySlope\":" + String(telemetry_.humSlope, 4) + ",";
  json += "\"humidityIntercept\":" + String(telemetry_.humIntercept, 2) + ",";
  json += "\"humidityRSquared\":" + String(telemetry_.humRSquared, 3);
  json += "}}";

  server_.send(200, "application/json", json);
}

void WebServerManager::handleDataCsv() {
  File file = SPIFFS.open(csvFilePath_, FILE_READ);
  if (!file) {
    server_.send(500, "text/plain", "Failed to open CSV file");
    return;
  }
  server_.streamFile(file, "text/csv");
  file.close();
}

String WebServerManager::getFormattedDateTime() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 10)) {
    char datetime[25];
    strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(datetime);
  }
  // Fallback to uptime seconds if NTP is unreachable
  unsigned long totalSeconds = millis() / 1000UL;
  char fallback[25];
  snprintf(fallback, sizeof(fallback), "UPTIME+%lus", totalSeconds);
  return String(fallback);
}
