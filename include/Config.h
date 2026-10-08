#pragma once

#include <Arduino.h>
#include <Preferences.h>

#define BMS_TYPE_AUTO 0
#define BMS_TYPE_JK   1
#define BMS_TYPE_JBD  2

struct AppConfig {
    String  wifi_ssid;
    String  wifi_pass;
    String  bms_mac;
    String  bms_name;
    uint8_t bms_type;    // 0=Auto, 1=JK, 2=JBD
    String  bms_pin;     // Default "123456"
    uint8_t cell_count;  // 4, 8, 16, 24
    bool    ts_enabled;  // Tailscale VPN enabled
    String  ts_hostname; // Default "jbd-bms-probe"
    String  ts_auth_key; // Tailscale Auth Key
};

class ConfigManager {
public:
    static AppConfig load() {
        Preferences prefs;
        prefs.begin("bms_probe", true);

        AppConfig cfg;
        cfg.wifi_ssid   = prefs.getString("ssid", "");
        cfg.wifi_pass   = prefs.getString("pass", "");
        cfg.bms_mac     = prefs.getString("mac", "c8:47:80:1f:5a:1e");
        cfg.bms_name    = prefs.getString("name", "JK-BMS");
        cfg.bms_type    = prefs.getUChar("bms_type", BMS_TYPE_JK);
        cfg.bms_pin     = prefs.getString("pin", "123456");
        cfg.cell_count  = prefs.getUChar("cells", 4);
        cfg.ts_enabled  = prefs.getBool("ts_en", true);
        cfg.ts_hostname = prefs.getString("ts_host", "jbd-bms-probe");
        cfg.ts_auth_key = prefs.getString("ts_key", "tskey-auth-kKU7ahB6hj11CNTRL-EFJu3jE5VBTZjKuksJtxBT2ptmd2AuJ6");

        if (cfg.ts_auth_key.length() == 0) {
            cfg.ts_auth_key = "tskey-auth-kKU7ahB6hj11CNTRL-EFJu3jE5VBTZjKuksJtxBT2ptmd2AuJ6";
        }
        if (cfg.bms_mac.length() == 0) {
            cfg.bms_mac = "c8:47:80:1f:5a:1e";
        }
        if (cfg.cell_count != 4 && cfg.cell_count != 8 && cfg.cell_count != 16 && cfg.cell_count != 24) {
            cfg.cell_count = 4;
        }

        prefs.end();
        return cfg;
    }

    static void save(const AppConfig& cfg) {
        Preferences prefs;
        prefs.begin("bms_probe", false);

        prefs.putString("ssid", cfg.wifi_ssid);
        prefs.putString("pass", cfg.wifi_pass);
        prefs.putString("mac", cfg.bms_mac);
        prefs.putString("name", cfg.bms_name);
        prefs.putUChar("bms_type", cfg.bms_type);
        prefs.putString("pin", cfg.bms_pin);
        prefs.putUChar("cells", cfg.cell_count);
        prefs.putBool("ts_en", cfg.ts_enabled);
        prefs.putString("ts_host", cfg.ts_hostname);
        prefs.putString("ts_key", cfg.ts_auth_key);

        prefs.end();
    }

    static void clear() {
        Preferences prefs;
        prefs.begin("bms_probe", false);
        prefs.clear();
        prefs.end();
    }
};
