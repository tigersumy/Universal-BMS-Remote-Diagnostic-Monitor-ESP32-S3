# Специфікація комунікаційного протоколу Daly BMS по Bluetooth Low Energy (BLE)
### Для проєкту: «Універсальний Адаптер JKBMS / JBDBMS / DALY ESP32-S3» (Google Antigravity)

---

## 📌 1. Огляд та архітектурне призначення

У проєкті універсального бездротового адаптера на базі **ESP32-S3** мікроконтролер виконує роль **BLE-клієнта (Central)**, який підключається по Bluetooth до батарейних систем управління (BMS), зчитує телеметрію та транслює її в локальні протоколи (MQTT на Mac mini, емуляція Growatt/Pylontech для інвертора Datouboss/Sandisolar, веб-інтерфейс або HomeKit).

Для **Daly Smart BMS** зв'язок по BLE здійснюється не через Modbus RTU, а через фірмовий **13-байтний фреймовий протокол UART-over-BLE (заголовок 0xA5)**. При цьому внутрішня семантика даних (зсув струму 30000, зміщення температури -40 °C, бітові маски тривог) на 100% збігається з офіційним реєстровим описом `dalyModbusProtocol.pdf`.

---

## 📱 2. Специфікація транспортного рівня BLE (GATT Profile)

### 2.1. Реклама та ідентифікація (Advertising)
* **Device Name префікси:** `DL-` (наприклад `DL-321200000000`), `Daly-BMS`, `BMS-` або 12-значний серійний номер.
* **Advertised Service UUID:** Найчастіше транслює сервіс `0xFFF0`.

### 2.2. GATT Сервіси та Характеристики
У більшості модулів Daly Bluetooth використовується кастомний профіль віртуального послідовного порту:

| Елемент GATT | 16-бітний UUID | Повний 128-бітний UUID | Властивості (Properties) | Призначення |
| :--- | :---: | :--- | :--- | :--- |
| **Primary Service** | `0xFFF0` | `0000fff0-0000-1000-8000-00805f9b34fb` | - | Головний сервіс зв'язку Daly |
| **Write Characteristic (TX)** | `0xFFF2` | `0000fff2-0000-1000-8000-00805f9b34fb` | `WRITE`, `WRITE_NO_RSP` | Надсилання опитувальних команд від ESP32 до BMS |
| **Notify Characteristic (RX)**| `0xFFF1` | `0000fff1-0000-1000-8000-00805f9b34fb` | `NOTIFY`, `READ` | Отримання пакетів телеметрії від BMS до ESP32 |

> *Примітка:* Деякі зовнішні Bluetooth-донгли Daly (на чіпах nRF) можуть використовувати стандартний **Nordic UART Service (NUS)**:
> * Service: `6e400001-b5a3-f393-e0a9-e50e24dcca9e`
> * Write (RXD): `6e400002-b5a3-f393-e0a9-e50e24dcca9e`
> * Notify (TXD): `6e400003-b5a3-f393-e0a9-e50e24dcca9e`

---

## 📦 3. Структура протокольного кадру (13 байтів)

Усі пакети обміну (як запити від хоста, так і відповіді від BMS) мають **фіксовану довжину 13 байтів**:

```text
  +------+------+------+------+-------------------------------+------+
  | Byte |  0   |  1   |  2   | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11|  12  |
  +------+------+------+------+-------------------------------+------+
  | Назва| START| ADDR | CMD  |LEN|         DATA (8 байтів)       | CHK  |
  +------+------+------+------+-------------------------------+------+
```

### 3.1. Запит від ESP32 до BMS (Host Request):
* `Byte 0` (**START**): Завжди **`0xA5`** (стартовий байт синхронізації).
* `Byte 1` (**ADDR**): Адреса хоста — **`0x40`** (або `0x80`).
* `Byte 2` (**CMD**): Код запитуваної команди (`0x90` – `0x98`).
* `Byte 3` (**LEN**): Довжина поля даних — завжди **`0x08`**.
* `Byte 4..11` (**DATA**): 8 байтів корисного навантаження (у запитах читання заповнюються нулями `0x00`).
* `Byte 12` (**CHK**): Контрольна сума — молодший байт суми перших 12 байтів:
  $$\text{Checksum} = \left(\sum_{i=0}^{11} \text{Byte}[i]\right) \ \& \ \text{0xFF}$$

### 3.2. Відповідь від BMS до ESP32 (BMS Response):
* `Byte 0` (**START**): **`0xA5`**.
* `Byte 1` (**ADDR**): Адреса пристрою — **`0x01`** (Slave ID).
* `Byte 2` (**CMD**): Відлуння коду команди (`0x90` – `0x98`).
* `Byte 3` (**LEN**): Довжина даних — **`0x08`**.
* `Byte 4..11` (**DATA**): 8 байтів повернутих параметрів.
* `Byte 12` (**CHK**): Контрольна сума $\sum_{i=0}^{11} \text{Byte}[i] \ \& \ \text{0xFF}$.

---

## 📑 4. Повний набір команд та розпакування даних

### ⚡ 4.1. Команда `0x90`: Загальна напруга, струм та SOC
* **Запит:** `A5 40 90 08 00 00 00 00 00 00 00 00 7D`
* **Відповідь:** `A5 01 90 08 [D0 D1 D2 D3 D4 D5 D6 D7] [CHK]`
  * `D0..D1` (uint16_t MSB): **Загальна напруга батареї (Total Voltage)**.
    $$\text{Voltage (V)} = \frac{\text{raw}}{10.0}$$
    *(Приклад: `0x0214` = 532 $\rightarrow$ 53.2 В)*.
  * `D2..D3` (uint16_t MSB): Напруга збору / резерв.
  * `D4..D5` (uint16_t MSB): **Струм пакета (Current)**. Використовує апаратний зсув **30000**:
    $$\text{Current (A)} = \frac{\text{raw} - 30000}{10.0}$$
    * Значення `30000` $\rightarrow$ `0.0 А` (спокій).
    * Значення `30150` $\rightarrow$ `+15.0 А` (заряд).
    * Значення `29400` $\rightarrow$ `-60.0 А` (розряд).
  * `D6..D7` (uint16_t MSB): **Рівень заряду (SOC)**.
    $$\text{SOC (\%)} = \frac{\text{raw}}{10.0}$$
    *(Приклад: `0x03E8` = 1000 $\rightarrow$ 100.0%)*.

---

### 🔋 4.2. Команда `0x91`: Екстремуми напруг комірок (Max / Min Cell)
* **Запит:** `A5 40 91 08 00 00 00 00 00 00 00 00 7E`
* **Відповідь:**
  * `D0..D1` (uint16_t MSB): **Максимальна напруга комірки (Max Cell Voltage)** в мВ (наприклад `3350` = 3.350 В).
  * `D2` (uint8_t): **Номер комірки з максимальною напругою** (1..48).
  * `D3..D4` (uint16_t MSB): **Мінімальна напруга комірки (Min Cell Voltage)** в мВ.
  * `D5` (uint8_t): **Номер комірки з мінімальною напругою** (1..48).
  * `D6..D7`: Резерв / Різниця (Delta mV).

---

### 🌡️ 4.3. Команда `0x92`: Екстремуми температур (Max / Min Temp)
* **Запит:** `A5 40 92 08 00 00 00 00 00 00 00 00 7F`
* **Відповідь:**
  * `D0` (uint8_t): **Максимальна температура**. Зсув **40**:
    $$T_{\max} (^\circ\text{C}) = \text{D0} - 40$$
  * `D1` (uint8_t): Номер датчика максимальної температури (1..8).
  * `D2` (uint8_t): **Мінімальна температура**:
    $$T_{\min} (^\circ\text{C}) = \text{D2} - 40$$
  * `D3` (uint8_t): Номер датчика мінімальної температури (1..8).

---

### ⚙️ 4.4. Команда `0x93`: Статус MOSFET та залишкова ємність
* **Запит:** `A5 40 93 08 00 00 00 00 00 00 00 00 80`
* **Відповідь:**
  * `D0` (uint8_t): **Статус роботи АКБ**:
    * `0` = Очікування (Stationary / Idle).
    * `1` = Заряджання (Charging).
    * `2` = Розряджання (Discharging).
  * `D1` (uint8_t): **Charge MOSFET State**: `0` = Вимкнено (OFF), `1` = Увімкнено (ON).
  * `D2` (uint8_t): **Discharge MOSFET State**: `0` = Вимкнено (OFF), `1` = Увімкнено (ON).
  * `D3` (uint8_t): BMS Life / лічильник серцебиття (0..255).
  * `D4..D7` (uint32_t MSB): **Залишкова ємність (Remaining Capacity)**:
    $$\text{Capacity (Ah)} = \frac{\text{raw}}{1000.0}$$
    *(Приклад: `150000` $\rightarrow$ 150.000 Аг)*.

---

### 📊 4.5. Команда `0x94`: Конфігурація пакета та цикли
* **Запит:** `A5 40 94 08 00 00 00 00 00 00 00 00 81`
* **Відповідь:**
  * `D0` (uint8_t): **Кількість підключених комірок (Cell Count)** (наприклад `4`, `8`, `16`).
  * `D1` (uint8_t): Кількість активних NTC-датчиків температури (наприклад `2`).
  * `D2` (uint8_t): Стан зарядного пристрою (`0` = відключено, `1` = підключено).
  * `D3` (uint8_t): Стан навантаження (`0` = відключено, `1` = підключено).
  * `D4`: Стан дискретних входів (DI status).
  * `D5..D6` (uint16_t MSB): **Кількість повних циклів (Cycle Count)**.

---

### 🧪 4.6. Команда `0x95`: Покомірковий моніторинг (Cell Voltages)
* **Запит:** `A5 40 95 08 00 00 00 00 00 00 00 00 82`
* **Особливість:** BMS відповідає **декількома послідовними 13-байтними кадрами** (по 3 комірки в кожному кадрі):
  * **Кадр 1:** `D0` = `0x01` (номер кадру 1).
    * `D1..D2` (uint16_t MSB): Напруга комірки 1 (мВ).
    * `D3..D4` (uint16_t MSB): Напруга комірки 2 (мВ).
    * `D5..D6` (uint16_t MSB): Напруга комірки 3 (мВ).
  * **Кадр 2:** `D0` = `0x02` (номер кадру 2).
    * `D1..D2`: Напруга комірки 4 (мВ).
    * `D3..D4`: Напруга комірки 5 (мВ).
    * `D5..D6`: Напруга комірки 6 (мВ).
  * ... і так далі до досягнення загальної кількості комірок.

---

### ⚖️ 4.7. Команда `0x97`: Стан активного балансування
* **Запит:** `A5 40 97 08 00 00 00 00 00 00 00 00 84`
* **Відповідь:**
  * `D0..D5` (48 бітів): Бітова маска балансування (Bit 0 = Cell 1, Bit 1 = Cell 2 ... Bit 47 = Cell 48).
  * `1` = комірка балансується в даний момент, `0` = не балансується.

---

### 🚨 4.8. Команда `0x98`: Повні бітові маски аварій та помилок
* **Запит:** `A5 40 98 08 00 00 00 00 00 00 00 00 85`
* **Відповідь (8 байтів статусів аварій):**
  * **`D0` (Напруги):**
    * `bit 0`: Перевищення напруги комірки (Рівень 1)
    * `bit 1`: Перевищення напруги комірки (Рівень 2)
    * `bit 2`: Заниження напруги комірки (Рівень 1)
    * `bit 3`: Заниження напруги комірки (Рівень 2)
    * `bit 4`: Перевищення загальної напруги АКБ (Рівень 1)
    * `bit 5`: Перевищення загальної напруги АКБ (Рівень 2)
    * `bit 6`: Заниження загальної напруги АКБ (Рівень 1)
    * `bit 7`: Заниження загальної напруги АКБ (Рівень 2)
  * **`D1` (Температури):**
    * `bit 0`: Перегрів при заряді (Рівень 1)
    * `bit 1`: Перегрів при заряді (Рівень 2)
    * `bit 2`: Переохолодження при заряді (Рівень 1)
    * `bit 3`: Переохолодження при заряді (Рівень 2)
    * `bit 4`: Перегрів при розряді (Рівень 1)
    * `bit 5`: Перегрів при розряді (Рівень 2)
    * `bit 6`: Переохолодження при розряді (Рівень 1)
    * `bit 7`: Переохолодження при розряді (Рівень 2)
  * **`D2` (Струми та SOC):**
    * `bit 0`: Перевантаження по струму заряду (Рівень 1)
    * `bit 1`: Перевантаження по струму заряду (Рівень 2)
    * `bit 2`: Перевантаження по струму розряду (Рівень 1)
    * `bit 3`: Перевантаження по струму розряду (Рівень 2)
    * `bit 4`: SOC занадто високий (Рівень 1)
    * `bit 5`: SOC занадто високий (Рівень 2)
    * `bit 6`: SOC занадто низький (Рівень 1)
    * `bit 7`: SOC занадто низький (Рівень 2)
  * **`D3` (Дельти):**
    * `bit 0..1`: Велика різниця напруг комірок (Рівні 1 / 2)
    * `bit 2..3`: Велика різниця температур сенсорів (Рівні 1 / 2)
  * **`D4` (Стан ключів MOS):**
    * `bit 0`: Перегрів MOSFET заряду
    * `bit 1`: Перегрів MOSFET розряду
    * `bit 2`: Обрив датчика температури ключа заряду
    * `bit 3`: Обрив датчика температури ключа розряду
    * `bit 4..5`: Залипання (пробій) ключів заряду / розряду
    * `bit 6..7`: Аварія ланцюга драйвера MOSFET
  * **`D5` (Апаратні збої):**
    * `bit 0`: Збій мікросхеми вимірювання (AFE failure)
    * `bit 1`: Обрив дроту балансування комірки
    * `bit 2`: Обрив NTC датчика температури
    * `bit 3`: Помилка пам'яті EEPROM
    * `bit 4`: Збій годинника RTC

---

### 🔘 4.9. Команди керування ключами MOSFET (Write)
Для віддаленого вмикання або вимикання ключів заряду/розряду надсилається команда `0xD9`:
* **Керування ключем заряду (Charge MOS):**
  * Увімкнути: `A5 40 D9 08 01 00 00 00 00 00 00 00 C2`
  * Вимкнути: `A5 40 D9 08 00 00 00 00 00 00 00 00 C1`
* **Керування ключем розряду (Discharge MOS):**
  * Увімкнути: `A5 40 D9 08 00 01 00 00 00 00 00 00 C2`
  * Вимкнути: `A5 40 D9 08 00 00 00 00 00 00 00 00 C1`

---

## 🔄 5. Порівняльна таблиця: Modbus RTU vs BLE 0xA5 Protocol

| Параметр | Modbus RTU (dalyModbusProtocol.pdf) | BLE UART Протокол (0xA5) | Коефіцієнт / Формула |
| :--- | :---: | :---: | :---: |
| **Загальна напруга** | Регістр `0x28` | Команда `0x90`, байти 4..5 | $\text{raw} / 10.0$ (В) |
| **Струм АКБ** | Регістр `0x29` | Команда `0x90`, байти 8..9 | $(\text{raw} - 30000) / 10.0$ (А) |
| **SOC (%)** | Регістр `0x2A` | Команда `0x90`, байти 10..11 | $\text{raw} / 10.0$ (%) |
| **Max / Min комірка** | Регістри `0x2B`, `0x2C` | Команда `0x91`, байти 4..9 | $1\text{ мВ}$, номери комірок 1..48 |
| **Температури** | Регістри `0x20`–`0x27` | Команда `0x92`, `0x96` | $\text{raw} - 40$ ($^\circ\text{C}$) |
| **Залишкова ємність** | Регістр `0x30` | Команда `0x93`, байти 8..11 | $\text{raw} / 1000.0$ (Аг) |
| **Напруги комірок 1..48** | Регістри `0x00`–`0x1F` | Команда `0x95` (кадри по 3 шт.) | $1\text{ мВ}$ на комірку |
| **Аварії / Помилки** | Регістри `0x3A`–`0x3D` | Команда `0x98`, байти 4..11 | Бітові маски ідентичні |

---

## 💻 6. Готовий C++ модуль клієнта NimBLE для ESP32-S3

Цей драйвер оптимізовано для роботи в середовищі **FreeRTOS** без блокування Wi-Fi або інверторного зв'язку.

```cpp
#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>

#define DALY_SERVICE_UUID        "fff0"
#define DALY_CHAR_NOTIFY_UUID    "fff1"
#define DALY_CHAR_WRITE_UUID     "fff2"

struct DalyBmsData {
    float total_voltage = 0.0f;
    float current = 0.0f;
    float soc = 0.0f;
    float remaining_capacity_ah = 0.0f;
    uint16_t cell_count = 0;
    uint16_t cycles = 0;
    float cell_voltages[48] = {0};
    int8_t temp_sensors[8] = {0};
    bool charge_mos_enabled = false;
    bool discharge_mos_enabled = false;
    uint8_t fault_bits[8] = {0};
    bool connected = false;
    uint32_t last_update_ms = 0;
};

class DalyBmsBleClient : public NimBLEClientCallbacks {
private:
    NimBLEClient* pClient = nullptr;
    NimBLERemoteCharacteristic* pWriteChar = nullptr;
    NimBLERemoteCharacteristic* pNotifyChar = nullptr;
    std::string targetMac = "";
    uint8_t pollStep = 0;
    uint32_t lastPollTime = 0;

    static uint8_t calcChecksum(const uint8_t* data, size_t len) {
        uint8_t sum = 0;
        for (size_t i = 0; i < len; i++) sum += data[i];
        return sum;
    }

    void sendCommand(uint8_t cmd) {
        if (!pWriteChar) return;
        uint8_t frame[13] = {0xA5, 0x40, cmd, 0x08, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        frame[12] = calcChecksum(frame, 12);
        pWriteChar->writeValue(frame, sizeof(frame), false);
    }

    void parseFrame(const uint8_t* pData, size_t length) {
        if (length < 13 || pData[0] != 0xA5) return;
        if (calcChecksum(pData, 12) != pData[12]) return;

        uint8_t cmd = pData[2];
        const uint8_t* d = &pData[4];

        switch (cmd) {
            case 0x90: // V, I, SOC
                data.total_voltage = ((d[0] << 8) | d[1]) * 0.1f;
                data.current = (((int32_t)((d[4] << 8) | d[5])) - 30000) * 0.1f;
                data.soc = ((d[6] << 8) | d[7]) * 0.1f;
                data.last_update_ms = millis();
                break;

            case 0x93: // MOS Status & Remaining Cap
                data.charge_mos_enabled = (d[1] == 1);
                data.discharge_mos_enabled = (d[2] == 1);
                data.remaining_capacity_ah = ((uint32_t)d[4] << 24 | (uint32_t)d[5] << 16 | (uint32_t)d[6] << 8 | d[7]) * 0.001f;
                break;

            case 0x94: // Cell count & Cycles
                data.cell_count = d[0];
                data.cycles = (d[5] << 8) | d[6];
                break;

            case 0x95: { // Multi-frame cell voltages
                uint8_t frameNo = d[0];
                uint8_t startIdx = (frameNo - 1) * 3;
                if (startIdx < 48) {
                    data.cell_voltages[startIdx] = ((d[1] << 8) | d[2]) * 0.001f;
                    data.cell_voltages[startIdx + 1] = ((d[3] << 8) | d[4]) * 0.001f;
                    data.cell_voltages[startIdx + 2] = ((d[5] << 8) | d[6]) * 0.001f;
                }
                break;
            }

            case 0x98: // Fault masks
                for (int i = 0; i < 8; i++) data.fault_bits[i] = d[i];
                break;
        }
    }

public:
    DalyBmsData data;

    void init(const std::string& mac) {
        targetMac = mac;
    }

    bool connect() {
        if (targetMac.empty()) return false;
        if (!pClient) {
            pClient = NimBLEDevice::createClient();
            pClient->setClientCallbacks(this, false);
            pClient->setConnectTimeout(10);
        }

        if (pClient->isConnected()) return true;

        NimBLEAddress addr(targetMac);
        if (!pClient->connect(addr)) return false;

        NimBLERemoteService* pSvc = pClient->getService(DALY_SERVICE_UUID);
        if (!pSvc) {
            pClient->disconnect();
            return false;
        }

        pWriteChar = pSvc->getCharacteristic(DALY_CHAR_WRITE_UUID);
        pNotifyChar = pSvc->getCharacteristic(DALY_CHAR_NOTIFY_UUID);

        if (!pWriteChar || !pNotifyChar) {
            pClient->disconnect();
            return false;
        }

        if (pNotifyChar->canNotify()) {
            pNotifyChar->subscribe(true, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
                this->parseFrame(pData, length);
            });
        }

        data.connected = true;
        return true;
    }

    void onDisconnect(NimBLEClient* pClient) override {
        data.connected = false;
        pWriteChar = nullptr;
        pNotifyChar = nullptr;
    }

    void loop() {
        if (!data.connected) {
            if (millis() - lastPollTime > 10000) {
                lastPollTime = millis();
                connect();
            }
            return;
        }

        // Циклічний неблокуючий опитувач кожні 500 мс (1 крок)
        if (millis() - lastPollTime > 500) {
            lastPollTime = millis();
            switch (pollStep) {
                case 0: sendCommand(0x90); break; // V, I, SOC
                case 1: sendCommand(0x93); break; // MOS status, Cap
                case 2: sendCommand(0x94); break; // Cell count, Cycles
                case 3: sendCommand(0x95); break; // Cell voltages
                case 4: sendCommand(0x98); break; // Fault bits
            }
            pollStep = (pollStep + 1) % 5;
        }
    }
};
```

---

## 🚀 7. Покрокова інтеграція в проєкт Antigravity

1. **Скопіюйте файл специфікації:** Розмістіть цей документ у каталозі вашого адаптера `docs/DALY_BMS_BLE_SPEC.md`.
2. **Підключення в PlatformIO:** Переконайтеся, що в `platformio.ini` додано бібліотеку `NimBLE-Arduino`:
   ```ini
   lib_deps =
       h2zero/NimBLE-Arduino@^1.4.2
   ```
3. **Уніфікація структури:** Поєднайте екземпляри `JkBmsBleClient`, `JbdBmsBleClient` та `DalyBmsBleClient` через єдиний поліморфний інтерфейс `IBmsClient`, щоб ваш емулятор Growatt та MQTT-паблішер брали дані з активної BMS прозоро.
