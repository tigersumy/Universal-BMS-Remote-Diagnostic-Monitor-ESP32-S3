# Universal BMS Remote Diagnostic Monitor & Tailscale Probe (ESP32-S3)

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Build%20Passing-brightgreen.svg)](https://platformio.org/)
[![Hardware](https://img.shields.io/badge/Hardware-ESP32--S3%20SuperMini%20(2MB%20PSRAM)-blue.svg)](https://www.espressif.com/)
[![Tailscale](https://img.shields.io/badge/Tailscale-VPN%20Embedded-darkblue.svg)](https://tailscale.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

*Read this in other languages: [English](#-english-overview) | [Українська](#-українська-версія)*

---

# 🇬🇧 English Overview

**Universal BMS Remote Diagnostic Monitor & Tailscale Probe** is a standalone hardware-software diagnostic bridge built on the **ESP32-S3 (2MB PSRAM)** with embedded **Tailscale VPN**. It enables remote monitoring, protocol reverse engineering, and live diagnostics of battery management systems based on **JK-BMS** and **JBD-BMS (Xiaoxiang / Overkill / Smart BMS)** without requiring physical on-site presence.

---

## 🌟 Key Features

- **Multi-Protocol BLE Engine:**
  - **JBD-BMS (Xiaoxiang):** Supports 4S / 8S / 16S / 24S battery configurations, automatic discovery of `0xFF00`, `0xFFF0`, `0xFEE7`, and Nordic UART services, multi-tier PIN authentication (`123456`, `1234`, `000000`, direct poll fallback), and automatic write mode detection (`Write with/without Response`).
  - **JK-BMS (JiKong / Hankzor):** Complete 24S and 32S protocol support (0x97 / 0x96 handshake) with dynamic frame offset alignment.
- **Embedded Tailscale VPN Client (MicroLink + WireGuard lwIP):**
  - Secure remote access from anywhere across the globe without port forwarding, NAT traversal issues, or public IP addresses.
  - Dynamically allocated in **2 MB PSRAM** buffers supporting up to 32 active peers in your tailnet.
  - 360-second hardware watchdog and 120-second periodic DERP reconnect cycle.
- **Interactive Live BLE Diagnostics (Live Packet Trace):**
  - Web-embedded terminal displaying color-coded transmitted (`[TX >>]`) and received (`[RX <<]`) raw HEX frames.
  - 1-Click quick test buttons (`Basic Info 0x03`, `Cells 0x04`, `Device Name 0x05`, `PIN Auth Tests 123456 / 1234 / 000000`).
  - Raw HEX command console allowing manual injection of arbitrary frames over BLE (`POST /api/send-raw-ble`).
  - 200-entry in-memory ring buffer with clipboard export.
- **Credential Privacy:**
  - Tailscale Auth Keys are stored in non-volatile storage (NVS) and never exposed in plain text on the web interface.
- **Web OTA Firmware Flashing:**
  - Upload pre-compiled firmware binaries remotely via `/update` over Tailscale.
- **Thermal Optimization:**
  - Underclocked CPU frequency (160 MHz) ensures continuous, cool 24/7 standalone operation.

---

## 📐 Hardware Specifications

| Component | Specification |
| :--- | :--- |
| **Microcontroller** | ESP32-S3 SuperMini / DevKit (Dual-Core Xtensa LX7 @ 160 MHz) |
| **Memory** | 4 MB Quad-SPI Flash, **2 MB+ Embedded PSRAM (AP_3v3)** |
| **Wireless** | 2.4 GHz Wi-Fi (802.11 b/g/n) + Bluetooth 5.0 LE |
| **Power** | 5V USB-C (~60-80 mA typical power consumption) |

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
4. Provide your Tailscale Auth Key (or leave empty if already stored in NVS).
5. Click **Save & Reboot**.

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
| `POST`| `/api/switch` | Toggle charge/discharge protection MOSFETs (`{"switch":"charging","state":true}`) |
| `GET` | `/api/bms/params` | Read configured EEPROM capacity parameters (Nominal Ah, Cycle Ah, Cells, Full mV) |
| `POST`| `/api/bms/read_params` | Query EEPROM configuration registers from JBD BMS (`0x10`, `0x11`, `0x2F`, `0x12`) |
| `POST`| `/api/bms/params` | Write capacity settings into JBD EEPROM via Factory Mode (`0x00` -> `0x01`) |
| `GET` | `/api/scan-ble` | Scan Bluetooth air for JK/JBD battery systems |
| `GET` | `/api/scan-wifi` | Scan available 2.4 GHz Wi-Fi networks |
| `POST`| `/api/save-config` | Save parameters into NVS and reboot |
| `POST`| `/update` | Web OTA multipart firmware binary upload |
| `GET` | `/en` | Direct English version of the diagnostic web dashboard |

---
---

# 🇺🇦 Українська версія

**Universal BMS Remote Diagnostic Monitor** — це автономний апаратно-програмний діагностичний адаптер на базі **ESP32-S3 (2MB PSRAM)** із вбудованим клієнтом **Tailscale VPN**, розроблений для дистанційного моніторингу, реверс-інжинірингу та онлайн-діагностики акумуляторних систем на базі **JK-BMS** та **JBD-BMS (Xiaoxiang / Overkill / Smart BMS)** без необхідності фізичної присутності на об'єкті.

---

## 🌟 Ключові можливості

- **Універсальний мультипротокольний BLE-рушій:**
  - **JBD-BMS (Xiaoxiang):** підтримка конфігурацій 4S / 8S / 16S / 24S, автоматичне розпізнавання сервісів `0xFF00`, `0xFFF0`, `0xFEE7` та Nordic UART, багаторівневий перебір PIN-авторизації (`123456`, `1234`, `000000`, пряме опитування), автопідбір режиму запису (`Write with/without Response`).
  - **JK-BMS (JiKong / Hankzor):** повна підтримка 24S та 32S протоколу (0x97 / 0x96 Handshake) з автоматичною адаптацією зміщення кадрів.
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

## 📐 Апаратні вимоги

| Компонент | Специфікація |
| :--- | :--- |
| **Мікроконтролер** | ESP32-S3 SuperMini / DevKit (Dual Core Xtensa LX7 @ 160 MHz) |
| **Пам'ять** | 4 MB Flash (Quad SPI), **2 MB+ Embedded PSRAM** |
| **Бездротові інтерфейси** | Wi-Fi 2.4 GHz (802.11 b/g/n) + Bluetooth 5.0 LE |
| **Живлення** | 5V USB-C (споживання ~60-80 mA) |

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
4. Вкажіть ваш Tailscale Auth Key (або залиште порожнім, якщо вже збережено в NVS).
5. Натисніть **💾 Зберегти та перезавантажити**.

### 3. Віддалений доступ через Tailscale
Після підключення до домашнього Wi-Fi пристрій автоматично з'явиться у вашій мережі Tailscale:
- **MagicDNS:** `http://jbd-bms-probe.your-tailnet.ts.net/`
- **Пряма Tailscale IP:** `http://100.x.y.z/`

---

## 🤝 Acknowledgements & Credits

This project builds upon and integrates several exceptional open-source libraries:
- **[MicroLink by CamM2325](https://github.com/CamM2325/microlink):** The embedded Tailscale client implementation for ESP32 (ts2021 control protocol, Noise IK mutual authentication, DERP relay protocol, and magicsock architecture) is adapted from the MicroLink project.
- **[wireguard-lwip by Daniel Hope](https://github.com/djp952/wireguard-lwip):** Lightweight WireGuard implementation for lwIP providing ChaCha20-Poly1305, BLAKE2s, and Curve25519 cryptographic primitives.
- **[NimBLE-Arduino by h2zero](https://github.com/h2zero/NimBLE-Arduino):** Memory-efficient Bluetooth Low Energy stack for ESP32.
- **[ArduinoJson by Benoît Blanchon](https://arduinojson.org/):** High-performance JSON parser for embedded systems.

---

## 📄 License

This project is licensed under the [MIT License](LICENSE).
Feel free to use, modify, and integrate into your home energy systems!
