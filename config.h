#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <DHT.h>

// ============================================================================
// Wi-Fi Credentials (Loaded from secrets.h if present; otherwise placeholders)
// ============================================================================
#if __has_include("secrets.h")
#include "secrets.h"
#else
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

// ============================================================================
// Hardware Configuration
// ============================================================================
#define DHT_PIN 26
#define DHT_TYPE DHT11
#define SERIAL_BAUD_RATE 115200

// ============================================================================
// Adaptive Sampling Schedule Configuration
// ============================================================================
// Phase 1: First 600 readings every 10 seconds (~1.67 hours rapid warmup)
// Phase 2: Next 1,008 readings every 10 minutes (7 days medium-resolution)
// Phase 3: Subsequent readings every 1 hour (long-term steady state)
#define PHASE1_READING_LIMIT 600
#define PHASE2_READING_LIMIT 1608

#define INTERVAL_PHASE1_MS 10000UL    // 10 seconds
#define INTERVAL_PHASE2_MS 600000UL   // 10 minutes
#define INTERVAL_PHASE3_MS 3600000UL  // 1 hour

// ============================================================================
// SPIFFS Storage Configuration
// ============================================================================
#define CSV_FILE_PATH "/data.csv"
#define TEMP_CSV_FILE_PATH "/data_tmp.csv"
#define MAX_CSV_FILE_SIZE (1024UL * 1024UL)  // 1 MB max file size
#define TRUNCATE_SKIP_LINES 50               // Oldest lines to drop when full

// ============================================================================
// NTP & Time Configuration
// ============================================================================
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 21600          // UTC+6 (Bangladesh Standard Time)
#define DAYLIGHT_OFFSET_SEC 0
#define WIFI_CONNECT_TIMEOUT_MS 15000UL

// ============================================================================
// Edge Linear Regression Configuration
// ============================================================================
#define LR_WINDOW_SIZE 48             // Rolling window size (samples stored in RAM)
#define LR_MIN_SAMPLES 2              // Minimum samples required to fit trendline
#define LR_FORECAST_STEPS 24.0f       // Forecast 24 steps ahead (24h in steady state)
#define LR_RIDGE_LAMBDA 1.0f          // L2 regularization to stabilize early slopes

// Physical clamping bounds for realistic environmental predictions
#define TEMP_MIN_BOUND -10.0f
#define TEMP_MAX_BOUND 60.0f
#define HUM_MIN_BOUND 0.0f
#define HUM_MAX_BOUND 100.0f

#endif // CONFIG_H
