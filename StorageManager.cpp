#include "StorageManager.h"
#include <FS.h>
#include <SPIFFS.h>

StorageManager::StorageManager(const char* csvPath,
                               const char* tempCsvPath,
                               size_t maxFileSize,
                               int skipLines)
    : csvPath_(csvPath),
      tempCsvPath_(tempCsvPath),
      maxFileSize_(maxFileSize),
      skipLines_(skipLines) {}

bool StorageManager::begin() {
  if (!SPIFFS.begin(true)) {
    Serial.println("[Storage] Error: Failed to mount SPIFFS.");
    return false;
  }
  Serial.println("[Storage] SPIFFS mounted successfully.");

  if (!SPIFFS.exists(csvPath_)) {
    File file = SPIFFS.open(csvPath_, FILE_WRITE);
    if (!file) {
      Serial.println("[Storage] Error: Unable to create CSV header.");
      return false;
    }
    file.println("Date&Time,Temperature,Humidity");
    file.close();
    Serial.println("[Storage] Created new CSV log file.");
  }
  return true;
}

bool StorageManager::appendReading(const String& datetime, float temperature, float humidity) {
  // Check existing file size before appending
  if (SPIFFS.exists(csvPath_)) {
    File checkFile = SPIFFS.open(csvPath_, FILE_READ);
    if (checkFile) {
      size_t currentSize = checkFile.size();
      checkFile.close();
      if (currentSize >= maxFileSize_) {
        Serial.println("[Storage] Max CSV size reached; truncating oldest entries...");
        truncateOldestData();
      }
    }
  }

  File file = SPIFFS.open(csvPath_, FILE_APPEND);
  if (!file) {
    Serial.println("[Storage] Error: Failed to open CSV file for appending.");
    return false;
  }

  file.printf("%s,%.2f,%.2f\n", datetime.c_str(), temperature, humidity);
  file.close();
  return true;
}

bool StorageManager::truncateOldestData() {
  File srcFile = SPIFFS.open(csvPath_, FILE_READ);
  if (!srcFile) {
    Serial.println("[Storage] Error: Failed to open source CSV for truncation.");
    return false;
  }

  File dstFile = SPIFFS.open(tempCsvPath_, FILE_WRITE);
  if (!dstFile) {
    Serial.println("[Storage] Error: Failed to open temporary CSV for truncation.");
    srcFile.close();
    return false;
  }

  // Write standard CSV header
  dstFile.println("Date&Time,Temperature,Humidity");

  int lineIndex = 0;
  while (srcFile.available()) {
    String line = srcFile.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) {
      continue;
    }
    // Line 0 is the original header; skip header + oldest skipLines_ rows
    if (lineIndex > skipLines_) {
      dstFile.println(line);
    }
    lineIndex++;
  }

  srcFile.close();
  dstFile.close();

  SPIFFS.remove(csvPath_);
  SPIFFS.rename(tempCsvPath_, csvPath_);
  Serial.println("[Storage] CSV truncation completed safely.");
  return true;
}

size_t StorageManager::warmStartModels(LinearRegression& tempModel, LinearRegression& humModel) {
  if (!SPIFFS.exists(csvPath_)) {
    return 0;
  }

  File file = SPIFFS.open(csvPath_, FILE_READ);
  if (!file) {
    return 0;
  }

  size_t loadedCount = 0;
  bool isHeader = true;

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) {
      continue;
    }
    if (isHeader) {
      isHeader = false;
      continue;
    }

    int firstComma = line.indexOf(',');
    int secondComma = line.indexOf(',', firstComma + 1);
    if (firstComma > 0 && secondComma > firstComma) {
      float temp = line.substring(firstComma + 1, secondComma).toFloat();
      float hum = line.substring(secondComma + 1).toFloat();
      tempModel.addSample(temp, false);
      humModel.addSample(hum, false);
      loadedCount++;
    }
  }

  file.close();

  if (loadedCount > 0) {
    tempModel.fit();
    humModel.fit();
  }

  Serial.printf("[Storage] Warm-started Linear Regression models with %u historical samples.\n",
                static_cast<unsigned int>(loadedCount));
  return loadedCount;
}
