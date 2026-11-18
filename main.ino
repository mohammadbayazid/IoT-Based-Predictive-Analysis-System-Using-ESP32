/**
 * @file main.ino
 * @brief Main entry point for the ESP32 Edge-Based Temperature & Humidity
 *        Predictive Analytics System using On-Device Linear Regression.
 */

#include <Arduino.h>
#include <DHT.h>
#include "config.h"
#include "LinearRegression.h"
#include "StorageManager.h"
#include "WebServerManager.h"

// Hardware sensor instance
DHT dht(DHT_PIN, DHT_TYPE);

// On-device Ordinary Least Squares (OLS) Linear Regression models
LinearRegression tempModel(LR_WINDOW_SIZE, LR_RIDGE_LAMBDA, TEMP_MIN_BOUND, TEMP_MAX_BOUND);
LinearRegression humModel(LR_WINDOW_SIZE, LR_RIDGE_LAMBDA, HUM_MIN_BOUND, HUM_MAX_BOUND);

// SPIFFS CSV storage manager
StorageManager storage(CSV_FILE_PATH, TEMP_CSV_FILE_PATH, MAX_CSV_FILE_SIZE, TRUNCATE_SKIP_LINES);

// Shared telemetry state for HTTP & JSON endpoints
SystemTelemetry telemetry = {
  0.0f, 0.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 0.0f,
  0, INTERVAL_PHASE1_MS
};

// Web server & network manager on port 80
WebServerManager webServer(80, CSV_FILE_PATH, telemetry);

// Adaptive sampling state
unsigned long lastReadingTime = 0;
unsigned int readingCount = 0;
unsigned long delayTime = INTERVAL_PHASE1_MS;
bool initialSampleTaken = false;

/**
 * @brief Compute the next sampling interval based on total readings collected.
 */
unsigned long getAdaptiveInterval(unsigned int count) {
  if (count < PHASE1_READING_LIMIT) {
    return INTERVAL_PHASE1_MS;  // First 600 readings: every 10 seconds
  } else if (count < PHASE2_READING_LIMIT) {
    return INTERVAL_PHASE2_MS;  // Next 1,008 readings: every 10 minutes
  }
  return INTERVAL_PHASE3_MS;    // Subsequent readings: every 1 hour
}

/**
 * @brief Refresh telemetry struct from the latest sensor readings and ML models.
 */
void updateTelemetryState(float temperature, float humidity) {
  telemetry.currentTemperature = temperature;
  telemetry.currentHumidity = humidity;
  telemetry.predictedTemperature = tempModel.predict(LR_FORECAST_STEPS);
  telemetry.predictedHumidity = humModel.predict(LR_FORECAST_STEPS);

  telemetry.tempSlope = tempModel.getSlope();
  telemetry.tempIntercept = tempModel.getIntercept();
  telemetry.tempRSquared = tempModel.getRSquared();

  telemetry.humSlope = humModel.getSlope();
  telemetry.humIntercept = humModel.getIntercept();
  telemetry.humRSquared = humModel.getRSquared();

  telemetry.sampleCount = tempModel.getSampleCount();
  telemetry.samplingIntervalMs = delayTime;
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  dht.begin();

  // 1. Mount SPIFFS and warm-start Linear Regression models from saved CSV data
  if (storage.begin()) {
    size_t loaded = storage.warmStartModels(tempModel, humModel);
    if (loaded > 0) {
      readingCount = static_cast<unsigned int>(loaded);
      delayTime = getAdaptiveInterval(readingCount);
      updateTelemetryState(tempModel.getLatestSample(), humModel.getLatestSample());
    }
  }

  // 2. Connect to Wi-Fi and synchronize clock via NTP
  webServer.connectWiFiAndSyncTime(
    WIFI_SSID,
    WIFI_PASSWORD,
    NTP_SERVER,
    GMT_OFFSET_SEC,
    DAYLIGHT_OFFSET_SEC,
    WIFI_CONNECT_TIMEOUT_MS
  );

  // 3. Start HTTP server (/ , /json , /data)
  webServer.begin();
}

void loop() {
  webServer.handleClient();

  unsigned long currentTime = millis();
  if (!initialSampleTaken || (currentTime - lastReadingTime >= delayTime)) {
    lastReadingTime = currentTime;
    initialSampleTaken = true;

    // Acquire temperature and humidity from DHT sensor
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("[Sensor] Warning: Failed to read from DHT sensor!");
      return;
    }

    readingCount++;
    delayTime = getAdaptiveInterval(readingCount);

    // Train on-device Linear Regression models with new sample
    tempModel.addSample(temperature);
    humModel.addSample(humidity);

    // Update predictions and model diagnostics
    updateTelemetryState(temperature, humidity);

    // Format timestamp and log to Serial & SPIFFS CSV
    String datetime = WebServerManager::getFormattedDateTime();
    Serial.printf(
      "[%s] Temp: %.2f C (Pred: %.2f C, Slope: %.4f) | Hum: %.2f %% (Pred: %.2f %%, Slope: %.4f)\n",
      datetime.c_str(),
      temperature,
      telemetry.predictedTemperature,
      telemetry.tempSlope,
      humidity,
      telemetry.predictedHumidity,
      telemetry.humSlope
    );

    storage.appendReading(datetime, temperature, humidity);
  }
}
