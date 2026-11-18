#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <Arduino.h>
#include <WebServer.h>
#include "LinearRegression.h"

/**
 * @brief Snapshot of current sensor readings and Linear Regression forecasts
 *        exposed via the HTTP dashboard and JSON API.
 */
struct SystemTelemetry {
    float currentTemperature;
    float currentHumidity;
    float predictedTemperature;
    float predictedHumidity;
    float tempSlope;
    float tempIntercept;
    float tempRSquared;
    float humSlope;
    float humIntercept;
    float humRSquared;
    size_t sampleCount;
    unsigned long samplingIntervalMs;
};

/**
 * @brief Handles Wi-Fi connection, NTP time synchronization, and HTTP routes
 *        (`/`, `/json`, `/data`).
 */
class WebServerManager {
public:
    WebServerManager(uint16_t port,
                     const char* csvFilePath,
                     const SystemTelemetry& telemetry);

    /**
     * @brief Connect to Wi-Fi and synchronize system clock with NTP server.
     */
    bool connectWiFiAndSyncTime(const char* ssid,
                                const char* password,
                                const char* ntpServer,
                                long gmtOffsetSec,
                                int daylightOffsetSec,
                                unsigned long timeoutMs);

    /**
     * @brief Register HTTP routes and start the web server.
     */
    void begin();

    /**
     * @brief Process incoming HTTP client requests in the main loop.
     */
    void handleClient();

    /**
     * @brief Get current formatted timestamp (`YYYY-MM-DD HH:MM:SS`),
     *        or fallback uptime string if NTP is unavailable.
     */
    static String getFormattedDateTime();

private:
    WebServer server_;
    const char* csvFilePath_;
    const SystemTelemetry& telemetry_;

    void handleRoot();
    void handleJson();
    void handleDataCsv();
};

#endif // WEB_SERVER_MANAGER_H
