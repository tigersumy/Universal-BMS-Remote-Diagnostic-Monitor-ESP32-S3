#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

enum LogLevel {
    LOG_LVL_INFO = 0,
    LOG_LVL_WARN,
    LOG_LVL_ERROR,
    LOG_LVL_TX,
    LOG_LVL_RX
};

struct LogEntry {
    uint32_t timestamp_ms;
    uint8_t  level;
    String   tag;
    String   message;
};

class DebugLogger {
public:
    static void init(size_t maxEntries = 200) {
        if (!s_mutex) {
            s_mutex = xSemaphoreCreateMutex();
        }
        s_maxEntries = maxEntries;
    }

    static void log(uint8_t level, const String& tag, const String& msg) {
        if (!s_mutex) init();
        if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            LogEntry entry;
            entry.timestamp_ms = millis();
            entry.level = level;
            entry.tag = tag;
            entry.message = msg;

            if (s_entries.size() >= s_maxEntries) {
                s_entries.erase(s_entries.begin());
            }
            s_entries.push_back(entry);
            xSemaphoreGive(s_mutex);
        }
        // Also echo to Serial for local USB debugging
        const char* prefix = "[INFO]";
        if (level == LOG_LVL_WARN) prefix = "[WARN]";
        else if (level == LOG_LVL_ERROR) prefix = "[ERR]";
        else if (level == LOG_LVL_TX) prefix = "[TX >>]";
        else if (level == LOG_LVL_RX) prefix = "[RX <<]";
        Serial.printf("%s [%s] %s\n", prefix, tag.c_str(), msg.c_str());
    }

    static void info(const String& tag, const String& msg) {
        log(LOG_LVL_INFO, tag, msg);
    }

    static void warn(const String& tag, const String& msg) {
        log(LOG_LVL_WARN, tag, msg);
    }

    static void error(const String& tag, const String& msg) {
        log(LOG_LVL_ERROR, tag, msg);
    }

    static void logTx(const String& tag, const uint8_t* data, size_t len, const String& desc = "") {
        String hexStr;
        hexStr.reserve(len * 3 + 20);
        for (size_t i = 0; i < len; ++i) {
            char buf[4];
            sprintf(buf, "%02X ", data[i]);
            hexStr += buf;
        }
        if (desc.length() > 0) {
            hexStr += " (" + desc + ")";
        }
        log(LOG_LVL_TX, tag, hexStr);
    }

    static void logRx(const String& tag, const uint8_t* data, size_t len, const String& desc = "") {
        String hexStr;
        hexStr.reserve(len * 3 + 20);
        for (size_t i = 0; i < len; ++i) {
            char buf[4];
            sprintf(buf, "%02X ", data[i]);
            hexStr += buf;
        }
        if (desc.length() > 0) {
            hexStr += " (" + desc + ")";
        }
        log(LOG_LVL_RX, tag, hexStr);
    }

    static void clear() {
        if (!s_mutex) init();
        if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            s_entries.clear();
            xSemaphoreGive(s_mutex);
        }
    }

    static String toJson() {
        if (!s_mutex) init();
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();

        if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            for (const auto& entry : s_entries) {
                JsonObject obj = arr.add<JsonObject>();
                obj["time"] = entry.timestamp_ms;
                obj["lvl"]  = entry.level;
                obj["tag"]  = entry.tag;
                obj["msg"]  = entry.message;
            }
            xSemaphoreGive(s_mutex);
        }

        String json;
        serializeJson(doc, json);
        return json;
    }

private:
    static inline std::vector<LogEntry> s_entries;
    static inline size_t s_maxEntries = 120;
    static inline SemaphoreHandle_t s_mutex = nullptr;
};
