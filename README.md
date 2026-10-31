# Universal BMS Remote Diagnostic Monitor & Tailscale Probe (ESP32-S3)

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Build%20Passing-brightgreen.svg)](https://platformio.org/)
[![Hardware](https://img.shields.io/badge/Hardware-ESP32--S3%20SuperMini%20(2MB%20PSRAM)-blue.svg)](https://www.espressif.com/)
[![Tailscale](https://img.shields.io/badge/Tailscale-VPN%20Embedded-darkblue.svg)](https://tailscale.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![BMS Protocols](https://img.shields.io/badge/BMS%20Protocols-JK%20%7C%20JBD%20%7C%20Daly%20%7C%20Ant%20%7C%20Redodo-orange.svg)](#-key-features)

*Read this in other languages: [English](#-english-overview) | [Українська](#-українська-версія)*

---

# 🇬🇧 English Overview

**Universal BMS Remote Diagnostic Monitor & Tailscale Probe** is a standalone 5-in-1 hardware-software diagnostic bridge built on the **ESP32-S3 (2MB PSRAM)** with embedded **Tailscale VPN**. It enables remote monitoring, protocol reverse engineering, live diagnostics, and MOS switch control for battery management systems based on **JK-BMS**, **JBD-BMS (Xiaoxiang / Overkill)**, **Daly-BMS**, **Ant-BMS**, and **Redodo / LiTime Smart BMS** without requiring physical on-site presence.

---

## 🌟 Key Features

- **5-in-1 Universal Multi-Protocol BLE Engine:**
  - **JK-BMS (JiKong / Hankzor):** Complete 24S and 32S protocol support (`0x97` / `0x96` handshake) with dynamic frame offset alignment and charge/discharge switch controls.
  - **JBD-BMS (Xiaoxiang / Overkill):** Supports 4S / 8S / 16S / 24S battery configurations, automatic discovery of `0xFF00`, `0xFFF0`, `0xFEE7`, and Nordic UART services, multi-tier PIN authentication (`123456`, `1234`, `000000`, direct poll fallback), auto-detection of write mode (`Write with/without Response`), charge/discharge/balancer switches, and EEPROM capacity configuration.
  - **Daly-BMS (Daly Smart BMS):** Full telemetry support (`0x90`–`0x98`) covering total pack voltage, current (0.1A with 30000 offset), SOC, individual cell voltages, NTC temperatures, MOS states, cell balancing bitmasks, and remote capacity tuning.
  - **Ant-BMS (Classic & 2021+ series):** Autonomous 140-byte continuous telemetry stream (active polling `0xDB` / `0x5A 0x5A 0x00 0x00 0x00 0x00`), high-precision cell voltages up to 32S, MOS temperatures, and remote switch actuation (`0xA5` charge, discharge, balancing).
  - **Redodo / LiTime Smart BMS:** Support for monolithic 105-byte telemetry frames (`0x13` query command), decoding individual cell voltages, NTC probe temperatures, charging/discharging protection flags, and balancer status.
- **Embedded Tailscale VPN Client (MicroLink + WireGuard lwIP):**
  - Secure remote access from anywhere across the globe without port forwarding, NAT traversal issues, or public IP addresses.
  - Dynamically allocated in **2 MB PSRAM** buffers supporting up to 32 active peers in your tailnet.
  - 360-second hardware watchdog and 120-second periodic DERP reconnect cycle.
- **Interactive Live BLE Diagnostics (Live Packet Trace):**
  - Web-embedded terminal displaying color-coded transmitted (`[TX >>]`) and received (`[RX <<]`) raw HEX frames.
  - 1-Click quick test buttons (`Basic Info 0x03`, `Cells 0x04`, `Device Name 0x05`, `PIN Auth Tests 123456 / 1234 / 000000`).
  - Raw HEX command console allowing manual injection of arbitrary frames over BLE (`POST /api/send-raw-ble`).
  - 200-entry in-memory ring buffer with clipboard export.
- **Credential Privacy & Security:**
  - Tailscale Auth Keys and Wi-Fi credentials are stored in non-volatile storage (NVS) and never exposed in plain text on the web interface.
- **Web OTA Firmware Flashing:**
  - Upload pre-compiled firmware binaries remotely via `/update` over Tailscale.
- **Thermal Optimization:**
  - Underclocked CPU frequency (160 MHz) ensures continuous, cool 24/7 standalone operation.

---

## 📊 BMS Compatibility Matrix

| BMS Family | Supported Models / Series | Supported Cells | Telemetry (V, A, SOC, Cells, NTC) | MOSFET Controls | Status |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **JK-BMS** | JK-B1A8S20P, JK-B2A8S20P, JK-BD6A20S10P, JK-B2A24S20P | 4S - 24S / 32S | Total V, Current, SOC, Cells, NTC, Errors | Charge, Discharge | ✅ Verified |
| **JBD-BMS** | SP04S034, SP04S020, SP16S001, Overkill Solar | 4S - 24S | Total V, Current, SOC, Cells, NTC, Alarms | Charge, Discharge, Balancer, EEPROM | ✅ Verified |
| **Daly Smart** | Daly 4S–24S LiFePO4 / Li-ion Smart BMS | 4S - 24S | Total V, Current, SOC, Cells, NTC, Alarms | Charge, Discharge | ✅ Verified |
| **Ant-BMS** | Ant BMS 8S–24S / 32S (Classic & 2021+ editions) | 8S - 32S | Total V, Current, SOC, Cells, NTC, Power | Charge, Discharge, Balancer | ✅ Verified |
| **Redodo / LiTime** | Redodo 12V 100A Smart, LiTime 12.8V 100A Smart | 4S | Total V, Current, SOC, Cells, NTC, Protections | Protection & State Read | ✅ Verified |

---

## 🏛️ Project Architecture

The project codebase is architectured around a multi-protocol BLE engine with modular decoders and diagnostic sinks:

```
Universal-BMS-Remote-Diagnostic-Monitor-ESP32-S3/
├── bin/
│   ├── firmware.bin                               # Latest compiled application firmware
│   ├── firmware.factory.bin                       # Combined binary (bootloader + partitions + app)
│   ├── firmware_esp32s3_universal_bms_v1.3.0.bin  # Standalone v1.3.0 application binary
│   └── firmware_esp32s3_universal_bms_v1.3.0.factory.bin # Combined v1.3.0 factory image
├── include/
│   ├── BmsBleClient.h                             # Universal BLE client state machine & protocol dispatchers
│   │   ├── JBD Driver                             # Services 0xFF00, 0xFFF0, 0xFEE7, NUS, PIN auth, EEPROM
│   │   ├── JK Driver                              # Service 0xFFE0, 0x97/0x96 handshake & dynamic offset alignment
│   │   ├── Daly Driver                            # Services 0xFFF0, 02F00000, commands 0x90–0x98
│   │   ├── Ant Driver                             # Active 0xDB polling, 140B stream, 0xA5 MOS switch control
│   │   └── Redodo / LiTime Driver                 # Command 0x13 query, 105B monolithic frame decoder
│   ├── BmsProtocol.h                              # Unified BmsTelemetry model, alarm bitmasks, protocol enums
│   ├── Config.h                                   # Persistent NVS configuration manager (Wi-Fi, BMS type, Tailscale)
│   ├── DebugLogger.h                              # High-speed thread-safe ring buffer for TX/RX live BLE packet traces
│   └── WebDashboard.h                             # Embedded responsive multilingual web UI (EN/UA) & gauges
├── src/
│   ├── BmsBleClient.cpp                           # Multi-protocol BLE connection, polling engines & packet decoders
│   ├── DebugLogger.cpp                            # In-memory packet trace buffer & serial mirror
│   └── main.cpp                                   # FreeRTOS task orchestration, Tailscale VPN & HTTP REST server
├── docs & specifications/
│   ├── ANT_communication_protocol_EN.1.pdf        # Official Ant BMS protocol specifications
│   ├── DALY_BMS_BLE_SPEC.md                       # Daly BLE 0xFFF0 & proprietary GATT profile spec
│   ├── dalyModbusProtocol.pdf                     # Daly Modbus RTU register mapping guide
│   └── Protocol.md                                # JBD UART/RS485 frame formats, checksums & golden captures
├── platformio.ini                                 # PlatformIO board configuration (ESP32-S3 SuperMini N4R2)
└── partitions_ota.csv                             # Dual OTA flash partition table (OTA + Tailscale)
```

---

## 🔍 Live Packet Trace: Diagnostics & Examples

The built-in **Live Packet Trace** terminal provides real-time visibility into raw Bluetooth Low Energy packets exchanged between the ESP32-S3 and the BMS.

### 📋 Protocol Frame Reference Table

| BMS Protocol | Function / Command | Transmitted Raw Frame (`[TX >>]`) | Expected Raw Response (`[RX <<]`) | Decoded Information |
| :--- | :--- | :--- | :--- | :--- |
| **JBD-BMS** | Read Basic Info (`0x03`) | `DD A5 03 00 FF FD 77` | `DD 03 00 1B ... CK 77` (34 bytes) | Pack voltage (10mV), Current (10mA), Remaining Ah, SOC%, NTC probes, Protection status |
| **JBD-BMS** | Read Cell Voltages (`0x04`) | `DD A5 04 00 FF FC 77` | `DD 04 00 [2N] [mV1] [mV2]... CK 77` | Individual cell voltages (1mV resolution, 4S–24S) |
| **JBD-BMS** | Read Device Name (`0x05`) | `DD A5 05 00 FF FB 77` | `DD 05 00 [LEN] [ASCII] CK 77` | Factory BLE broadcast name |
| **JBD-BMS** | PIN Auth (`123456`) | `DD 5A 0B 06 31 32 33 34 35 36 CK 77` | `DD 0B 00 00 CK 77` | Unlocks read/write access on PIN-protected JBD models |
| **JK-BMS** | Request Telemetry Handshake | `4E 57 00 13 00 00 00 00 06 03 00 00 00 00 00 00 68` | `4E 57 [LEN] 01 ... [~300B TLV]` | Full battery telemetry block (cells, currents, MOS state, balancer) |
| **Daly Smart** | Query Basic Info (`0x90`) | `A5 40 90 08 00 00 00 00 00 00 00 00 7D` | `A5 01 90 08 [V] [A] [SOC] CK` (13 bytes) | Pack voltage (0.1V), Current (30000 offset, 0.1A), SOC (0.1%) |
| **Daly Smart** | Query Cell Voltages (`0x95`) | `A5 40 95 08 00 00 00 00 00 00 00 00 82` | `A5 01 95 08 [Frame#] [3×Cell mV] CK` | Multi-frame individual cell voltages up to 24S |
| **Ant-BMS** | Poll Telemetry Stream (`0xDB`) | `5A 5A 00 00 00 00` *(or `DB DB`)* | `AA 55 AA FF [140 bytes] CK` | 140B stream: cell voltages up to 32S, MOS temperatures, power, remaining Ah |
| **Ant-BMS** | Toggle Charge Switch | `A5 A5 01 01 [State 01/00] CK` | `AA 55 ...` confirmation frame | Actuates charge protection MOSFET on Ant BMS |
| **Redodo / LiTime** | Query Status & Cells (`0x13`) | `00 00 04 01 13 55 AA 17` | `00 00 61 01 93 55 AA ... [105 bytes]` | 105B monolithic frame: 16S cells, pack voltage, current, temps, protections, balancer |

### 🛠️ How to Use the Diagnostic Tools

1. **Send Raw Frames via Web UI:**
   - In the web dashboard (`http://100.x.y.z/` or `http://192.168.4.1/setup`), scroll down to the **Live BLE Diagnostics** section.
   - Click any 1-Click quick test button (e.g. `[Basic Info 0x03]`, `[Cells 0x04]`, `[Name 0x05]`).
   - Or enter custom hex bytes in the input field (e.g. `DD A5 03 00 FF FD 77`) and click **Send Frame**.
2. **Send Raw Frames via REST API:**
   ```bash
   curl -X POST http://100.x.y.z/api/send-raw-ble \
        -H "Content-Type: application/json" \
        -d '{"hex":"DD A5 03 00 FF FD 77"}'
   ```
3. **Verify Responses in Live Terminal:**
   - Transmitted frames appear in cyan: `[TX >>] DD A5 03 00 FF FD 77`.
   - Incoming BMS frames appear in bright green: `[RX <<] DD 03 00 1B 14 D0 00 00 ... 77`.
   - Parsed values and status updates appear in white/yellow notifications.
4. **Export & Save Packet Logs:**
   - In the web dashboard, click **📋 Copy Log** to export all entries to your clipboard.
   - Or download the entire 200-entry ring buffer in JSON format via HTTP:
     ```bash
     curl -s http://100.x.y.z/api/debug-log > bms_packet_trace.json
     ```
   - Clear the log buffer at any time using **🧹 Clear Log** or `POST /api/clear-log`.

---

## 📐 Hardware Specifications

| Component | Specification |
| :--- | :--- |
| **Microcontroller** | ESP32-S3 SuperMini / DevKit (Dual-Core Xtensa LX7 @ 160 MHz) |
| **Memory** | 4 MB Quad-SPI Flash, **2 MB+ Embedded PSRAM (AP_3v3)** |
| **Wireless** | 2.4 GHz Wi-Fi (802.11 b/g/n) + Bluetooth 5.0 LE |
| **Power** | 5V USB-C (~60-80 mA typical power consumption) |

---

## 🧪 Testing with BMS Emulator

To safely test the diagnostic monitor without physical batteries, you can use our companion ESP32-C6 project: [JBD-BMS-Emulator-ESP32C6](https://github.com/tigersumy/JBD-BMS-Emulator-ESP32C6).

1. Flash the emulator firmware onto an ESP32-C6 development board.
2. Select your desired emulation protocol (JK, JBD, Daly, Ant, or Redodo).
3. In the diagnostic monitor web interface (`http://192.168.4.1/setup` or via Tailscale), scan for BLE devices.
4. Select the emulated BMS device and verify live cell voltages, telemetry metrics, and the real-time packet trace in the terminal.

---

## 🚀 Quick Setup Guide

### 1. Build and Upload via PlatformIO
```bash
git clone https://github.com/tigersumy/Universal-BMS-Remote-Diagnostic-Monitor-ESP32-S3.git
cd Universal-BMS-Remote-Diagnostic-Monitor-ESP32-S3
pio run -t upload
```

### 2. Initial Configuration (Captive Portal)
1. Connect your smartphone/PC to the open Wi-Fi AP: **`Universal-BMS-Setup`**.
2. Open `http://192.168.4.1/setup` in your web browser.
3. Select your local Wi-Fi SSID, enter the Wi-Fi password, and scan for nearby BMS MAC addresses.
4. Select your BMS protocol type (`Auto`, `JK-BMS`, `JBD-BMS`, `Daly-BMS`, `Ant-BMS`, or `Redodo-BMS`).
5. Provide your Tailscale Auth Key (or leave empty if already stored in NVS).
6. Click **Save & Reboot**.

### 3. Remote Access over Tailscale
Once connected to the local Wi-Fi network, the device joins your Tailnet automatically:
- **MagicDNS:** `http://jbd-bms-probe.your-tailnet.ts.net/`
- **Tailscale IP:** `http://100.x.y.z/`

---

## 📡 REST API Endpoints

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `GET` | `/api/data` | Real-time battery telemetry, cell voltages, protection switches, and VPN status |
| `GET` | `/api/debug-log` | Recent 200 entries of the live BLE packet trace |
| `POST`| `/api/send-raw-ble` | Send raw HEX frame to BMS (`{"hex":"DD A5 03 00 FF FD 77"}`) |
| `POST`| `/api/reconnect-ble` | Force BLE peripheral reconnection |
| `POST`| `/api/clear-log` | Clear in-memory diagnostic log buffer |
| `POST`| `/api/switch` | Toggle charge/discharge/balancer switches (examples below) |
| `GET` | `/api/bms/params` | Read configured EEPROM capacity parameters (Nominal Ah, Cycle Ah, Cells, Full mV) |
| `POST`| `/api/bms/read_params` | Query EEPROM configuration registers from JBD BMS (`0x10`, `0x11`, `0x2F`, `0x12`) |
| `POST`| `/api/bms/params` | Write capacity settings into JBD EEPROM via Factory Mode (`0x00` -> `0x01`) |
| `GET` | `/api/scan-ble` | Scan Bluetooth air for BMS battery systems |
| `GET` | `/api/scan-wifi` | Scan available 2.4 GHz Wi-Fi networks |
| `POST`| `/api/save-config` | Save parameters into NVS and reboot |
| `POST`| `/update` | Web OTA multipart firmware binary upload |
| `GET` | `/en` | Direct English version of the diagnostic web dashboard |

### 💡 `/api/switch` Request Examples

**1. Toggle Charging MOSFET:**
```json
{
  "switch": "charging",
  "state": true
}
```

**2. Toggle Discharging MOSFET:**
```json
{
  "switch": "discharging",
  "state": false
}
```

**3. Toggle Cell Balancer (JBD & Ant BMS):**
```json
{
  "switch": "balancer",
  "state": true
}
```

---

## 🏠 Home Assistant Integration & MQTT Discovery

The monitor provides high-speed real-time telemetry via `/api/data` (compatible with Home Assistant REST sensor integration).

```yaml
sensor:
  - platform: rest
    name: "BMS Battery Telemetry"
    resource: "http://100.x.y.z/api/data"
    scan_interval: 5
    json_attributes:
      - voltage
      - current
      - power
      - soc
      - temp_mos
      - cell_count
    value_template: "{{ value_json.voltage }} V"
```

> [!NOTE]
> Native **Home Assistant MQTT Discovery** with automatic sensor and switch entity generation is on the active roadmap and will be included in the upcoming release.

---
---

# 🇺🇦 Українська версія

**Universal BMS Remote Diagnostic Monitor** — це універсальний автономний 5-в-1 апаратно-програмний діагностичний адаптер на базі **ESP32-S3 (2MB PSRAM)** із вбудованим клієнтом **Tailscale VPN**, розроблений для дистанційного моніторингу, реверс-інжинірингу та онлайн-діагностики акумуляторних систем на базі **JK-BMS**, **JBD-BMS (Xiaoxiang / Overkill)**, **Daly-BMS**, **Ant-BMS** та **Redodo / LiTime Smart BMS** без необхідності фізичної присутності на об'єкті.

---

## 🌟 Ключові можливості

- **Універсальний мультипротокольний BLE-рушій 5-в-1:**
  - **JK-BMS (JiKong / Hankzor):** повна підтримка 24S та 32S протоколів (`0x97` / `0x96` Handshake) з автоматичною адаптацією зміщення кадрів та керуванням ключами заряду/розряду.
  - **JBD-BMS (Xiaoxiang / Overkill):** підтримка конфігурацій 4S / 8S / 16S / 24S, автоматичне розпізнавання сервісів `0xFF00`, `0xFFF0`, `0xFEE7` та Nordic UART, багаторівневий перебір PIN-авторизації (`123456`, `1234`, `000000`, пряме опитування), автопідбір режиму запису (`Write with/without Response`), керування зарядом, розрядом, балансиром та конфігурація EEPROM.
  - **Daly-BMS (Daly Smart BMS):** повна підтримка телеметрії (`0x90`–`0x98`): загальна напруга, струм (0.1A зі зміщенням 30000), SOC, напруги кожної комірки, температури NTC, статуси MOS-ключів та налаштування параметрів ємності.
  - **Ant-BMS (Classic та серія 2021+):** прийом безперервного 140-байтового потоку телеметрії (активне опитування `0xDB` / `0x5A 0x5A 0x00 0x00 0x00 0x00`), напруги комірок до 32S, температури MOS, струм, ємність та керування ключами (`0xA5`: заряд, розряд, балансування).
  - **Redodo / LiTime Smart BMS:** підтримка монолітного 105-байтового кадру телеметрії (команда опитування `0x13`), декодування напруг комірок, датчиків температури, захистів та стану балансування.
- **Вбудований клієнт Tailscale VPN (MicroLink + WireGuard lwIP):**
  - Повний віддалений доступ до пристрою з будь-якої точки світу без білої IP-адреси та без прокидання портів на роутері.
  - Оптимізовано для роботи в **2 МБ PSRAM** (буфери HTTP/2, таблиці маршрутизації до 32 пірів).
  - Апаратний Watchdog на 360 с та автоповтор DERP реле кожні 120 с.
- **Інтерактивна Live BLE діагностика (Live Packet Trace):**
  - Вбудований у веб-інтерфейс термінал з кольоровим виведенням відправлених (`[TX >>]`) та отриманих (`[RX <<]`) HEX-пакетів.
  - Кнопки швидкого тестування в 1 клік (`Basic Info 0x03`, `Cells 0x04`, `Name 0x05`, `PIN Test 123456 / 1234 / 000000`).
  - Консоль для ручної відправки довільних HEX-кадрів на BMS (`POST /api/send-raw-ble`).
  - Буфер на 200 записів у PSRAM з миттєвим експортом у буфер обміну.
- **Безпека авторизаційних даних:**
  - Ключ Tailscale Auth Key надійно захищений у NVS: інтерфейс налаштувань дозволяє підключити пристрій до Wi-Fi без розкриття чи передачі ключа кінцевому користувачеві.
- **Віддалене оновлення прошивки (Web OTA):**
  - Можливість завантаження бінарних оновлень по мережі через ендпоінт `/update` прямо через Tailscale.
- **Холодний термопрофіль 160 МГц:**
  - Знижена частота CPU для стабільної цілодобової роботи 24/7 без нагріву корпусу.

---

## 📊 Таблиця сумісності BMS

| Родина BMS | Підтримувані моделі | Кількість комірок | Телеметрія (V, A, SOC, Cells, NTC) | Керування ключами | Статус |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **JK-BMS** | JK-B1A8S20P, JK-B2A8S20P, JK-BD6A20S10P, JK-B2A24S20P | 4S - 24S / 32S | Повний пакет телеметрії | Заряд, Розряд | ✅ Перевірено |
| **JBD-BMS** | SP04S034, SP04S020, SP16S001, Overkill Solar | 4S - 24S | Повний пакет телеметрії | Заряд, Розряд, Балансир, EEPROM | ✅ Перевірено |
| **Daly Smart** | Daly 4S–24S LiFePO4 / Li-ion Smart BMS | 4S - 24S | Повний пакет телеметрії | Заряд, Розряд | ✅ Перевірено |
| **Ant-BMS** | Ant BMS 8S–24S / 32S (Classic та версії 2021+) | 8S - 32S | Повний пакет телеметрії | Заряд, Розряд, Балансир | ✅ Перевірено |
| **Redodo / LiTime** | Redodo 12V 100A Smart, LiTime 12.8V 100A Smart | 4S | Повний пакет телеметрії | Моніторинг захистів | ✅ Перевірено |

---

## 🏛️ Архітектура проєкту

Кодова база побудована навколо універсального мультипротокольного ядра з підтримкою 5 типів BMS:

```
Universal-BMS-Remote-Diagnostic-Monitor-ESP32-S3/
├── bin/
│   ├── firmware.bin                               # Актуальна скомпільована прошивка програми
│   ├── firmware.factory.bin                       # Повний комбінований бінарник (bootloader + partitions + app)
│   ├── firmware_esp32s3_universal_bms_v1.3.0.bin  # Автономний релізний бінарник v1.3.0
│   └── firmware_esp32s3_universal_bms_v1.3.0.factory.bin # Повний заводський образ v1.3.0
├── include/
│   ├── BmsBleClient.h                             # Заголовок та диспетчер станів BLE для 5 протоколів:
│   │   ├── JBD Драйвер                            # Сервіси 0xFF00, 0xFFF0, 0xFEE7, NUS, PIN auth, EEPROM
│   │   ├── JK Драйвер                             # Сервіс 0xFFE0, рукостискання 0x97/0x96, динамічне зміщення
│   │   ├── Daly Драйвер                           # Сервіси 0xFFF0, 02F00000, команди 0x90–0x98
│   │   ├── Ant Драйвер                            # Активне опитування 0xDB, 140B потік, ключі 0xA5
│   │   └── Redodo / LiTime Драйвер                # Запит 0x13, монолітний 105-байтовий декодер
│   ├── BmsProtocol.h                              # Уніфікована структура BmsTelemetry, маски помилок
│   ├── Config.h                                   # Менеджер конфігурації NVS (Wi-Fi, тип BMS, Tailscale)
│   ├── DebugLogger.h                              # Кільцевий буфер для логування BLE-трафіку (TX/RX)
│   └── WebDashboard.h                             # Вбудований багатомовний веб-інтерфейс (UA/EN)
├── src/
│   ├── BmsBleClient.cpp                           # Реалізація опитування, парсингу та відправки для всіх 5 протоколів
│   ├── DebugLogger.cpp                            # Реалізація логера пакетів у PSRAM та дзеркала UART
│   └── main.cpp                                   # Запуск FreeRTOS задач, Tailscale VPN та HTTP сервера
├── docs & specifications/
│   ├── ANT_communication_protocol_EN.1.pdf        # Офіційна інженерна специфікація Ant BMS
│   ├── DALY_BMS_BLE_SPEC.md                       # Специфікація Daly BLE 0xFFF0 та пропрієтарного сервісу
│   ├── dalyModbusProtocol.pdf                     # Довідник регістрів Daly Modbus RTU
│   └── Protocol.md                                # Специфікація кадрів, контрольних сум та відповідей JBD
├── platformio.ini                                 # Налаштування збірки PlatformIO (ESP32-S3 SuperMini N4R2)
└── partitions_ota.csv                             # Таблиця розділів Flash Dual OTA (OTA + Tailscale)
```

---

## 🔍 Live Packet Trace: Діагностика та приклади пакетів

Вбудований у веб-панель термінал **Live Packet Trace** дає змогу бачити кожен сирий байт зв'язку між ESP32-S3 та BMS у реальному часі.

### 📋 Зведена таблиця еталонних кадрів

| BMS Протокол | Команда / Функція | Відправлений кадр (`[TX >>]`) | Очікувана відповідь (`[RX <<]`) | Що декодується з кадру |
| :--- | :--- | :--- | :--- | :--- |
| **JBD-BMS** | Запит Basic Info (`0x03`) | `DD A5 03 00 FF FD 77` | `DD 03 00 1B ... CK 77` (34 байти) | Загальна напруга (10мВ), струм (10мА), залишок Ah, SOC%, NTC датчики, біти захистів |
| **JBD-BMS** | Запит напруг комірок (`0x04`)| `DD A5 04 00 FF FC 77` | `DD 04 00 [2N] [mV1] [mV2]... CK 77` | Напруга кожної комірки окремо (точність 1 мВ, від 4S до 24S) |
| **JBD-BMS** | Запит імені пристрою (`0x05`)| `DD A5 05 00 FF FB 77` | `DD 05 00 [LEN] [ASCII] CK 77` | Заводська назва пристрою у BLE-ефірі |
| **JBD-BMS** | Авторизація PIN (`123456`) | `DD 5A 0B 06 31 32 33 34 35 36 CK 77` | `DD 0B 00 00 CK 77` | Розблокування запису/читання на захищених паролем JBD |
| **JK-BMS** | Рукостискання телеметрії | `4E 57 00 13 00 00 00 00 06 03 00 00 00 00 00 00 68` | `4E 57 [LEN] 01 ... [~300B TLV]` | Повний масив комірок, струми заряду/розряду, стан балансира |
| **Daly Smart** | Запит Basic Info (`0x90`) | `A5 40 90 08 00 00 00 00 00 00 00 00 7D` | `A5 01 90 08 [V] [A] [SOC] CK` (13 байт) | Напруга (0.1V), струм (зміщення 30000, 0.1A), SOC (0.1%) |
| **Daly Smart** | Запит напруг комірок (`0x95`)| `A5 40 95 08 00 00 00 00 00 00 00 00 82` | `A5 01 95 08 [Frame#] [3×Cell mV] CK` | Багатокадрова передача напруг комірок до 24S |
| **Ant-BMS** | Опитування телеметрії (`0xDB`) | `5A 5A 00 00 00 00` *(або `DB DB`)* | `AA 55 AA FF [140 байт] CK` | 140-байтовий потік: напруги до 32S, потужність, температури MOS, ємність |
| **Ant-BMS** | Увімкнення заряду | `A5 A5 01 01 01 CK` | `AA 55 ...` кадр підтвердження | Безпосереднє перемикання MOSFET заряду |
| **Redodo / LiTime** | Опитування статусу (`0x13`) | `00 00 04 01 13 55 AA 17` | `00 00 61 01 93 55 AA ... [105 байт]` | Монолітний 105-байтовий кадр: 16S осередків, напруга, струм, температури, захисти |

### 🛠️ Практична інструкція використання діагностики

1. **Відправка сирих команд через Web UI:**
   - Відкрийте веб-панель (`http://100.x.y.z/` або `http://192.168.4.1/setup`) та перейдіть до блоку **Live BLE Diagnostics**.
   - Натисніть кнопку швидкого тесту в 1 клік (наприклад, `[Basic Info 0x03]`, `[Cells 0x04]`, `[Name 0x05]`).
   - Або введіть довільний HEX-рядок у поле вводу (наприклад, `DD A5 03 00 FF FD 77`) і натисніть кнопку **Відправити кадр**.
2. **Відправка через REST API:**
   ```bash
   curl -X POST http://100.x.y.z/api/send-raw-ble \
        -H "Content-Type: application/json" \
        -d '{"hex":"DD A5 03 00 FF FD 77"}'
   ```
3. **Аналіз відповіді в терміналі:**
   - Вихідний пакет відображається бірюзовим кольором: `[TX >>] DD A5 03 00 FF FD 77`.
   - Вхідна відповідь від BMS підсвічується зеленим: `[RX <<] DD 03 00 1B 14 D0 00 00 ... 77`.
   - Логер автоматично розбирає вміст і виводить розшифровані величини.
4. **Збереження та експорт діагностичного логу:**
   - Натисніть **📋 Копіювати лог** у веб-інтерфейсі для миттєвого збереження всього трейсу в буфер обміну.
   - Або збережіть повний кільцевий буфер на 200 записів у форматі JSON через термінал:
     ```bash
     curl -s http://100.x.y.z/api/debug-log > bms_packet_trace.json
     ```
   - За потреби очистити буфер натисніть **🧹 Очистити** або виконайте `POST /api/clear-log`.

---

## 🧪 Тестування з емулятором BMS

Для безпечного тестування монітора без фізичних акумуляторних батарей використовуйте наш проєкт емулятора на ESP32-C6: [JBD-BMS-Emulator-ESP32C6](https://github.com/tigersumy/JBD-BMS-Emulator-ESP32C6).

1. Прошийте емулятор на плату ESP32-C6.
2. Оберіть необхідний режим емуляції (JK, JBD, Daly, Ant або Redodo).
3. У веб-інтерфейсі діагностичного монітора (`http://192.168.4.1/setup` або через Tailscale) виконайте сканування Bluetooth.
4. Підключіться до емульованої BMS та перевірте передачу телеметрії, напруг комірок та живий трейс пакетів у веб-терміналі.

---

## 🚀 Швидкий запуск

### 1. Збірка та прошивка через PlatformIO
```bash
# Клонування репозиторію
git clone https://github.com/tigersumy/Universal-BMS-Remote-Diagnostic-Monitor-ESP32-S3.git
cd Universal-BMS-Remote-Diagnostic-Monitor-ESP32-S3

# Збірка та завантаження через USB
pio run -t upload
```

### 2. Первинне налаштування (Captive Portal)
1. Підключіться зі смартфона/ПК до відкритої точки доступу **`Universal-BMS-Setup`**.
2. Перейдіть у браузері на `http://192.168.4.1/setup`.
3. Оберіть ваш домашній Wi-Fi, вкажіть пароль та натисніть **🔍 Знайти BMS в ефірі**.
4. Оберіть протокол вашої BMS (`Auto`, `JK-BMS`, `JBD-BMS`, `Daly-BMS`, `Ant-BMS` або `Redodo-BMS`).
5. Вкажіть ваш Tailscale Auth Key (або залиште порожнім, якщо вже збережено в NVS).
6. Натисніть **💾 Зберегти та перезавантажити**.

### 3. Віддалений доступ через Tailscale
Після підключення до домашнього Wi-Fi пристрій автоматично з'явиться у вашій мережі Tailscale:
- **MagicDNS:** `http://jbd-bms-probe.your-tailnet.ts.net/`
- **Пряма Tailscale IP:** `http://100.x.y.z/`

---

## 📡 Ендпоінти REST API

| Метод | Ендпоінт | Опис |
| :--- | :--- | :--- |
| `GET` | `/api/data` | Телеметрія батареї в реальному часі, напруги комірок, стан ключів та VPN |
| `GET` | `/api/debug-log` | Останні 200 записів живого BLE-трейсу пакетів |
| `POST`| `/api/send-raw-ble` | Відправка довільного HEX-кадру на BMS (`{"hex":"DD A5 03 00 FF FD 77"}`) |
| `POST`| `/api/reconnect-ble` | Примусове перепідключення до BLE-периферії |
| `POST`| `/api/clear-log` | Очищення кільцевого буфера діагностичного логу |
| `POST`| `/api/switch` | Перемикання ключів заряду/розряду/балансира |
| `GET` | `/api/bms/params` | Читання налаштувань ємності EEPROM (Ah, цикл, комірки, повна напруга) |
| `POST`| `/api/bms/read_params` | Запит регістрів конфігурації EEPROM JBD (`0x10`, `0x11`, `0x2F`, `0x12`) |
| `POST`| `/api/bms/params` | Запис ємності в EEPROM JBD через Factory Mode (`0x00` -> `0x01`) |
| `GET` | `/api/scan-ble` | Сканування ефіру Bluetooth на наявність BMS систем |
| `GET` | `/api/scan-wifi` | Пошук доступних Wi-Fi мереж 2.4 GHz |
| `POST`| `/api/save-config` | Збереження параметрів у NVS та перезавантаження |
| `POST`| `/update` | Завантаження бінарного оновлення прошивки по мережі (OTA) |
| `GET` | `/en` | Прямий перехід на англійську версію діагностичної панелі |

### 💡 Приклади виклику `/api/switch`

**1. Керування зарядним MOSFET:**
```json
{
  "switch": "charging",
  "state": true
}
```

**2. Керування розрядним MOSFET:**
```json
{
  "switch": "discharging",
  "state": false
}
```

**3. Керування балансиром (JBD та Ant BMS):**
```json
{
  "switch": "balancer",
  "state": true
}
```

---

## 🤝 Acknowledgements & Credits

This project builds upon and integrates several exceptional open-source libraries, protocol reverse-engineering efforts, and community research:
- **[MicroLink by CamM2325](https://github.com/CamM2325/microlink):** The embedded Tailscale client implementation for ESP32 (ts2021 control protocol, Noise IK mutual authentication, DERP relay protocol, and magicsock architecture) is adapted from the MicroLink project.
- **[wireguard-lwip by Daniel Hope](https://github.com/djp952/wireguard-lwip):** Lightweight WireGuard implementation for lwIP providing ChaCha20-Poly1305, BLAKE2s, and Curve25519 cryptographic primitives.
- **[NimBLE-Arduino by h2zero](https://github.com/h2zero/NimBLE-Arduino):** Memory-efficient Bluetooth Low Energy stack for ESP32.
- **[ArduinoJson by Benoît Blanchon](https://arduinojson.org/):** High-performance JSON parser for embedded systems.
- **[sshoecraft/jbdtool](https://github.com/sshoecraft/jbdtool):** Reference C implementation for Linux/Embedded monitoring and configuration of JBD BMS. Provides a comprehensive EEPROM register map, configuration parameters, capacity calibration routines, and robust timeout handling logic.
- **[fl4p/batmon-ha](https://github.com/fl4p/batmon-ha):** Outstanding Bluetooth Battery Monitor for Home Assistant. Invaluable reference for multi-BMS Bluetooth Low Energy (BLE) reverse engineering, packet decoding specifications, and GATT communication profiles across Daly, JBD, JK, ANT, and other smart battery management systems.
- **[syssi/esphome-jbd-bms](https://github.com/syssi/esphome-jbd-bms):** Modern reference implementation for the ESP32 / ESPHome ecosystem. Provides validated algorithms for handling fragmented UART/BLE packet streams, full 16-bit protection/fault mask decoding, and MOSFET charge/discharge (`0xE1`) and balancer (`0xE2`) switching logic.
- **[wholybee/esp32-smartBMSdisplay](https://github.com/wholybee/esp32-smartBMSdisplay):** Practical ESP32 BLE implementation covering `0xFF00` / `0xFFF0` / `0xFEE7` GATT services, request queue processing, and alarm/disconnect handling patterns.
- **[ieb/N2KLifePo4 (JBD-BMS-SERIAL-INTERFACE.md)](https://github.com/ieb/N2KLifePo4/blob/main/JBD-BMS-SERIAL-INTERFACE.md):** Clean, structured specification and register reference for the JBD serial interface detailing byte offsets, data formats (U16, S16), scaling factors, and measurement units (10 mV, 10 mA, 0.1 K, 10 mAh).

---

## 📄 License

This project is licensed under the [MIT License](LICENSE).
Feel free to use, modify, and integrate into your home energy systems!
