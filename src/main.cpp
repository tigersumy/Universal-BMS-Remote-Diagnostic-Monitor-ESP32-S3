#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <ArduinoJson.h>

#include "Config.h"
#include "BmsProtocol.h"
#include "BmsBleClient.h"
#include "DebugLogger.h"
#include "WebDashboard.h"

#if ENABLE_TAILSCALE
extern "C" {
#include "microlink.h"
#include "microlink_internal.h"
}
#endif

// Hardware pins
#define BOOT_BUTTON_PIN 0
#define DNS_PORT 53

static WebServer server(80);
static DNSServer dnsServer;
static BmsBleClient bleClient;
static AppConfig currentConfig;
static bool isApMode = false;
static uint32_t bootPressStart = 0;

#if ENABLE_TAILSCALE
static microlink_t* mlHandle = nullptr;
static microlink_state_t tsState = ML_STATE_IDLE;
static String tsVpnIpStr = "";
static String tsStatusStr = "IDLE";

static void onTailscaleStateChange(microlink_t* ml, microlink_state_t state, void* user_data) {
    tsState = state;
    const char *state_names[] = {
        "IDLE", "WIFI_WAIT", "CONNECTING", "REGISTERING",
        "CONNECTED", "RECONNECTING", "ERROR"
    };
    tsStatusStr = (state < sizeof(state_names)/sizeof(state_names[0])) ? state_names[state] : "UNKNOWN";
    DebugLogger::info("TAILSCALE", "State: " + tsStatusStr);

    if (state == ML_STATE_CONNECTED) {
        uint32_t ip = microlink_get_vpn_ip(ml);
        char ip_str[16];
        microlink_ip_to_str(ip, ip_str);
        tsVpnIpStr = String(ip_str);
        DebugLogger::info("TAILSCALE", "Connected to Tailnet! VPN IP: " + tsVpnIpStr);
    }
}

void startTailscaleClient() {
    if (!currentConfig.ts_enabled || currentConfig.ts_auth_key.length() == 0) {
        DebugLogger::warn("TAILSCALE", "Disabled or no Auth Key configured");
        return;
    }
    if (mlHandle != nullptr) {
        DebugLogger::info("TAILSCALE", "Already running");
        return;
    }

    DebugLogger::info("TAILSCALE", "Starting Tailscale client (hostname: '" + currentConfig.ts_hostname + "')...");

    microlink_config_t config = {
        .auth_key = currentConfig.ts_auth_key.c_str(),
        .device_name = currentConfig.ts_hostname.length() > 0 ? currentConfig.ts_hostname.c_str() : "jbd-bms-probe",
        .enable_derp = true,
        .enable_stun = true,
        .enable_disco = true,
        .max_peers = 32,
        .wifi_tx_power_dbm = 13,
        .priority_peer_ip = 0,
        .disco_heartbeat_ms = 0,
        .stun_interval_ms = 0,
        .ctrl_watchdog_ms = 0
    };

    mlHandle = microlink_init(&config);
    if (!mlHandle) {
        DebugLogger::error("TAILSCALE", "Failed to initialize MicroLink!");
        tsStatusStr = "INIT_FAILED";
        return;
    }

    microlink_set_state_callback(mlHandle, onTailscaleStateChange, NULL);
    esp_err_t err = microlink_start(mlHandle);
    if (err != ESP_OK) {
        DebugLogger::error("TAILSCALE", "microlink_start returned error: " + String(err));
        tsStatusStr = "START_FAILED";
    } else {
        DebugLogger::info("TAILSCALE", "MicroLink started in background");
        tsStatusStr = "CONNECTING";
    }
}

void stopTailscaleClient() {
    if (mlHandle != nullptr) {
        DebugLogger::info("TAILSCALE", "Stopping and cleaning up MicroLink client...");
        microlink_destroy(mlHandle);
        mlHandle = nullptr;
        tsState = ML_STATE_IDLE;
        tsStatusStr = "STOPPED";
        tsVpnIpStr = "";
        DebugLogger::info("TAILSCALE", "MicroLink client stopped");
    }
}

void restartTailscaleClient() {
    DebugLogger::info("TAILSCALE", "Soft-restarting Tailscale client...");
    stopTailscaleClient();
    delay(500);
    startTailscaleClient();
}
#endif

// Captive Portal detection
bool isCaptivePortalRequest() {
    if (!isApMode) return false;
    String host = server.hostHeader();
    if (host.length() == 0 || host.indexOf("192.168.4.1") >= 0) {
        return false;
    }
    return true;
}

void setupWebServerRoutes() {
    server.on("/", HTTP_GET, []() {
        if (isApMode || currentConfig.wifi_ssid.length() == 0) {
            server.send_P(200, "text/html", SETUP_HTML);
        } else {
            server.send_P(200, "text/html", INDEX_HTML);
        }
    });

    server.on("/setup", HTTP_GET, []() {
        server.send_P(200, "text/html", SETUP_HTML);
    });

    server.on("/settings", HTTP_GET, []() {
        server.send_P(200, "text/html", SETUP_HTML);
    });

    // Captive Portal Redirects
    server.on("/generate_204", HTTP_GET, []() {
        server.sendHeader("Location", "http://192.168.4.1/setup", true);
        server.send(302, "text/plain", "");
    });
    server.on("/hotspot-detect.html", HTTP_GET, []() {
        server.sendHeader("Location", "http://192.168.4.1/setup", true);
        server.send(302, "text/plain", "");
    });
    server.on("/canonical.html", HTTP_GET, []() {
        server.sendHeader("Location", "http://192.168.4.1/setup", true);
        server.send(302, "text/plain", "");
    });

    auto sendFavicon = []() {
        server.sendHeader("Cache-Control", "public, max-age=604800");
        server.send_P(200, "image/svg+xml", FAVICON_SVG);
    };
    server.on("/favicon.ico", HTTP_GET, sendFavicon);
    server.on("/favicon.svg", HTTP_GET, sendFavicon);

    // API: Live Telemetry
    server.on("/api/data", HTTP_GET, []() {
        const auto& t = bleClient.getTelemetry();
        JsonDocument doc;
        doc["connected"] = bleClient.isConnected();
        doc["bms_type"]  = t.bms_type;
        doc["device_name"] = t.device_name;
        doc["mac"] = t.mac_address;
        doc["total_voltage"] = t.total_voltage;
        doc["current"] = t.current;
        doc["power"] = t.power;
        doc["charge_power"] = t.charge_power;
        doc["discharge_power"] = t.discharge_power;
        doc["soc"] = t.soc;
        doc["capacity_remain"] = t.capacity_remain;
        doc["cycle_count"] = t.cycle_count;
        doc["cell_count"] = t.cell_count;

        JsonArray cells = doc["cells"].to<JsonArray>();
        for (int i = 0; i < t.cell_count && i < 32; i++) {
            cells.add(t.cell_voltages[i]);
        }
        doc["min_cell_idx"] = t.min_cell_idx;
        doc["max_cell_idx"] = t.max_cell_idx;
        doc["min_cell_v"]   = t.min_cell_v;
        doc["max_cell_v"]   = t.max_cell_v;
        doc["delta_cell_v"] = t.delta_cell_v;

        doc["temp_mos"] = t.temp_mos;
        doc["temp_sensor1"] = t.temp_sensor1;
        doc["temp_sensor2"] = t.temp_sensor2;

        doc["switch_charging"] = t.switch_charging;
        doc["switch_discharging"] = t.switch_discharging;
        doc["switch_balancer"] = t.switch_balancer;

        doc["errors"] = t.errors_str;

#if ENABLE_TAILSCALE
        doc["ts_enabled"] = currentConfig.ts_enabled;
        doc["ts_connected"] = (tsState == ML_STATE_CONNECTED);
        doc["ts_ip"] = tsVpnIpStr;
        doc["ts_status"] = tsStatusStr;
        if (mlHandle) {
            doc["ts_derp_conn"] = mlHandle->derp.connected;
            doc["ts_peer_cnt"] = mlHandle->peer_count;
            doc["ts_derp_region"] = mlHandle->derp_home_region;
        }
#else
        doc["ts_enabled"] = false;
        doc["ts_connected"] = false;
        doc["ts_ip"] = "";
        doc["ts_status"] = "NOT_SUPPORTED";
#endif

        doc["heap_free"] = ESP.getFreeHeap() / 1024;
        doc["psram_free"] = ESP.getFreePsram() / 1024;

        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    });

    // API: Config
    server.on("/api/config", HTTP_GET, []() {
        JsonDocument doc;
        doc["ssid"] = currentConfig.wifi_ssid;
        doc["mac"]  = currentConfig.bms_mac;
        doc["name"] = currentConfig.bms_name;
        doc["bms_type"] = currentConfig.bms_type;
        doc["pin"]  = currentConfig.bms_pin;
        doc["cells"] = currentConfig.cell_count;
        doc["ts_hostname"] = currentConfig.ts_hostname;
        doc["has_ts_key"] = (currentConfig.ts_auth_key.length() > 0);
        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    });

    // API: Live Debug Log (Remote Diagnostics)
    server.on("/api/debug-log", HTTP_GET, []() {
        server.send(200, "application/json", DebugLogger::toJson());
    });

    // API: Clear Debug Log
    server.on("/api/clear-log", HTTP_POST, []() {
        DebugLogger::clear();
        server.send(200, "application/json", "{\"status\":\"cleared\"}");
    });

    // API: Reconnect BLE
    server.on("/api/reconnect-ble", HTTP_POST, []() {
        bleClient.reconnect();
        server.send(200, "application/json", "{\"status\":\"reconnecting\"}");
    });

    // API: Send Raw HEX command over BLE
    server.on("/api/send-raw-ble", HTTP_POST, []() {
        if (!server.hasArg("plain")) {
            server.send(400, "application/json", "{\"error\":\"Missing body\"}");
            return;
        }
        JsonDocument doc;
        deserializeJson(doc, server.arg("plain"));
        String hexStr = doc["hex"] | "";
        hexStr.replace(" ", "");
        hexStr.replace("0x", "");
        hexStr.replace("0X", "");

        if (hexStr.length() == 0 || (hexStr.length() % 2 != 0)) {
            server.send(400, "application/json", "{\"error\":\"Invalid HEX length\"}");
            return;
        }

        size_t byteCount = hexStr.length() / 2;
        std::vector<uint8_t> buf(byteCount);
        for (size_t i = 0; i < byteCount; ++i) {
            char bStr[3] = { hexStr[i * 2], hexStr[i * 2 + 1], 0 };
            buf[i] = (uint8_t)strtoul(bStr, nullptr, 16);
        }

        bool ok = bleClient.sendRawBle(buf.data(), byteCount);
        if (ok) {
            server.send(200, "application/json", "{\"status\":\"ok\",\"bytes\":" + String(byteCount) + "}");
        } else {
            server.send(500, "application/json", "{\"status\":\"error\",\"error\":\"Send failed or not connected\"}");
        }
    });

    // API: Toggle Switches
    server.on("/api/switch", HTTP_POST, []() {
        if (!server.hasArg("plain")) {
            server.send(400, "application/json", "{\"error\":\"Missing body\"}");
            return;
        }
        JsonDocument doc;
        deserializeJson(doc, server.arg("plain"));
        const char* sw = doc["switch"];
        bool state = doc["state"];

        bool ok = bleClient.setSwitch(String(sw), state);
        if (ok) {
            server.send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            server.send(500, "application/json", "{\"status\":\"error\"}");
        }
    });

    // API: Wi-Fi Scan
    server.on("/api/scan-wifi", HTTP_GET, []() {
        int n = WiFi.scanNetworks(false, false);
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < n; i++) {
            JsonObject item = arr.add<JsonObject>();
            item["ssid"] = WiFi.SSID(i);
            item["rssi"] = WiFi.RSSI(i);
        }
        WiFi.scanDelete();
        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    });

    // API: BLE Scan
    server.on("/api/scan-ble", HTTP_GET, []() {
        auto list = bleClient.getDiscoveredDevices();
        if (list.empty()) {
            bleClient.performScanSync(4);
            list = bleClient.getDiscoveredDevices();
        }
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (const auto& item : list) {
            JsonObject obj = arr.add<JsonObject>();
            obj["name"] = item.name;
            obj["mac"]  = item.address;
            obj["rssi"] = item.rssi;
            obj["type"] = item.bms_type;
        }
        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    });

    // API: Save Config
    server.on("/api/save-config", HTTP_POST, []() {
        if (!server.hasArg("plain")) {
            server.send(400, "application/json", "{\"error\":\"Missing body\"}");
            return;
        }
        JsonDocument doc;
        deserializeJson(doc, server.arg("plain"));

        AppConfig cfg;
        cfg.wifi_ssid  = doc["ssid"].as<String>();
        cfg.wifi_pass  = doc["pass"].as<String>();
        cfg.bms_mac    = doc["mac"].as<String>();
        cfg.bms_name   = doc["name"] | "BMS Device";
        cfg.bms_type   = doc["bms_type"] | BMS_TYPE_AUTO;
        cfg.bms_pin    = doc["pin"] | "123456";
        cfg.cell_count = doc["cells"] | 4;

        cfg.ts_enabled = doc["ts_enabled"] | true;
        cfg.ts_hostname = doc["ts_hostname"] | "jbd-bms-probe";
        if (doc["ts_auth_key"].is<String>() && doc["ts_auth_key"].as<String>().length() > 0) {
            cfg.ts_auth_key = doc["ts_auth_key"].as<String>();
        } else {
            cfg.ts_auth_key = currentConfig.ts_auth_key;
        }

        ConfigManager::save(cfg);
        server.send(200, "application/json", "{\"status\":\"saved\"}");
        delay(1000);
        ESP.restart();
    });

    // API: Soft-restart Tailscale
    server.on("/api/restart-tailscale", HTTP_POST, []() {
#if ENABLE_TAILSCALE
        server.send(200, "application/json", "{\"status\":\"restarting\"}");
        restartTailscaleClient();
#else
        server.send(400, "application/json", "{\"error\":\"Tailscale not enabled\"}");
#endif
    });

    // API: Reboot
    server.on("/api/reboot", HTTP_POST, []() {
        server.send(200, "application/json", "{\"status\":\"rebooting\"}");
        delay(500);
        ESP.restart();
    });

    // Web OTA update
    server.on("/update", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        delay(500);
        ESP.restart();
    }, []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            DebugLogger::info("OTA", "Update Start: " + upload.filename);
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                DebugLogger::info("OTA", "Success: " + String(upload.totalSize) + " bytes");
            } else {
                Update.printError(Serial);
            }
        }
    });

    server.onNotFound([]() {
        if (isCaptivePortalRequest()) {
            server.sendHeader("Location", "http://192.168.4.1/setup", true);
            server.send(302, "text/plain", "");
        } else {
            server.send(404, "text/plain", "Not Found");
        }
    });
}

void startApMode() {
    isApMode = true;
    WiFi.persistent(false);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    IPAddress apIP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(apIP, gateway, subnet);
    bool apOk = WiFi.softAP("Universal-BMS-Setup", nullptr, 1, 0, 4);
    delay(200);
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", apIP);
    DebugLogger::info("WIFI", "Started AP Mode: 'Universal-BMS-Setup' (192.168.4.1), status: " + String(apOk ? "OK" : "ERR"));
}

void checkBootButton() {
    if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
        if (bootPressStart == 0) {
            bootPressStart = millis();
        } else if (millis() - bootPressStart > 4000) {
            DebugLogger::warn("BOOT", "Held > 4s! Factory resetting configuration...");
            ConfigManager::clear();
            delay(300);
            ESP.restart();
        }
    } else {
        if (bootPressStart != 0) {
            bootPressStart = 0;
        }
    }
}

static void bleWorkerTask(void* param) {
    DebugLogger::info("BLE", "BLE Worker Task started on background FreeRTOS thread");
    while (true) {
        bleClient.loop();
        vTaskDelay(pdMS_TO_TICKS(15));
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    DebugLogger::init(120);

    Serial.println("\n=========================================");
    Serial.println("  Universal BMS Diagnostic Probe (ESP32-S3) ");
    Serial.println("=========================================");

    char sysBuf[128];
    sprintf(sysBuf, "Chip: %s | CPU: %d MHz | Free Heap: %d KB | PSRAM: %d KB / %d KB",
            ESP.getChipModel(), ESP.getCpuFreqMHz(),
            ESP.getFreeHeap() / 1024, ESP.getFreePsram() / 1024, ESP.getPsramSize() / 1024);
    DebugLogger::info("SYSTEM", sysBuf);

    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

    currentConfig = ConfigManager::load();
    DebugLogger::info("CONFIG", "SSID: '" + currentConfig.wifi_ssid + "', MAC: '" + currentConfig.bms_mac + 
                                "', Cells: " + String(currentConfig.cell_count) + "S, PIN: '" + currentConfig.bms_pin + "'");

    bleClient.init();
    bleClient.setTargetConfig(currentConfig);

    if (currentConfig.wifi_ssid.length() > 0) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(currentConfig.wifi_ssid.c_str(), currentConfig.wifi_pass.c_str());
        DebugLogger::info("WIFI", "Connecting to '" + currentConfig.wifi_ssid + "'...");

        uint32_t startAttempt = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            DebugLogger::info("WIFI", "Connected! IP: " + WiFi.localIP().toString());
            if (MDNS.begin(currentConfig.ts_hostname.c_str())) {
                MDNS.addService("http", "tcp", 80);
            }
#if ENABLE_TAILSCALE
            startTailscaleClient();
#endif
        } else {
            DebugLogger::warn("WIFI", "Connection timeout. Fallback to AP Mode");
            startApMode();
        }
    } else {
        DebugLogger::info("WIFI", "No SSID configured. Starting AP Mode");
        startApMode();
    }

    setupWebServerRoutes();
    server.begin();
    DebugLogger::info("HTTP", "WebServer started on port 80");

    // Start BLE worker task in background FreeRTOS thread
    xTaskCreatePinnedToCore(bleWorkerTask, "ble_worker", 8192, NULL, 1, NULL, 0);
}

void loop() {
    checkBootButton();

    if (isApMode) {
        dnsServer.processNextRequest();
    }

    server.handleClient();
    delay(2);

#if ENABLE_TAILSCALE
    static uint32_t derpDisconnectedStartMs = 0;
    if (currentConfig.ts_enabled && WiFi.status() == WL_CONNECTED && mlHandle != nullptr) {
        bool derpOk = mlHandle->derp.connected;
        if (derpOk) {
            derpDisconnectedStartMs = 0;
        } else {
            if (derpDisconnectedStartMs == 0) {
                derpDisconnectedStartMs = millis();
            } else if (millis() - derpDisconnectedStartMs >= 360000) {
                DebugLogger::warn("TAILSCALE", "Watchdog alert: DERP disconnected > 360s! Soft restart...");
                derpDisconnectedStartMs = millis();
                restartTailscaleClient();
            }
        }
    } else {
        derpDisconnectedStartMs = 0;
    }
#endif

    static uint32_t lastHb = 0;
    if (millis() - lastHb > 60000) {
        lastHb = millis();
        char hbBuf[128];
        sprintf(hbBuf, "RSSI=%d dBm, Free Heap=%d KB, Free PSRAM=%d KB",
                WiFi.RSSI(), ESP.getFreeHeap() / 1024, ESP.getFreePsram() / 1024);
        DebugLogger::info("SYS", hbBuf);
    }
}
