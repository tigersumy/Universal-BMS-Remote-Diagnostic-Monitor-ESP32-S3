#include "BmsBleClient.h"
#include <ArduinoJson.h>

static const char* TAG = "BLE";
static BmsBleClient* s_pInstance = nullptr;

class BmsClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) override {
        DebugLogger::info(TAG, "Connected to BLE peripheral successfully");
    }

    void onDisconnect(NimBLEClient* pClient, int reason) override {
        char buf[64];
        sprintf(buf, "Disconnected from peripheral (reason: %d)", reason);
        DebugLogger::warn(TAG, buf);
        if (s_pInstance) {
            s_pInstance->disconnect();
        }
    }

    void onPassKeyEntry(NimBLEConnInfo& connInfo) override {
        uint32_t pin = 123456;
        if (s_pInstance && s_pInstance->getConfig().bms_pin.length() > 0) {
            pin = s_pInstance->getConfig().bms_pin.toInt();
        }
        DebugLogger::info("SEC", "BLE Passkey requested -> injecting PIN: " + String(pin));
        NimBLEDevice::injectPassKey(connInfo, pin);
    }

    uint32_t onPassKeyDisplay(NimBLEConnInfo& connInfo) override {
        uint32_t pin = 123456;
        if (s_pInstance && s_pInstance->getConfig().bms_pin.length() > 0) {
            pin = s_pInstance->getConfig().bms_pin.toInt();
        }
        DebugLogger::info("SEC", "BLE Passkey display -> returning PIN: " + String(pin));
        return pin;
    }

    void onAuthenticationComplete(NimBLEConnInfo& connInfo) override {
        if (connInfo.isEncrypted()) {
            DebugLogger::info("SEC", "BLE Pairing / Authentication SUCCESSFUL (Encrypted link established)");
        } else {
            DebugLogger::warn("SEC", "BLE Pairing finished (Unencrypted link)");
        }
    }

    void onConfirmPasskey(NimBLEConnInfo& connInfo, uint32_t pin) override {
        DebugLogger::info("SEC", "BLE Confirm Passkey PIN: " + String(pin));
        NimBLEDevice::injectConfirmPasskey(connInfo, true);
    }
};

class BmsScanCallbacks : public NimBLEScanCallbacks {
    void processDevice(const NimBLEAdvertisedDevice* advertisedDevice) {
        if (!s_pInstance) return;

        String name = advertisedDevice->getName().c_str();
        String addr = advertisedDevice->getAddress().toString().c_str();
        int rssi = advertisedDevice->getRSSI();
        String type = "Unknown";

        if (name.startsWith("JK") || advertisedDevice->isAdvertisingService(NimBLEUUID("ffe0")) || advertisedDevice->isAdvertisingService(NimBLEUUID("FFE0"))) {
            type = "JK-BMS";
        } else if (name.startsWith("JBD") || name.startsWith("Xiaoxiang") || name.startsWith("SP") || name.startsWith("BS-") || name.startsWith("Smart") ||
                   advertisedDevice->isAdvertisingService(NimBLEUUID("ff00")) || advertisedDevice->isAdvertisingService(NimBLEUUID("FF00")) ||
                   advertisedDevice->isAdvertisingService(NimBLEUUID("fff0")) || advertisedDevice->isAdvertisingService(NimBLEUUID("FFF0")) ||
                   advertisedDevice->isAdvertisingService(NimBLEUUID("fee7")) || advertisedDevice->isAdvertisingService(NimBLEUUID("FEE7")) ||
                   advertisedDevice->isAdvertisingService(NimBLEUUID("0000FF00-0000-1000-8000-00805F9B34FB")) ||
                   advertisedDevice->isAdvertisingService(NimBLEUUID("0000FFF0-0000-1000-8000-00805F9B34FB"))) {
            type = "JBD-BMS";
        }

        for (auto& dev : s_pInstance->m_discoveredDevices) {
            if (dev.address.equalsIgnoreCase(addr)) {
                dev.rssi = rssi;
                if (dev.name.length() == 0 && name.length() > 0) dev.name = name;
                if (type != "Unknown") dev.bms_type = type;
                return;
            }
        }

        BleDiscoveredDevice d;
        d.name = name.length() > 0 ? name : "BMS Device";
        d.address = addr;
        d.rssi = rssi;
        d.bms_type = type;
        d.addr_type = advertisedDevice->getAddress().getType();
        s_pInstance->m_discoveredDevices.push_back(d);

        char logBuf[128];
        sprintf(logBuf, "Found: %s [%s] RSSI:%d Type:%s", d.name.c_str(), addr.c_str(), rssi, type.c_str());
        DebugLogger::info("SCAN", logBuf);
    }

    void onDiscovered(const NimBLEAdvertisedDevice* advertisedDevice) override {
        processDevice(advertisedDevice);
    }

    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
        processDevice(advertisedDevice);
    }

    void onScanEnd(const NimBLEScanResults& results, int reason) override {
        char buf[64];
        sprintf(buf, "Scan finished (found %d devices)", results.getCount());
        DebugLogger::info("SCAN", buf);
        if (s_pInstance) s_pInstance->m_isScanning = false;
    }
};

BmsBleClient::BmsBleClient() {
    s_pInstance = this;
}

bool BmsBleClient::init() {
    NimBLEDevice::init("Universal-BMS-Probe");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    NimBLEDevice::setSecurityAuth(true, true, false);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_ONLY);
    NimBLEDevice::setSecurityPasskey(123456);
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(new BmsScanCallbacks());
    pScan->setActiveScan(true);
    pScan->setInterval(160);
    pScan->setWindow(40);
    DebugLogger::info(TAG, "BLE Client Initialized (P9 power, active scan, SMP passkey security enabled)");
    return true;
}

void BmsBleClient::setTargetConfig(const AppConfig& cfg) {
    m_config = cfg;
    m_telemetry.cell_count = cfg.cell_count;
}

void BmsBleClient::startScan(uint32_t durationSeconds) {
    if (m_isScanning) return;
    m_discoveredDevices.clear();
    m_isScanning = true;
    m_lastScanStartTime = millis();
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setInterval(160);
    pScan->setWindow(40);
    pScan->start(durationSeconds * 1000, false);
    DebugLogger::info("SCAN", "Starting background BLE scan for " + String(durationSeconds) + "s");
}

String BmsBleClient::performScanSync(uint32_t durationSeconds) {
    m_discoveredDevices.clear();
    m_isScanning = true;
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->clearResults();
    pScan->setActiveScan(true);
    pScan->setInterval(100);
    pScan->setWindow(90);
    DebugLogger::info("SCAN", "Starting intensive active BLE scan for " + String(durationSeconds) + "s");
    NimBLEScanResults results = pScan->getResults(durationSeconds * 1000, false);
    m_isScanning = false;
    return getDiscoveredDevicesJson();
}

std::vector<BleDiscoveredDevice> BmsBleClient::getDiscoveredDevices() {
    return m_discoveredDevices;
}

String BmsBleClient::getDiscoveredDevicesJson() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (const auto& dev : m_discoveredDevices) {
        JsonObject obj = arr.add<JsonObject>();
        obj["name"] = dev.name;
        obj["mac"] = dev.address;
        obj["rssi"] = dev.rssi;
        obj["type"] = dev.bms_type;
    }
    String json;
    serializeJson(doc, json);
    return json;
}

void BmsBleClient::loop() {
    uint32_t now = millis();

    if (m_isScanning && (now - m_lastScanStartTime > 7000)) {
        m_isScanning = false;
    }

    if (!m_isConnected && !m_isScanning && (now - m_lastConnectAttempt > 4000)) {
        m_lastConnectAttempt = now;

        if (m_config.bms_mac.length() > 0) {
            unsigned int firstByte = 0;
            sscanf(m_config.bms_mac.c_str(), "%02x", &firstByte);
            static bool s_tryAlt = false;
            uint8_t primaryType = ((firstByte & 0xC0) == 0xC0) ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;
            uint8_t secondaryType = (primaryType == BLE_ADDR_PUBLIC) ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;
            uint8_t useType = s_tryAlt ? secondaryType : primaryType;
            s_tryAlt = !s_tryAlt;

            NimBLEAddress targetAddr(std::string(m_config.bms_mac.c_str()), useType);
            connectToDevice(targetAddr, m_config.bms_name, m_config.bms_type);
        } else {
            for (const auto& dev : m_discoveredDevices) {
                if (dev.bms_type != "Unknown") {
                    NimBLEAddress targetAddr(std::string(dev.address.c_str()), dev.addr_type);
                    if (connectToDevice(targetAddr, dev.name, m_config.bms_type)) {
                        break;
                    }
                }
            }
            if (!m_isConnected && m_discoveredDevices.empty() && (now - m_lastScanStartTime > 20000)) {
                startScan(3);
            }
        }
    }

    // Polling logic
    if (m_isConnected && (now - m_lastPollTime >= 1000)) {
        m_lastPollTime = now;

        if (m_telemetry.bms_type == "JBD-BMS") {
            if (m_pollStep % 2 == 0) {
                sendJbdCommand(0xA5, 0x03); // Basic Info
            } else {
                sendJbdCommand(0xA5, 0x04); // Cell Voltages
            }
            m_pollStep++;
        } else if (m_telemetry.bms_type == "JK-BMS") {
            if (m_telemetry.last_update == 0 || (now - m_telemetry.last_update > 4000)) {
                sendJkPollRequest();
            }
        }
    }
}

bool BmsBleClient::connectToDevice(const NimBLEAddress& address, const String& name, uint8_t forcedType) {
    char buf[128];
    sprintf(buf, "Connecting to %s (type: %d, name: '%s', forced: %d)...", 
            address.toString().c_str(), address.getType(), name.c_str(), forcedType);
    DebugLogger::info(TAG, buf);

    if (m_pClient == nullptr) {
        m_pClient = NimBLEDevice::createClient();
        m_pClient->setClientCallbacks(new BmsClientCallbacks(), false);
        m_pClient->setConnectionParams(24, 48, 0, 400); // 30ms - 60ms interval, 4s timeout (Optimal for Wi-Fi & Tailscale coexistence)
        m_pClient->setConnectTimeout(2500);
    }

    bool ok = m_pClient->connect(address, false);
    if (!ok) {
        DebugLogger::warn(TAG, "Connection attempt failed for address type " + String(address.getType()));
        return false;
    }

    DebugLogger::info(TAG, "Connected! Discovering all services & characteristics...");
    m_rxBuffer.clear();

    std::vector<NimBLERemoteService*> services = m_pClient->getServices(true);
    std::vector<NimBLERemoteCharacteristic*> notifyChars;
    std::vector<NimBLERemoteCharacteristic*> writeChars;
    NimBLERemoteService* pJkService = nullptr;

    for (auto* s : services) {
        String sUuid = s->getUUID().toString().c_str();
        sUuid.toLowerCase();
        DebugLogger::info("DISC", "Found Service: " + sUuid);

        if (sUuid.indexOf("ffe0") >= 0) {
            pJkService = s;
        }

        for (auto* c : s->getCharacteristics(true)) {
            String cUuid = c->getUUID().toString().c_str();
            cUuid.toLowerCase();
            char cBuf[160];
            sprintf(cBuf, "Svc [%s] Char [%s] H:%d [Notify:%d, Indic:%d, Wr:%d, WrNoResp:%d]",
                    sUuid.c_str(), cUuid.c_str(), c->getHandle(), c->canNotify(), c->canIndicate(), c->canWrite(), c->canWriteNoResponse());
            DebugLogger::info("DISC", cBuf);

            if (c->canNotify() || c->canIndicate() || cUuid.indexOf("ff01") >= 0 || cUuid.indexOf("fff1") >= 0 || cUuid.indexOf("0002") >= 0) {
                notifyChars.push_back(c);
            }
            if (c->canWrite() || c->canWriteNoResponse() || cUuid.indexOf("ff02") >= 0 || cUuid.indexOf("fff2") >= 0 || cUuid.indexOf("0003") >= 0) {
                writeChars.push_back(c);
            }
        }
    }

    // 1. Process JBD (if not JK service or if forcedType is JBD)
    if (!notifyChars.empty() && !writeChars.empty() && (forcedType == BMS_TYPE_AUTO || forcedType == BMS_TYPE_JBD) && (!pJkService || forcedType == BMS_TYPE_JBD)) {
        if (m_config.bms_pin.length() > 0) {
            DebugLogger::info(TAG, "Securing JBD connection with Passkey: " + m_config.bms_pin);
            m_pClient->secureConnection();
        }
        m_pJbdNotifyChar = notifyChars[0];
        m_pJbdWriteChar  = writeChars[0];

        // Prefer FF01/FF02 if available
        for (auto* c : notifyChars) {
            String u = c->getUUID().toString().c_str();
            u.toLowerCase();
            if (u.indexOf("ff01") >= 0 || u.indexOf("fff1") >= 0 || u.indexOf("0002") >= 0) {
                m_pJbdNotifyChar = c;
                break;
            }
        }
        for (auto* c : writeChars) {
            String u = c->getUUID().toString().c_str();
            u.toLowerCase();
            if (u.indexOf("ff02") >= 0 || u.indexOf("fff2") >= 0 || u.indexOf("0003") >= 0) {
                m_pJbdWriteChar = c;
                break;
            }
        }

        m_jbdWriteWithResponse = m_pJbdWriteChar->canWrite() && !m_pJbdWriteChar->canWriteNoResponse();
        DebugLogger::info(TAG, "Selected JBD Primary: Notify H:" + String(m_pJbdNotifyChar->getHandle()) + 
                              ", Write H:" + String(m_pJbdWriteChar->getHandle()) + 
                              " (WithResponse: " + String(m_jbdWriteWithResponse) + ")");

        // Subscribe to all notification candidates
        for (auto* nc : notifyChars) {
            bool subRes = nc->subscribe(true, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
                this->handleJbdPacket(pData, length);
            });
            DebugLogger::info(TAG, "Subscribed to H:" + String(nc->getHandle()) + " (" + String(nc->getUUID().toString().c_str()) + ") -> Status: " + String(subRes ? "SUCCESS" : "FAILED"));
        }

        m_isConnected = true;
        m_telemetry.connected = true;
        m_telemetry.bms_type = "JBD-BMS";
        m_telemetry.device_name = name.length() > 0 ? name : "JBD-BMS";
        m_telemetry.mac_address = address.toString().c_str();
        m_telemetry.last_update = millis();

        // Standard JBD Interrogation:
        // First try standard query 0x03 & 0x04 without noisy AT frames
        sendJbdCommand(0xA5, 0x03);
        delay(80);
        sendJbdCommand(0xA5, 0x04);
        delay(80);

        // Also send standard PIN unlock register (0x01) if PIN is configured
        String pinToTry = m_config.bms_pin.length() > 0 ? m_config.bms_pin : "123456";
        if (pinToTry.length() == 6) {
            uint8_t pinFrame[13];
            pinFrame[0] = 0xDD;
            pinFrame[1] = 0x5A;
            pinFrame[2] = 0x01; // Register 0x01 = Password
            pinFrame[3] = 0x06;
            for (int i = 0; i < 6; ++i) pinFrame[4 + i] = pinToTry[i];
            uint32_t sum = 0x01 + 0x06;
            for (int i = 0; i < 6; ++i) sum += pinToTry[i];
            uint16_t crc = (uint16_t)(0x10000 - sum);
            pinFrame[10] = (uint8_t)(crc >> 8);
            pinFrame[11] = (uint8_t)(crc & 0xFF);
            pinFrame[12] = 0x77;
            DebugLogger::logTx(TAG, pinFrame, 13, "JBD Reg 0x01 PIN Unlock ('" + pinToTry + "')");
            m_pJbdWriteChar->writeValue(pinFrame, 13, m_jbdWriteWithResponse);
        }

        return true;
    }

    // 2. Process JK Service (0xFFE0)
    if (pJkService && (forcedType == BMS_TYPE_AUTO || forcedType == BMS_TYPE_JK)) {
        DebugLogger::info(TAG, "Selected JK Service (0xFFE0)");
        m_pJkNotifyChar = nullptr;
        m_pJkWriteChar = nullptr;

        for (auto* c : pJkService->getCharacteristics(true)) {
            String uuid = c->getUUID().toString().c_str();
            uuid.toLowerCase();
            if (uuid.indexOf("ffe2") >= 0 || (c->canWriteNoResponse() && !c->canNotify())) {
                m_pJkWriteChar = c;
            }
            if (uuid.indexOf("ffe1") >= 0 || c->canNotify()) {
                m_pJkNotifyChar = c;
                c->subscribe(true, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
                    this->handleJkPacket(pData, length);
                });
                DebugLogger::info(TAG, "Subscribed to JK Notify H:" + String(c->getHandle()));
            }
        }

        if (!m_pJkWriteChar && m_pJkNotifyChar) {
            m_pJkWriteChar = m_pJkNotifyChar;
        }

        if (m_pJkNotifyChar) {
            m_isConnected = true;
            m_telemetry.connected = true;
            m_telemetry.bms_type = "JK-BMS";
            m_telemetry.device_name = name.length() > 0 ? name : "JK-BMS";
            m_telemetry.mac_address = address.toString().c_str();
            m_telemetry.last_update = millis();

            auto frameDev = buildJkFrame(0x97, 0, 0);
            DebugLogger::logTx(TAG, frameDev.data(), frameDev.size(), "JK DeviceInfo (0x97)");
            m_pJkNotifyChar->writeValue(frameDev.data(), frameDev.size(), false);
            delay(250);

            sendJkPollRequest();
            return true;
        }
    }

    DebugLogger::error(TAG, "No usable JBD (FF00/FFF0/FEE7) or JK (FFE0) characteristics discovered");
    disconnect();
    return false;
}

void BmsBleClient::disconnect() {
    if (m_pClient && m_pClient->isConnected()) {
        m_pClient->disconnect();
    }
    m_isConnected = false;
    m_telemetry.connected = false;
    m_pJbdNotifyChar = nullptr;
    m_pJbdWriteChar  = nullptr;
    m_pJkNotifyChar  = nullptr;
    m_pJkWriteChar   = nullptr;
    m_rxBuffer.clear();
}

void BmsBleClient::reconnect() {
    DebugLogger::info(TAG, "Manual reconnection triggered by user");
    disconnect();
    m_lastConnectAttempt = 0;
}

bool BmsBleClient::sendRawBle(const uint8_t* data, size_t len) {
    if (!m_isConnected) {
        DebugLogger::error(TAG, "sendRawBle failed: not connected");
        return false;
    }
    if (m_telemetry.bms_type == "JBD-BMS" && m_pJbdWriteChar) {
        DebugLogger::logTx(TAG, data, len, "Raw JBD Frame");
        return m_pJbdWriteChar->writeValue(data, len, m_jbdWriteWithResponse);
    }
    if (m_telemetry.bms_type == "JK-BMS" && m_pJkWriteChar) {
        DebugLogger::logTx(TAG, data, len, "Raw JK Frame");
        return m_pJkWriteChar->writeValue(data, len, false);
    }
    return false;
}

// ======================== JBD Implementation ========================

void BmsBleClient::sendJbdCommand(uint8_t cmd, uint8_t reg, const uint8_t* payload, uint8_t len) {
    if (!m_pJbdWriteChar || !m_isConnected) return;

    size_t totalLen = 7 + len;
    uint8_t frame[64];
    frame[0] = 0xDD;
    frame[1] = cmd;
    frame[2] = reg;
    frame[3] = len;

    if (payload && len > 0) {
        memcpy(&frame[4], payload, len);
    }

    uint32_t sum = reg + len;
    for (size_t i = 0; i < len; ++i) sum += payload[i];
    uint16_t crc = (uint16_t)(0x10000 - sum);

    frame[4 + len] = (uint8_t)(crc >> 8);
    frame[5 + len] = (uint8_t)(crc & 0xFF);
    frame[6 + len] = 0x77;

    char descBuf[32];
    sprintf(descBuf, "JBD Cmd:0x%02X Reg:0x%02X", cmd, reg);
    DebugLogger::logTx(TAG, frame, totalLen, descBuf);

    m_pJbdWriteChar->writeValue(frame, totalLen, m_jbdWriteWithResponse);
}

void BmsBleClient::handleJbdPacket(const uint8_t* data, size_t len) {
    if (len == 0) return;

    DebugLogger::logRx(TAG, data, len, "JBD Rx Chunk");

    m_rxBuffer.insert(m_rxBuffer.end(), data, data + len);

    if (m_rxBuffer.size() > 512) {
        DebugLogger::warn(TAG, "Rx buffer overflow >512, resetting");
        m_rxBuffer.clear();
        return;
    }

    while (!m_rxBuffer.empty()) {
        // Module Response (FF AA <cmd> <len> <data...> <chk>)
        if (m_rxBuffer.size() >= 2 && m_rxBuffer[0] == 0xFF && m_rxBuffer[1] == 0xAA) {
            if (m_rxBuffer.size() < 4) return;
            uint8_t modCmd = m_rxBuffer[2];
            uint8_t modLen = m_rxBuffer[3];
            size_t totalModLen = 4 + modLen + 1;
            if (m_rxBuffer.size() < totalModLen) return;

            char buf[64];
            sprintf(buf, "JBD Module Resp 0x%02X (len %u)", modCmd, modLen);
            DebugLogger::info(TAG, buf);
            m_rxBuffer.erase(m_rxBuffer.begin(), m_rxBuffer.begin() + totalModLen);
            continue;
        }

        // Align to 0xDD
        if (m_rxBuffer[0] != 0xDD) {
            m_rxBuffer.erase(m_rxBuffer.begin());
            continue;
        }

        if (m_rxBuffer.size() < 7) return;

        uint8_t reg = m_rxBuffer[1];
        uint8_t status = m_rxBuffer[2];
        uint8_t dataLen = m_rxBuffer[3];
        size_t expectedLen = 4 + dataLen + 2 + 1;

        if (m_rxBuffer.size() < expectedLen) return;

        if (m_rxBuffer[expectedLen - 1] != 0x77) {
            DebugLogger::warn(TAG, "JBD Frame missing 0x77 terminator, skipping byte");
            m_rxBuffer.erase(m_rxBuffer.begin());
            continue;
        }

        std::vector<uint8_t> frame(m_rxBuffer.begin(), m_rxBuffer.begin() + expectedLen);
        m_rxBuffer.erase(m_rxBuffer.begin(), m_rxBuffer.begin() + expectedLen);

        if (status != 0x00) {
            char buf[64];
            sprintf(buf, "JBD Reg 0x%02X Error Status: 0x%02X", reg, status);
            DebugLogger::warn(TAG, buf);
            continue;
        }

        if (reg == 0x03) {
            decodeJbdTelemetry(frame);
        } else if (reg == 0x04) {
            decodeJbdCells(frame);
        } else if (reg == 0x05) {
            String devName = "";
            for (size_t i = 0; i < dataLen; ++i) {
                char c = (char)frame[4 + i];
                if (c >= 32 && c <= 126) devName += c;
            }
            if (devName.length() > 0) {
                m_telemetry.device_name = devName;
                DebugLogger::info(TAG, "JBD Device Name: " + devName);
            }
        }
    }
}

void BmsBleClient::decodeJbdTelemetry(const std::vector<uint8_t>& data) {
    if (data.size() < 4 + 23) return;

    auto get16 = [&](size_t idx) -> uint16_t {
        return (uint16_t(data[idx]) << 8) | uint16_t(data[idx + 1]);
    };

    m_telemetry.total_voltage = (float)get16(4) * 0.01f;
    int16_t rawCurrent = (int16_t)get16(6);
    m_telemetry.current = (float)rawCurrent * 0.01f;
    m_telemetry.power = m_telemetry.total_voltage * m_telemetry.current;
    if (m_telemetry.power >= 0) {
        m_telemetry.charge_power = m_telemetry.power;
        m_telemetry.discharge_power = 0.0f;
    } else {
        m_telemetry.charge_power = 0.0f;
        m_telemetry.discharge_power = -m_telemetry.power;
    }

    m_telemetry.capacity_remain = (float)get16(8) * 0.01f;
    m_telemetry.capacity_total  = (float)get16(10) * 0.01f;
    m_telemetry.cycle_count     = get16(12);

    uint8_t reportedCellCount = data[25];
    if (reportedCellCount >= 3 && reportedCellCount <= 32) {
        m_telemetry.cell_count = reportedCellCount;
    }

    uint8_t ntcCount = data[26];
    if (ntcCount > 0 && data.size() >= 4 + 23 + (ntcCount * 2)) {
        int16_t rawT1 = (int16_t)get16(27);
        m_telemetry.temp_sensor1 = ((float)rawT1 - 2731.0f) * 0.1f;
        if (ntcCount > 1) {
            int16_t rawT2 = (int16_t)get16(29);
            m_telemetry.temp_sensor2 = ((float)rawT2 - 2731.0f) * 0.1f;
            m_telemetry.temp_mos = m_telemetry.temp_sensor2;
        } else {
            m_telemetry.temp_mos = m_telemetry.temp_sensor1;
        }
    }

    if (data.size() >= 4 + 20) {
        m_telemetry.soc = (float)data[23];
        uint8_t mosStatus = data[24];
        m_telemetry.switch_charging    = (mosStatus & 0x01) != 0;
        m_telemetry.switch_discharging = (mosStatus & 0x02) != 0;
    }

    if (data.size() >= 4 + 18) {
        m_telemetry.raw_errors = get16(20);
        m_telemetry.errors_str = (m_telemetry.raw_errors == 0) ? "OK (Без помилок)" : ("0x" + String(m_telemetry.raw_errors, HEX));
    }

    m_telemetry.last_update = millis();
    m_telemetry.connected = true;

    char buf[128];
    sprintf(buf, "JBD Telemetry: V=%.2fV, I=%.2fA, SOC=%.0f%%, Cells=%d, Errors: %s",
            m_telemetry.total_voltage, m_telemetry.current, m_telemetry.soc, m_telemetry.cell_count, m_telemetry.errors_str.c_str());
    DebugLogger::info(TAG, buf);
}

void BmsBleClient::decodeJbdCells(const std::vector<uint8_t>& data) {
    if (data.size() < 6) return;
    uint8_t dataLen = data[3];
    uint8_t numCells = dataLen / 2;
    if (numCells > 32) numCells = 32;

    float minV = 999.0f;
    float maxV = 0.0f;
    uint8_t minIdx = 1;
    uint8_t maxIdx = 1;

    for (uint8_t i = 0; i < numCells; ++i) {
        uint16_t rawMv = (uint16_t(data[4 + i * 2]) << 8) | uint16_t(data[4 + i * 2 + 1]);
        float v = (float)rawMv * 0.001f;
        m_telemetry.cell_voltages[i] = v;

        if (v > 0.5f) {
            if (v < minV) {
                minV = v;
                minIdx = i + 1;
            }
            if (v > maxV) {
                maxV = v;
                maxIdx = i + 1;
            }
        }
    }

    m_telemetry.min_cell_v = (minV < 900.0f) ? minV : 0.0f;
    m_telemetry.max_cell_v = maxV;
    m_telemetry.delta_cell_v = (maxV > minV) ? (maxV - minV) : 0.0f;
    m_telemetry.min_cell_idx = minIdx;
    m_telemetry.max_cell_idx = maxIdx;
    m_telemetry.last_update = millis();

    char buf[128];
    sprintf(buf, "JBD Cells: Delta=%.3fV (Min C%d=%.3fV, Max C%d=%.3fV)",
            m_telemetry.delta_cell_v, minIdx, m_telemetry.min_cell_v, maxIdx, m_telemetry.max_cell_v);
    DebugLogger::info(TAG, buf);
}

// ======================== JK Implementation ========================

std::vector<uint8_t> BmsBleClient::buildJkFrame(uint8_t address, uint32_t value, uint8_t length) {
    std::vector<uint8_t> frame(20, 0x00);
    frame[0] = 0xAA;
    frame[1] = 0x55;
    frame[2] = 0x90;
    frame[3] = 0xEB;
    frame[4] = address;
    frame[5] = length;
    frame[6] = (value >> 0) & 0xFF;
    frame[7] = (value >> 8) & 0xFF;
    frame[8] = (value >> 16) & 0xFF;
    frame[9] = (value >> 24) & 0xFF;

    uint8_t crc = 0;
    for (size_t i = 0; i < 19; ++i) crc += frame[i];
    frame[19] = crc;
    return frame;
}

void BmsBleClient::sendJkPollRequest() {
    if (!m_pJkNotifyChar || !m_isConnected) return;
    auto frame = buildJkFrame(0x96, 0, 0);
    DebugLogger::logTx(TAG, frame.data(), frame.size(), "JK CellInfo (0x96)");
    m_pJkNotifyChar->writeValue(frame.data(), frame.size(), false);
}

void BmsBleClient::handleJkPacket(const uint8_t* data, size_t len) {
    if (len == 0) return;

    if (m_rxBuffer.size() > 500) {
        m_rxBuffer.clear();
    }
    // If packet starts with JK response header 0x55 0xAA 0xEB 0x90, reset buffer
    if (len >= 4 && data[0] == 0x55 && data[1] == 0xAA && data[2] == 0xEB && data[3] == 0x90) {
        m_rxBuffer.clear();
    }

    m_rxBuffer.insert(m_rxBuffer.end(), data, data + len);

    if (m_rxBuffer.size() >= 5 && m_rxBuffer[0] == 0x55 && m_rxBuffer[1] == 0xAA && m_rxBuffer[2] == 0xEB && m_rxBuffer[3] == 0x90) {
        uint8_t frameType = m_rxBuffer[4];
        if (frameType == 0x01 && m_rxBuffer.size() >= 130) {
            decodeJkSettings(m_rxBuffer);
            m_rxBuffer.clear();
        } else if (frameType == 0x02 && m_rxBuffer.size() >= 300) {
            decodeJkCellInfo(m_rxBuffer);
            m_rxBuffer.clear();
        }
    }
}

void BmsBleClient::decodeJkSettings(const std::vector<uint8_t>& data) {
    if (data.size() < 130) return;
    // Charge switch at 118, Discharge switch at 122, Balancer switch at 126
    m_telemetry.switch_charging    = (data[118] != 0);
    m_telemetry.switch_discharging = (data[122] != 0);
    m_telemetry.switch_balancer    = (data[126] != 0);
    m_telemetry.last_update = millis();
    char buf[128];
    sprintf(buf, "JK Settings: Charge=%d, Discharge=%d, Balancer=%d",
            m_telemetry.switch_charging, m_telemetry.switch_discharging, m_telemetry.switch_balancer);
    DebugLogger::info(TAG, buf);
}

void BmsBleClient::decodeJkCellInfo(const std::vector<uint8_t>& data) {
    if (data.size() < 300) return;

    auto get16 = [&](size_t i) -> uint16_t {
        if (i + 1 >= data.size()) return 0;
        return (uint16_t(data[i + 1]) << 8) | uint16_t(data[i]);
    };
    auto get32 = [&](size_t i) -> uint32_t {
        return (uint32_t(get16(i + 2)) << 16) | uint32_t(get16(i));
    };

    // Detect 32S offset vs 24S offset
    size_t offset = 0;
    float v32 = (float)get32(150) * 0.001f;
    float v24 = (float)get32(118) * 0.001f;
    if (v32 >= 8.0f && v32 <= 80.0f) {
        offset = 32; // JK02_32S protocol
    } else if (v24 >= 8.0f && v24 <= 80.0f) {
        offset = 0;  // JK02_24S protocol
    } else {
        offset = 32; // Default to 32S
    }

    // Cell Voltages (bytes 6 + i * 2)
    float minV = 999.0f;
    float maxV = 0.0f;
    uint8_t minIdx = 1;
    uint8_t maxIdx = 1;
    uint8_t detectedCells = 0;

    for (int i = 0; i < 32; i++) {
        float v = (float)get16(6 + i * 2) * 0.001f;
        if (v >= 0.5f && v <= 5.0f) {
            m_telemetry.cell_voltages[i] = v;
            detectedCells = i + 1;
            if (v < minV) {
                minV = v;
                minIdx = i + 1;
            }
            if (v > maxV) {
                maxV = v;
                maxIdx = i + 1;
            }
        } else {
            m_telemetry.cell_voltages[i] = 0.0f;
        }
    }

    if (detectedCells > 0) {
        m_telemetry.cell_count = detectedCells;
    } else if (m_config.cell_count > 0) {
        m_telemetry.cell_count = m_config.cell_count;
    }

    m_telemetry.min_cell_v = (minV < 900.0f) ? minV : 0.0f;
    m_telemetry.max_cell_v = maxV;
    m_telemetry.delta_cell_v = (maxV > minV && minV < 900.0f) ? (maxV - minV) : 0.0f;
    m_telemetry.min_cell_idx = minIdx;
    m_telemetry.max_cell_idx = maxIdx;

    m_telemetry.total_voltage = (float)get32(118 + offset) * 0.001f;

    // Current (signed 32-bit: positive = charge, negative = discharge)
    int32_t rawCurrent = (int32_t)get32(126 + offset);
    m_telemetry.current = (float)rawCurrent * 0.001f;
    m_telemetry.power = m_telemetry.total_voltage * m_telemetry.current;
    if (m_telemetry.power >= 0) {
        m_telemetry.charge_power = m_telemetry.power;
        m_telemetry.discharge_power = 0.0f;
    } else {
        m_telemetry.charge_power = 0.0f;
        m_telemetry.discharge_power = -m_telemetry.power;
    }

    // Temperatures
    m_telemetry.temp_sensor1 = (float)((int16_t)get16(130 + offset)) * 0.1f;
    m_telemetry.temp_sensor2 = (float)((int16_t)get16(132 + offset)) * 0.1f;
    if (offset == 32) {
        m_telemetry.temp_mos = (float)((int16_t)get16(112 + offset)) * 0.1f;
    } else {
        m_telemetry.temp_mos = (float)((int16_t)get16(134 + offset)) * 0.1f;
    }

    // Balancer
    m_telemetry.balancing_current = (float)((int16_t)get16(138 + offset)) * 0.001f;
    uint8_t balState = (140 + offset < data.size()) ? data[140 + offset] : 0;
    m_telemetry.balancing_active = (balState != 0);

    // SOC & Capacity
    if (141 + offset < data.size()) {
        m_telemetry.soc = (float)data[141 + offset];
    }
    m_telemetry.capacity_remain = (float)get32(142 + offset) * 0.001f;
    m_telemetry.capacity_total  = (float)get32(146 + offset) * 0.001f;

    // Cycle Count
    m_telemetry.cycle_count = get32(150 + offset);

    // Errors bitmask
    uint32_t errs = get32(134 + offset);
    m_telemetry.raw_errors = errs;
    m_telemetry.errors_str = (errs == 0) ? "OK (Без помилок)" : ("0x" + String(errs, HEX));

    // Real-time switch states from live cell info frame
    if (167 + offset < data.size()) {
        m_telemetry.switch_charging    = (data[166 + offset] != 0);
        m_telemetry.switch_discharging = (data[167 + offset] != 0);
    }

    m_telemetry.connected = true;
    m_telemetry.last_update = millis();

    char buf[160];
    sprintf(buf, "JK Telemetry (%dS): V=%.2fV, I=%.2fA, SOC=%.0f%%, Delta=%.3fV (Min=C%d:%.3fV, Max=C%d:%.3fV)",
            m_telemetry.cell_count,
            m_telemetry.total_voltage, m_telemetry.current, m_telemetry.soc,
            m_telemetry.delta_cell_v,
            m_telemetry.min_cell_idx, m_telemetry.min_cell_v,
            m_telemetry.max_cell_idx, m_telemetry.max_cell_v);
    DebugLogger::info(TAG, buf);
}

bool BmsBleClient::setSwitch(const String& sw, bool state) {
    if (!m_isConnected) {
        DebugLogger::error(TAG, "setSwitch failed: not connected");
        return false;
    }

    if (m_telemetry.bms_type == "JBD-BMS") {
        uint8_t mosPayload[2];
        uint8_t currentChg = m_telemetry.switch_charging ? 0 : 1;
        uint8_t currentDsg = m_telemetry.switch_discharging ? 0 : 2;

        if (sw == "charging") currentChg = state ? 0 : 1;
        if (sw == "discharging") currentDsg = state ? 0 : 2;

        mosPayload[0] = 0x00;
        mosPayload[1] = currentChg | currentDsg;

        sendJbdCommand(0x5A, 0xE1, mosPayload, 2);
        if (sw == "charging") m_telemetry.switch_charging = state;
        if (sw == "discharging") m_telemetry.switch_discharging = state;
        return true;
    } else if (m_telemetry.bms_type == "JK-BMS") {
        uint8_t reg = 0;
        if (sw == "charging") reg = 0x1D;
        else if (sw == "discharging") reg = 0x1E;
        else if (sw == "balancer") reg = 0x1F;
        if (reg == 0) return false;

        auto frame = buildJkFrame(reg, state ? 1 : 0, 4);
        DebugLogger::logTx(TAG, frame.data(), frame.size(), "JK Switch " + sw);
        if (m_pJkNotifyChar) {
            bool ok = m_pJkNotifyChar->writeValue(frame.data(), frame.size(), false);
            if (ok) {
                if (sw == "charging") m_telemetry.switch_charging = state;
                else if (sw == "discharging") m_telemetry.switch_discharging = state;
                else if (sw == "balancer") m_telemetry.switch_balancer = state;
            }
            return ok;
        }
    }
    return false;
}
