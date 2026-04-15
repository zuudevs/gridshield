/**
 * @file prod_main.cpp
 * @brief GridShield Production Firmware — PZEM-004T + DHT11 + Relay + Tamper
 *
 * Production entry point for ESP32 DevKit V1 with actual hardware:
 *   - PZEM-004T v3.0 (energy meter — V, I, P, E, freq, PF via UART)
 *   - PZKHCT CT Clamp (current transformer for PZEM)
 *   - DHT11 (temperature & humidity)
 *   - JQC-3FF-S-Z Relay (AC load control)
 *   - Buzzer (GPIO25 — alert/alarm tone)
 *   - Tamper switch with pull-up (enclosure protection)
 *   - MCB IC60N (AC circuit breaker — external protection)
 *   - Terminal blocks (AC wiring)
 *
 * NO mock/demo/Arduino dependencies — 100% ESP-IDF native.
 *
 * @copyright Copyright (c) 2026 zuudevs
 */

#if !defined(GS_QEMU_BUILD)

#include <cstdio>
#include <cstdlib>
#include <cstring>

// GridShield Core
#include "core/system.hpp"
#include "hardware/sensor_manager.hpp"
#include "hardware/sensors/dht11.hpp"
#include "platform/esp32_platform.hpp"
#include "platform/platform.hpp"

// ESP-IDF
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_random.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/ledc.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "cJSON.h"

// ============================================================================
// CONFIGURATION — from menuconfig
// ============================================================================
#define WIFI_SSID           CONFIG_GRIDSHIELD_WIFI_SSID
#define WIFI_PASS           CONFIG_GRIDSHIELD_WIFI_PASS
#define BACKEND_HOST        CONFIG_GRIDSHIELD_BACKEND_HOST
#define BACKEND_PORT        CONFIG_GRIDSHIELD_BACKEND_PORT

#define METER_ID            0x00000000000003E9ULL   // 1001 decimal
#define READING_INTERVAL_S  5                       // send reading every 5s
#define VERSION_STRING      "3.3.1"

// ============================================================================
// PIN ASSIGNMENTS — ESP32 DevKit V1
// ============================================================================
//
//  Komponen              Interface    ESP32 Pin     GPIO
//  ─────────────────────────────────────────────────────
//  PZEM-004T + CT Clamp  UART2        TX=GPIO17     RX=GPIO16
//  DHT11                 Digital      GPIO13
//  JQC-3FF Relay         Digital OUT  GPIO26
//  Buzzer                PWM (LEDC)   GPIO25
//  Tamper Switch         Digital IN   GPIO4         (pull-up)
//  LED Indicator         Digital OUT  GPIO2         (built-in)
//
// ============================================================================

static constexpr uint8_t PIN_PZEM_TX         = 17;
static constexpr uint8_t PIN_PZEM_RX         = 16;
static constexpr uint8_t PIN_DHT11           = 13;
static constexpr uint8_t PIN_RELAY           = 26;
static constexpr uint8_t PIN_BUZZER          = 25;
static constexpr uint8_t PIN_TAMPER          = 4;
static constexpr uint8_t PIN_LED             = 2;

static const char* TAG = "GridShield";

using namespace gridshield;

// ============================================================================
// ESP32-NATIVE PLATFORM IMPLEMENTATIONS (no mock, no Arduino)
// ============================================================================

class Esp32Time : public platform::IPlatformTime
{
public:
    core::timestamp_t get_timestamp_ms() noexcept override
    {
        return static_cast<core::timestamp_t>(xTaskGetTickCount()) * portTICK_PERIOD_MS;
    }
    void delay_ms(uint32_t milliseconds) noexcept override
    {
        vTaskDelay(pdMS_TO_TICKS(milliseconds));
    }
};

class Esp32GPIO : public platform::IPlatformGPIO
{
public:
    core::Result<void> configure(uint8_t pin, PinMode mode) noexcept override
    {
        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask = (1ULL << pin);
        io_conf.intr_type = GPIO_INTR_DISABLE;
        switch (mode) {
            case PinMode::Input:
                io_conf.mode = GPIO_MODE_INPUT;
                break;
            case PinMode::Output:
                io_conf.mode = GPIO_MODE_OUTPUT;
                break;
            case PinMode::InputPullup:
                io_conf.mode = GPIO_MODE_INPUT;
                io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
                break;
            case PinMode::InputPulldown:
                io_conf.mode = GPIO_MODE_INPUT;
                io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
                break;
        }
        esp_err_t err = gpio_config(&io_conf);
        if (err != ESP_OK) return GS_MAKE_ERROR(core::ErrorCode::HardwareFailure);
        return core::Result<void>{};
    }
    core::Result<bool> read(uint8_t pin) noexcept override
    {
        return core::Result<bool>(gpio_get_level(static_cast<gpio_num_t>(pin)) != 0);
    }
    core::Result<void> write(uint8_t pin, bool value) noexcept override
    {
        gpio_set_level(static_cast<gpio_num_t>(pin), value ? 1 : 0);
        return core::Result<void>{};
    }
};

class Esp32Interrupt : public platform::IPlatformInterrupt
{
public:
    Esp32Interrupt() noexcept
    {
        memset(callbacks_, 0, sizeof(callbacks_));
        memset(contexts_, 0, sizeof(contexts_));
        memset(enabled_, 0, sizeof(enabled_));
    }
    core::Result<void> attach(uint8_t pin, TriggerMode mode,
                               InterruptCallback callback, void* context) noexcept override
    {
        if (!callback) return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        callbacks_[pin] = callback;
        contexts_[pin] = context;
        gpio_int_type_t isr_type = GPIO_INTR_ANYEDGE;
        switch (mode) {
            case TriggerMode::Rising:  isr_type = GPIO_INTR_POSEDGE; break;
            case TriggerMode::Falling: isr_type = GPIO_INTR_NEGEDGE; break;
            case TriggerMode::Change:  isr_type = GPIO_INTR_ANYEDGE; break;
            case TriggerMode::Low:     isr_type = GPIO_INTR_LOW_LEVEL; break;
            case TriggerMode::High:    isr_type = GPIO_INTR_HIGH_LEVEL; break;
        }
        gpio_set_intr_type(static_cast<gpio_num_t>(pin), isr_type);
        gpio_install_isr_service(0);
        gpio_isr_handler_add(static_cast<gpio_num_t>(pin), gpio_isr_bridge, (void*)(uintptr_t)pin);
        return core::Result<void>{};
    }
    core::Result<void> detach(uint8_t pin) noexcept override
    {
        gpio_isr_handler_remove(static_cast<gpio_num_t>(pin));
        callbacks_[pin] = nullptr;
        contexts_[pin] = nullptr;
        return core::Result<void>{};
    }
    core::Result<void> enable(uint8_t pin) noexcept override
    {
        enabled_[pin] = true;
        gpio_intr_enable(static_cast<gpio_num_t>(pin));
        return core::Result<void>{};
    }
    core::Result<void> disable(uint8_t pin) noexcept override
    {
        enabled_[pin] = false;
        gpio_intr_disable(static_cast<gpio_num_t>(pin));
        return core::Result<void>{};
    }
    static Esp32Interrupt* instance;
private:
    static void gpio_isr_bridge(void* arg)
    {
        uint8_t pin = static_cast<uint8_t>((uintptr_t)arg);
        if (instance && instance->enabled_[pin] && instance->callbacks_[pin]) {
            instance->callbacks_[pin](instance->contexts_[pin]);
        }
    }
    InterruptCallback callbacks_[40];
    void* contexts_[40];
    bool enabled_[40];
};
Esp32Interrupt* Esp32Interrupt::instance = nullptr;

class Esp32Comm : public platform::IPlatformComm
{
public:
    core::Result<void> init() noexcept override { connected_ = true; return core::Result<void>{}; }
    core::Result<void> shutdown() noexcept override { connected_ = false; return core::Result<void>{}; }
    core::Result<size_t> send(const uint8_t*, size_t length) noexcept override
    { return core::Result<size_t>(length); }
    core::Result<size_t> receive(uint8_t*, size_t, uint32_t) noexcept override
    { return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::NetworkTimeout)); }
    bool is_connected() noexcept override { return connected_; }
private:
    bool connected_{false};
};

/**
 * ESP-IDF UART driver for PZEM-004T Modbus communication.
 */
class Esp32UART : public platform::IPlatformUART
{
public:
    core::Result<void> init(uint8_t port, uint32_t baud_rate,
                             uint8_t tx_pin, uint8_t rx_pin) noexcept override
    {
        uart_config_t uart_config = {};
        uart_config.baud_rate = static_cast<int>(baud_rate);
        uart_config.data_bits = UART_DATA_8_BITS;
        uart_config.parity = UART_PARITY_DISABLE;
        uart_config.stop_bits = UART_STOP_BITS_1;
        uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
        uart_config.source_clk = UART_SCLK_DEFAULT;

        auto uart_num = static_cast<uart_port_t>(port);
        esp_err_t err;

        err = uart_driver_install(uart_num, 256, 256, 0, NULL, 0);
        if (err != ESP_OK) {
            ESP_LOGE("UART", "driver install failed: %s", esp_err_to_name(err));
            return GS_MAKE_ERROR(core::ErrorCode::UARTError);
        }

        err = uart_param_config(uart_num, &uart_config);
        if (err != ESP_OK) {
            ESP_LOGE("UART", "param config failed: %s", esp_err_to_name(err));
            return GS_MAKE_ERROR(core::ErrorCode::UARTError);
        }

        err = uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        if (err != ESP_OK) {
            ESP_LOGE("UART", "set pin failed: %s", esp_err_to_name(err));
            return GS_MAKE_ERROR(core::ErrorCode::UARTError);
        }

        ESP_LOGI("UART", "Port %d initialized (baud=%lu, TX=GP%d, RX=GP%d)",
                 port, (unsigned long)baud_rate, tx_pin, rx_pin);
        return core::Result<void>{};
    }

    core::Result<size_t> write(uint8_t port, const uint8_t* data, size_t length) noexcept override
    {
        auto uart_num = static_cast<uart_port_t>(port);
        int written = uart_write_bytes(uart_num, data, length);
        if (written < 0) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::UARTError));
        }
        return core::Result<size_t>(static_cast<size_t>(written));
    }

    core::Result<size_t> read(uint8_t port, uint8_t* buffer, size_t max_length,
                               uint32_t timeout_ms) noexcept override
    {
        auto uart_num = static_cast<uart_port_t>(port);
        int read_bytes = uart_read_bytes(uart_num, buffer, max_length,
                                          pdMS_TO_TICKS(timeout_ms));
        if (read_bytes < 0) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::UARTError));
        }
        if (read_bytes == 0) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::SensorTimeout));
        }
        return core::Result<size_t>(static_cast<size_t>(read_bytes));
    }

    core::Result<void> shutdown(uint8_t port) noexcept override
    {
        uart_driver_delete(static_cast<uart_port_t>(port));
        return core::Result<void>{};
    }
};

// ============================================================================
// WiFi
// ============================================================================
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
static int s_retry_num = 0;
#define MAX_RETRY 10

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG, "Retrying WiFi (%d/%d)...", s_retry_num, MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static bool wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t inst_any, inst_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &inst_any));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &inst_ip));

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s ...", WIFI_SSID);
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    return (bits & WIFI_CONNECTED_BIT) != 0;
}

// ============================================================================
// HTTP POST
// ============================================================================
static bool http_post_json(const char* path, const char* json_str)
{
    char url[256];
    snprintf(url, sizeof(url), "http://%s:%d%s", BACKEND_HOST, BACKEND_PORT, path);

    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 5000;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return false;

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_str, strlen(json_str));

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "POST %s failed: %s", path, esp_err_to_name(err));
        return false;
    }
    if (status >= 200 && status < 300) {
        ESP_LOGI(TAG, "POST %s -> %d OK", path, status);
        return true;
    }
    ESP_LOGW(TAG, "POST %s -> %d", path, status);
    return false;
}

// ============================================================================
// Relay Control
// ============================================================================
static bool relay_state = false;

static void relay_init(void)
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << PIN_RELAY);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);
    gpio_set_level(static_cast<gpio_num_t>(PIN_RELAY), 0); // OFF by default
    relay_state = false;
}

static void relay_set(bool on)
{
    gpio_set_level(static_cast<gpio_num_t>(PIN_RELAY), on ? 1 : 0);
    relay_state = on;
    ESP_LOGI(TAG, "Relay: %s", on ? "ON" : "OFF");
}

// ============================================================================
// Buzzer (LEDC PWM on GPIO25)
// ============================================================================
static void buzzer_init(void)
{
    ledc_timer_config_t timer_conf = {};
    timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_conf.duty_resolution = LEDC_TIMER_8_BIT;
    timer_conf.timer_num = LEDC_TIMER_0;
    timer_conf.freq_hz = 2000;
    timer_conf.clk_cfg = LEDC_AUTO_CLK;
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t ch_conf = {};
    ch_conf.gpio_num = PIN_BUZZER;
    ch_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    ch_conf.channel = LEDC_CHANNEL_0;
    ch_conf.timer_sel = LEDC_TIMER_0;
    ch_conf.duty = 0;
    ch_conf.hpoint = 0;
    ledc_channel_config(&ch_conf);
}

static void buzzer_tone(uint32_t freq_hz, uint32_t duration_ms)
{
    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq_hz);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 128); // 50% duty
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static void buzzer_beep(int count, uint32_t freq_hz, uint32_t on_ms, uint32_t off_ms)
{
    for (int i = 0; i < count; i++) {
        buzzer_tone(freq_hz, on_ms);
        if (i < count - 1) vTaskDelay(pdMS_TO_TICKS(off_ms));
    }
}

// Predefined alert patterns
static void buzzer_alert_tamper(void)   { buzzer_beep(3, 3000, 500, 200); } // 3x long high
static void buzzer_alert_temp(void)     { buzzer_beep(2, 2500, 300, 150); } // 2x medium
static void buzzer_alert_pzem(void)     { buzzer_beep(1, 1500, 150, 0);   } // 1x short low
static void buzzer_boot_ok(void)        { buzzer_beep(2, 2000, 80, 80);   } // 2x quick chirp

// ============================================================================
// LED Indicator
// ============================================================================
static void led_init(void)
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << PIN_LED);
    io_conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&io_conf);
}

static void led_blink(int count, int ms)
{
    for (int i = 0; i < count; i++) {
        gpio_set_level(static_cast<gpio_num_t>(PIN_LED), 1);
        vTaskDelay(pdMS_TO_TICKS(ms));
        gpio_set_level(static_cast<gpio_num_t>(PIN_LED), 0);
        vTaskDelay(pdMS_TO_TICKS(ms));
    }
}

// ============================================================================
// Platform Instances (static, no heap)
// ============================================================================
static Esp32Time                       esp32_time;
static Esp32GPIO                       esp32_gpio;
static Esp32Interrupt                  esp32_interrupt;
static Esp32Comm                       esp32_comm;
static Esp32UART                       esp32_uart;
static platform::esp32::Esp32Crypto    esp32_crypto;
static platform::esp32::Esp32Storage   esp32_storage;

static platform::PlatformServices      services;
static GridShieldSystem                gs_system;

// DHT11 sensor (standalone, not in SensorManager)
static hardware::sensors::DHT11Driver  dht11;

// ============================================================================
// System Configuration
// ============================================================================
static SystemConfig create_production_config()
{
    SystemConfig config;
    config.meter_id = METER_ID;
    config.heartbeat_interval_ms = 60000;
    config.reading_interval_ms = READING_INTERVAL_S * 1000;

    // ================================================================
    // HARDWARE — Komponen yang terpasang:
    //   1. PZEM-004T + PZKHCT CT Clamp (UART2)
    //   2. DHT11 (GPIO15) — dibaca langsung, bukan via SensorManager
    //   3. JQC-3FF-S-Z Relay (GPIO26)
    //   4. Tamper switch pull-up (GPIO4)
    //   5. MCB IC60N (external AC protection)
    //   6. Terminal blocks (AC wiring)
    // ================================================================

    // Tamper detection — switch on enclosure
    config.tamper_config.sensor_pin = PIN_TAMPER;
    config.tamper_config.debounce_ms = 50;

    // Baseline consumption profile for anomaly detection
    for (size_t i = 0; i < analytics::PROFILE_HISTORY_SIZE; ++i) {
        config.baseline_profile.hourly_avg_wh[i] = 1200;
    }
    config.baseline_profile.daily_avg_wh = 1200;
    config.baseline_profile.variance_threshold = 30;

    // --- PZEM-004T Energy meter (UART2) ---
    config.sensor_config.enable_pzem004t = true;
    config.sensor_config.pzem_config.uart_port = 2;
    config.sensor_config.pzem_config.tx_pin = PIN_PZEM_TX;
    config.sensor_config.pzem_config.rx_pin = PIN_PZEM_RX;

    // Sensors NOT installed:
    config.sensor_config.enable_acs712 = false;   // PZEM handles current
    config.sensor_config.enable_zmpt101b = false;  // PZEM handles voltage
    config.sensor_config.enable_ds18b20 = false;   // Using DHT11 instead
    config.sensor_config.enable_mpu6050 = false;   // Not available

    return config;
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, "  GridShield v%s - Production Mode", VERSION_STRING);
    ESP_LOGI(TAG, "  PZEM-004T + DHT11 + Relay + Buzzer + Tamper");
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, "Backend: http://%s:%d", BACKEND_HOST, BACKEND_PORT);
    ESP_LOGI(TAG, "Meter ID: 0x%llX", (unsigned long long)METER_ID);

    // ---- Phase 1: NVS Storage ----
    auto nvs_result = esp32_storage.init();
    if (nvs_result.is_error()) {
        ESP_LOGE(TAG, "NVS init failed");
        return;
    }
    ESP_LOGI(TAG, "[OK] NVS storage initialized");

    // ---- Phase 2: LED + Relay ----
    led_init();
    relay_init();
    buzzer_init();
    led_blink(3, 100); // 3 blinks = starting
    ESP_LOGI(TAG, "[OK] LED + Relay + Buzzer initialized (Relay=GPIO%d, Buzzer=GPIO%d)", PIN_RELAY, PIN_BUZZER);

    // ---- Phase 3: Watchdog ----
    auto wdt_result = platform::esp32::Esp32Watchdog::init(30);
    if (wdt_result.is_ok()) {
        ESP_LOGI(TAG, "[OK] Watchdog timer (30s)");
    }

    // ---- Phase 4: WiFi ----
    if (!wifi_init_sta()) {
        ESP_LOGE(TAG, "WiFi FAILED. Restarting in 10s...");
        led_blink(10, 200); // rapid blink = error
        vTaskDelay(pdMS_TO_TICKS(10000));
        esp_restart();
    }
    ESP_LOGI(TAG, "[OK] WiFi connected");
    led_blink(2, 300); // 2 slow blinks = connected
    buzzer_boot_ok();   // chirp = boot OK

    // ---- Phase 5: DHT11 ----
    hardware::sensors::DHT11Config dht_config;
    dht_config.pin = PIN_DHT11;
    auto dht_result = dht11.init(dht_config);
    if (dht_result.is_ok()) {
        ESP_LOGI(TAG, "[OK] DHT11 initialized (GPIO%d)", PIN_DHT11);
    } else {
        ESP_LOGW(TAG, "[--] DHT11 init failed - continuing without");
    }

    // ---- Phase 6: Platform Services ----
    Esp32Interrupt::instance = &esp32_interrupt;
    services.time      = &esp32_time;
    services.gpio      = &esp32_gpio;
    services.interrupt = &esp32_interrupt;
    services.crypto    = &esp32_crypto;
    services.storage   = &esp32_storage;
    services.comm      = &esp32_comm;
    services.uart      = &esp32_uart;

    // ---- Phase 7: GridShield System ----
    SystemConfig config = create_production_config();
    auto result = gs_system.initialize(config, services);
    if (result.is_error()) {
        ESP_LOGE(TAG, "System init failed (code=%d)", static_cast<int>(result.error().code));
        return;
    }
    ESP_LOGI(TAG, "[OK] GridShield system initialized");

    // ---- Phase 8: SensorManager (manual init — system.cpp doesn't do it) ----
    auto sm_result = gs_system.sensors().initialize(services, config.sensor_config);
    if (sm_result.is_ok()) {
        ESP_LOGI(TAG, "[OK] SensorManager initialized (PZEM=%s)",
                 config.sensor_config.enable_pzem004t ? "ON" : "OFF");
    } else {
        ESP_LOGW(TAG, "[--] SensorManager init failed (code=%d) — continuing",
                 static_cast<int>(sm_result.error().code));
    }

    result = gs_system.start();
    if (result.is_error()) {
        ESP_LOGE(TAG, "System start failed");
        return;
    }

    // Turn relay ON — power is flowing through MCB
    relay_set(true);

    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, "  PRODUCTION LOOP RUNNING");
    ESP_LOGI(TAG, "  PZEM-004T: GPIO%d(TX) GPIO%d(RX)", PIN_PZEM_TX, PIN_PZEM_RX);
    ESP_LOGI(TAG, "  DHT11:     GPIO%d", PIN_DHT11);
    ESP_LOGI(TAG, "  Relay:     GPIO%d (ON)", PIN_RELAY);
    ESP_LOGI(TAG, "  Tamper:    GPIO%d (ACTIVE)", PIN_TAMPER);
    ESP_LOGI(TAG, "  Interval:  %ds", READING_INTERVAL_S);
    ESP_LOGI(TAG, "==============================================");

    // ---- Main Loop ----
    int cycle = 0;
    bool prev_tamper = false;

    while (true) {
        cycle++;
        platform::esp32::Esp32Watchdog::feed();

        // === Step 1: Process security cycle ===
        gs_system.process_cycle();

        // === Step 2: Read PZEM-004T (via SensorManager) ===
        hardware::SensorData sensor_data{};
        bool pzem_ok = false;

        if (gs_system.sensors().is_initialized()) {
            auto sr = gs_system.sensors().read_all();
            if (sr.is_ok()) {
                sensor_data = sr.value();
                pzem_ok = sensor_data.pzem_available;
                if (!pzem_ok) {
                    ESP_LOGW(TAG, "[%d] PZEM: SensorManager OK but pzem_available=false", cycle);
                }
            } else {
                ESP_LOGW(TAG, "[%d] PZEM: read_all failed (code=%d)",
                         cycle, static_cast<int>(sr.error().code));
            }
        } else {
            ESP_LOGW(TAG, "[%d] PZEM: SensorManager NOT initialized! uart=%p",
                     cycle, (void*)services.uart);
        }

        if (!pzem_ok) {
            ESP_LOGW(TAG, "[%d] PZEM-004T read FAILED -- skipping", cycle);
            buzzer_alert_pzem();
        }

        // === Step 3: Read DHT11 (temperature + humidity) ===
        int16_t dht_temp_c10 = 0;
        uint16_t dht_humidity_x10 = 0;
        bool dht_ok = false;

        if (dht11.is_initialized()) {
            auto dht_reading = dht11.read();
            if (dht_reading.is_ok()) {
                auto& d = dht_reading.value();
                dht_temp_c10 = d.temperature_c10;
                dht_humidity_x10 = d.humidity_x10;
                sensor_data.temperature_c10 = dht_temp_c10;
                dht_ok = true;
            } else {
                ESP_LOGW(TAG, "[%d] DHT11 read FAILED (code=%d, line=%lu, file=%s)",
                         cycle,
                         static_cast<int>(dht_reading.error().code),
                         (unsigned long)dht_reading.error().line,
                         dht_reading.error().file ? dht_reading.error().file : "?");
            }
        } else {
            ESP_LOGW(TAG, "[%d] DHT11 NOT initialized!", cycle);
        }

        // === Step 4: Send reading to backend (only real data) ===
        if (pzem_ok) {
            cJSON* json = cJSON_CreateObject();
            cJSON_AddNumberToObject(json, "meter_id", (double)METER_ID);
            cJSON_AddNumberToObject(json, "energy_wh", sensor_data.energy_wh);
            cJSON_AddNumberToObject(json, "voltage_mv", sensor_data.voltage_mv);
            cJSON_AddNumberToObject(json, "current_ma", sensor_data.current_ma);
            cJSON_AddNumberToObject(json, "power_factor", sensor_data.power_factor_100);
            cJSON_AddNumberToObject(json, "power_mw", sensor_data.power_mw);
            cJSON_AddNumberToObject(json, "phase", 0);

            if (dht_ok) {
                cJSON_AddNumberToObject(json, "temperature_c", dht_temp_c10 / 10.0f);
                cJSON_AddNumberToObject(json, "humidity_pct", dht_humidity_x10 / 10.0f);
            }

            cJSON_AddBoolToObject(json, "relay_on", relay_state);

            char* str = cJSON_PrintUnformatted(json);

            ESP_LOGI(TAG, "---- Reading #%d ----", cycle);
            ESP_LOGI(TAG, " %.1fV | %.2fA | %.1fW | %luWh | PF %.2f",
                     sensor_data.voltage_mv / 1000.0f,
                     sensor_data.current_ma / 1000.0f,
                     sensor_data.power_mw / 1000.0f,
                     (unsigned long)sensor_data.energy_wh,
                     sensor_data.power_factor_100 / 100.0f);

            if (dht_ok) {
                ESP_LOGI(TAG, " %.1f degC | %.1f%% RH",
                         dht_temp_c10 / 10.0f, dht_humidity_x10 / 10.0f);
            }

            http_post_json("/api/meter-data", str);
            free(str);
            cJSON_Delete(json);
        }

        // === Step 5: Security pipeline ===
        core::MeterReading reading = hardware::SensorManager::to_meter_reading(
            sensor_data, services.time->get_timestamp_ms());
        reading.power_factor = static_cast<uint16_t>(sensor_data.power_factor_100);
        gs_system.send_meter_reading(reading);

        // === Step 6: Tamper detection ===
        bool tamper_now = (gs_system.get_state() == core::SystemState::Tampered);
        if (tamper_now && !prev_tamper) {
            ESP_LOGE(TAG, "TAMPER DETECTED!");
            // Emergency: cut relay + alarm
            relay_set(false);
            buzzer_alert_tamper();
            led_blink(5, 100);

            cJSON* alert = cJSON_CreateObject();
            cJSON_AddNumberToObject(alert, "meter_id", (double)METER_ID);
            cJSON_AddStringToObject(alert, "tamper_type", "PhysicalTamper");
            cJSON_AddNumberToObject(alert, "severity", 4);
            char* s = cJSON_PrintUnformatted(alert);
            http_post_json("/api/tamper-alert", s);
            free(s);
            cJSON_Delete(alert);
        }
        prev_tamper = tamper_now;

        // === Step 7: Temperature protection ===
        if (dht_ok && dht_temp_c10 > 600) { // > 60 degC
            ESP_LOGW(TAG, "HIGH TEMP %.1fC -- cutting relay!", dht_temp_c10 / 10.0f);
            relay_set(false);
            buzzer_alert_temp();

            cJSON* alert = cJSON_CreateObject();
            cJSON_AddNumberToObject(alert, "meter_id", (double)METER_ID);
            cJSON_AddStringToObject(alert, "tamper_type", "TemperatureAnomaly");
            cJSON_AddNumberToObject(alert, "severity", 3);
            cJSON_AddNumberToObject(alert, "temperature_c", dht_temp_c10 / 10.0f);
            char* s = cJSON_PrintUnformatted(alert);
            http_post_json("/api/tamper-alert", s);
            free(s);
            cJSON_Delete(alert);
        }

        // LED heartbeat
        gpio_set_level(static_cast<gpio_num_t>(PIN_LED), 1);
        vTaskDelay(pdMS_TO_TICKS(50));
        gpio_set_level(static_cast<gpio_num_t>(PIN_LED), 0);

        ESP_LOGD(TAG, "--- Cycle %d | State: %d | Mode: %d ---",
                 cycle,
                 static_cast<int>(gs_system.get_state()),
                 static_cast<int>(gs_system.get_mode()));

        vTaskDelay(pdMS_TO_TICKS(READING_INTERVAL_S * 1000));
    }
}

#endif // !GS_QEMU_BUILD
