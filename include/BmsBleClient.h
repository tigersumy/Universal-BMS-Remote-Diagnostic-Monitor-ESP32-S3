#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <vector>
#include "Config.h"
#include "BmsProtocol.h"
#include "DebugLogger.h"

struct BleDiscoveredDevice {
    String  name;
    String  address;
    int     rssi;
    String  bms_type;
    uint8_t addr_type;
};

class BmsBleClient {
public:
    BmsBleClient();
    bool init();
    void setTargetConfig(const AppConfig& cfg);
    void startScan(uint32_t durationSeconds = 5);
    String performScanSync(uint32_t durationSeconds = 4);
    std::vector<BleDiscoveredDevice> getDiscoveredDevices();
    String getDiscoveredDevicesJson();

    void loop();
    bool connectToDevice(const NimBLEAddress& address, const String& name, uint8_t forcedType);
    void disconnect();
    void reconnect();

    bool isConnected() const { return m_isConnected; }
    const BmsTelemetry& getTelemetry() const { return m_telemetry; }
    const AppConfig& getConfig() const { return m_config; }

    bool setSwitch(const String& sw, bool state);
    bool sendRawBle(const uint8_t* data, size_t len);

    bool readJbdCapacityParams();
    bool writeJbdCapacityParams(float nominalAh, float cycleAh, uint8_t cellCount, uint16_t fullMv);
    bool writeDalyCapacityParams(float nominalAh, uint8_t cellCount);

private:
    void sendJbdCommand(uint8_t cmd, uint8_t reg, const uint8_t* payload = nullptr, uint8_t len = 0);
    void handleJbdPacket(const uint8_t* data, size_t len);
    void decodeJbdTelemetry(const std::vector<uint8_t>& data);
    void decodeJbdCells(const std::vector<uint8_t>& data);
    void decodeJbdEeprom(uint8_t reg, const std::vector<uint8_t>& data);

    void sendJkPollRequest();
    void handleJkPacket(const uint8_t* data, size_t len);
    void decodeJkCellInfo(const std::vector<uint8_t>& data);
    void decodeJkSettings(const std::vector<uint8_t>& data);
    std::vector<uint8_t> buildJkFrame(uint8_t address, uint32_t value, uint8_t length);

    void sendDalyCommand(uint8_t cmdId, const uint8_t* payload = nullptr);
    void handleDalyPacket(const uint8_t* data, size_t len);
    void decodeDalyTelemetry(uint8_t cmdId, const uint8_t* payload);
    uint8_t calcDalyChecksum(const uint8_t* buf, size_t len);

    AppConfig        m_config;
    NimBLEClient*    m_pClient = nullptr;
    bool             m_isConnected = false;
    bool             m_isScanning = false;
    uint32_t         m_lastScanStartTime = 0;
    uint32_t         m_lastConnectAttempt = 0;
    uint32_t         m_lastPollTime = 0;
    uint8_t          m_pollStep = 0;

    // JBD Characteristics
    NimBLERemoteCharacteristic* m_pJbdNotifyChar = nullptr;
    NimBLERemoteCharacteristic* m_pJbdWriteChar  = nullptr;
    bool                        m_jbdWriteWithResponse = false;

    // JK Characteristics
    NimBLERemoteCharacteristic* m_pJkNotifyChar  = nullptr;
    NimBLERemoteCharacteristic* m_pJkWriteChar   = nullptr;

    // Daly Characteristics
    NimBLERemoteCharacteristic* m_pDalyNotifyChar = nullptr;
    NimBLERemoteCharacteristic* m_pDalyWriteChar  = nullptr;
    bool                        m_dalyWriteWithResponse = false;

    BmsTelemetry     m_telemetry;
    std::vector<uint8_t> m_rxBuffer;
    std::vector<BleDiscoveredDevice> m_discoveredDevices;

    friend class BmsClientCallbacks;
    friend class BmsScanCallbacks;
};
