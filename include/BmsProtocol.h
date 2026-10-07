#pragma once

#include <Arduino.h>

struct BmsTelemetry {
    bool     connected = false;
    String   bms_type = "Unknown";
    String   device_name = "";
    String   mac_address = "";
    int      rssi = 0;

    float    total_voltage = 0.0f;
    float    current = 0.0f;
    float    power = 0.0f;
    float    charge_power = 0.0f;
    float    discharge_power = 0.0f;
    float    soc = 0.0f;
    float    capacity_remain = 0.0f;
    float    capacity_total = 0.0f;
    uint32_t cycle_count = 0;
    float    cycle_capacity = 0.0f;

    uint8_t  cell_count = 4;
    float    cell_voltages[32] = {0};
    uint8_t  min_cell_idx = 1;
    uint8_t  max_cell_idx = 1;
    float    min_cell_v = 0.0f;
    float    max_cell_v = 0.0f;
    float    delta_cell_v = 0.0f;

    float    temp_mos = 0.0f;
    float    temp_sensor1 = 0.0f;
    float    temp_sensor2 = 0.0f;

    bool     balancing_active = false;
    float    balancing_current = 0.0f;
    String   balancer_direction = "Очікування";

    bool     switch_charging = false;
    bool     switch_discharging = false;
    bool     switch_balancer = false;

    uint32_t raw_errors = 0;
    String   errors_str = "OK (Без помилок)";

    uint32_t last_update = 0;
};
