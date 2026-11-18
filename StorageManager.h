#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <Arduino.h>
#include "LinearRegression.h"

/**
 * @brief Manages SPIFFS mounting, CSV logging, memory-safe file truncation,
 *        and warm-starting Linear Regression models from stored CSV history.
 */
class StorageManager {
public:
    StorageManager(const char* csvPath,
                   const char* tempCsvPath,
                   size_t maxFileSize,
                   int skipLines);

    /**
     * @brief Mount SPIFFS and create the CSV file with header if absent.
     * @return true if SPIFFS mounted and CSV file is ready.
     */
    bool begin();

    /**
     * @brief Append a timestamped temperature & humidity record to the CSV file.
     *        Automatically truncates oldest rows if file exceeds maxFileSize.
     */
    bool appendReading(const String& datetime, float temperature, float humidity);

    /**
     * @brief Stream-copy CSV excluding the oldest `skipLines_` data rows.
     *        Uses a temporary swap file on SPIFFS to avoid RAM exhaustion.
     */
    bool truncateOldestData();

    /**
     * @brief Load existing readings from CSV on boot to warm-start ML models.
     */
    size_t warmStartModels(LinearRegression& tempModel, LinearRegression& humModel);

    const char* getCsvPath() const { return csvPath_; }

private:
    const char* csvPath_;
    const char* tempCsvPath_;
    size_t maxFileSize_;
    int skipLines_;
};

#endif // STORAGE_MANAGER_H
