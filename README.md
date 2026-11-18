# IoT-Based Predictive Analysis System Using ESP32

An edge-computing environmental telemetry and predictive analytics firmware built for the **ESP32** microcontroller and **DHT11/DHT22** sensor suite. The system performs **on-device Ordinary Least Squares (OLS) Linear Regression** with L2 (Ridge) regularization over a rolling circular buffer, logs long-term historical telemetry to **SPIFFS** flash storage using a three-phase adaptive sampling scheduler, and serves live forecasts via an embedded HTTP dashboard and JSON REST API.

---

## Core System Capabilities

- **On-Device Regularized OLS Forecasting (`LinearRegression.h` / `.cpp`):** Fits a continuous linear trendline (`y = β₁x + β₀`) with L2 regularization across a 48-sample rolling circular buffer, computes R² goodness-of-fit diagnostics in real time, and projects 24 steps ahead with physical range clamping.

- **Three-Phase Adaptive Sampling (`main.ino` / `config.h`):** Automatically transitions from rapid 10-second warmup polling to 10-minute medium-resolution profiling and 1-hour steady-state logging to balance fast model convergence with flash memory longevity.

- **Stream-Based Flash Persistence & Warm-Start (`StorageManager.h` / `.cpp`):** Persists timestamped telemetry to `/data.csv` on SPIFFS, performs low-memory stream truncation via a swap file (`/data_tmp.csv`) when storage reaches capacity, and warm-starts regression buffers from historical records on boot.

- **Embedded Web Dashboard & REST API (`WebServerManager.h` / `.cpp`):** Hosts a responsive HTML status dashboard (`/`), a machine-readable JSON telemetry and model diagnostics endpoint (`/json`), and direct CSV dataset streaming (`/data`) with NTP time synchronization.

- **Isolated Credential Configuration (`secrets.example.h` / `.gitignore`):** Separates local network credentials (`secrets.h`) from application logic via `#if __has_include("secrets.h")` compile-time resolution.

---

## Repository Structure

| File | Responsibility |
| :--- | :--- |
| `main.ino` | Application entry point (`setup()` and `loop()`), adaptive sampling scheduler, and telemetry orchestration |
| `config.h` | Hardware GPIO definitions, adaptive sampling thresholds, SPIFFS limits, NTP settings, and regression hyperparameters |
| `secrets.example.h` | Template for local Wi-Fi credentials (`WIFI_SSID` and `WIFI_PASSWORD`) |
| `LinearRegression.h` | Interface for the circular-buffer OLS Linear Regression engine |
| `LinearRegression.cpp` | Implementation of L2-stabilized OLS slope, intercept, R² coefficient of determination, and clamped multi-step forecasting |
| `StorageManager.h` | Interface for SPIFFS filesystem initialization, CSV logging, and historical data recovery |
| `StorageManager.cpp` | Stream-based CSV rotation and boot-time regression model warm-start |
| `WebServerManager.h` | Interface for Wi-Fi station management, NTP synchronization, and HTTP routing |
| `WebServerManager.cpp` | Embedded HTML UI (`/`), JSON diagnostics payload (`/json`), and SPIFFS CSV streaming (`/data`) |
| `platformio.ini` | PlatformIO build configuration (`esp32dev` board target and Adafruit DHT library dependencies) |
| `.gitignore` | Excludes `secrets.h`, compiled binaries, and IDE metadata from version control |

---

## Hardware Requirements & Pin Mapping

| Component | ESP32 Pin / Value | Notes |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32 Development Board | Requires 4 MB Flash with SPIFFS partition enabled |
| **Sensor** | DHT11 Temperature & Humidity | Configurable to `DHT22` via `DHT_TYPE` in `config.h` |
| **Data Signal** | `GPIO 26` (`DHT_PIN`) | Connect a 4.7 kΩ – 10 kΩ pull-up resistor between DATA and VCC if using a bare sensor |
| **Power** | `3.3V` and `GND` | Connect to ESP32 `3V3` and `GND` rails |

---

## Adaptive Sampling & Predictive Methodology

### 1. Three-Phase Adaptive Sampling Schedule

To capture rapid initial training data while preserving SPIFFS flash write endurance over multi-week deployments, the sampling period scales automatically based on cumulative valid observations:

| Phase | Cumulative Reading Count | Sampling Interval | Operational Purpose |
| :--- | :--- | :--- | :--- |
| **Phase 1** | `0 – 599` (First 600 samples) | `10 seconds` | Rapid initial data acquisition and regression buffer initialization (~1.67 hours) |
| **Phase 2** | `600 – 1,607` (Next 1,008 samples) | `10 minutes` | Medium-resolution diurnal environmental profiling across 7 days |
| **Phase 3** | `1,608+` (Subsequent samples) | `1 hour` | Long-term steady-state monitoring and 24-hour lookahead forecasting |

### 2. L2-Regularized Ordinary Least Squares (OLS) Model

Given a rolling window of `n` sequential observations `(xᵢ, yᵢ)` for `i = 0, 1, ..., n - 1` where `xᵢ = i` (`n ≤ 48`), the `LinearRegression` engine evaluates the sample means:

```text
x̄ = (n - 1) / 2
ȳ = (1 / n) * Σ yᵢ
```

The trend slope `β₁` and intercept `β₀` are computed with an L2 regularization parameter `λ` (`LR_RIDGE_LAMBDA = 1.0`) to dampen high-frequency sensor quantization noise across short windows:

```text
β₁ = Σ[(xᵢ - x̄)(yᵢ - ȳ)] / (Σ[(xᵢ - x̄)²] + λ)
β₀ = ȳ - (β₁ * x̄)
```

Goodness-of-fit is tracked continuously via the coefficient of determination (`R²`):

```text
R² = clamp(1 - [Σ(yᵢ - ŷᵢ)² / Σ(yᵢ - ȳ)²], 0.0, 1.0)
```

Forecasts at horizon `k` steps ahead (`LR_FORECAST_STEPS = 24.0`) are evaluated at `x_pred = (n - 1) + k` and constrained to physical sensor bounds `[y_min, y_max]`:

```text
ŷ(n - 1 + k) = clamp(β₁ * (n - 1 + k) + β₀, y_min, y_max)
```

---

## Getting Started

### 1. Clone the Repository

```bash
git clone https://github.com/mohammadbayazid/IoT-Based-Predictive-Analysis-System-Using-ESP32.git
cd IoT-Based-Predictive-Analysis-System-Using-ESP32
```

### 2. Configure Network Credentials

Create a local `secrets.h` header from the provided template:

```bash
cp secrets.example.h secrets.h
```

Populate `secrets.h` with your Wi-Fi credentials:

```cpp
#define WIFI_SSID "Your_WiFi_SSID"
#define WIFI_PASSWORD "Your_WiFi_Password"
```

### 3. Build and Flash Firmware

#### Option A: PlatformIO (Recommended)

```bash
pio run --target upload
pio device monitor -b 115200
```

#### Option B: Arduino IDE

1. Ensure the project folder is named `main` (matching `main.ino`) when opening in Arduino IDE.

2. Install the **ESP32 Board Support Package** and the **DHT sensor library** (`adafruit/DHT sensor library` + `Adafruit Unified Sensor`).

3. Select **ESP32 Dev Module** with a default SPIFFS partition scheme and upload.

---

## HTTP Dashboard & REST API

Upon connecting to Wi-Fi, the ESP32 logs its assigned IPv4 address to the Serial Monitor (`115200` baud) and serves three HTTP endpoints on port `80`:

| Endpoint | Method | Content-Type | Description |
| :--- | :--- | :--- | :--- |
| `/` | `GET` | `text/html` | Live web dashboard showing current readings, 24-step forecasts, and OLS regression equations (β₁, β₀, R²) |
| `/json` | `GET` | `application/json` | Structured JSON telemetry payload containing real-time measurements, forecasts, and model metrics |
| `/data` | `GET` | `text/csv` | Streams the persistent `/data.csv` historical telemetry log directly from SPIFFS |

### Sample `/json` Payload

```json
{
  "date": "2026-11-18 00:25:00",
  "currentTemperature": 27.40,
  "currentHumidity": 64.20,
  "predictedTemperature": 28.15,
  "predictedHumidity": 62.80,
  "modelMetrics": {
    "sampleCount": 48,
    "samplingIntervalMs": 3600000,
    "temperatureSlope": 0.0312,
    "temperatureIntercept": 25.93,
    "temperatureRSquared": 0.892,
    "humiditySlope": -0.0583,
    "humidityIntercept": 66.94,
    "humidityRSquared": 0.847
  }
}
```
