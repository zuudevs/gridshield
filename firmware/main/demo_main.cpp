/**
 * @file demo_main.cpp
 * @brief GridShield Demo Firmware — WiFi + HTTP to Backend
 *
 * Runs on real ESP32 hardware WITHOUT sensors. Generates simulated meter
 * readings, tamper alerts, and anomaly events, then POSTs them to the
 * GridShield FastAPI backend over WiFi.
 *
 * Build:
 *   cd firmware
 *   idf.py set-target esp32
 *   idf.py menuconfig   (set WiFi creds in "GridShield Demo Config")
 *   idf.py build
 *   idf.py -p COM3 flash monitor
 *
 * @copyright Copyright (c) 2026 zuudevs
 */

#if !defined(GS_QEMU_BUILD)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

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
#include "lwip/err.h"
#include "lwip/sys.h"
#include "cJSON.h"

// ============================================================================
// CONFIGURATION — Edit these before flashing!
// ============================================================================
#define WIFI_SSID           CONFIG_GRIDSHIELD_WIFI_SSID
#define WIFI_PASS           CONFIG_GRIDSHIELD_WIFI_PASS
#define BACKEND_HOST        CONFIG_GRIDSHIELD_BACKEND_HOST
#define BACKEND_PORT        CONFIG_GRIDSHIELD_BACKEND_PORT

#define METER_ID            0x00000000000003E9ULL   // 1001 decimal
#define READING_INTERVAL_S  5                       // send reading every 5s
#define ALERT_CHANCE_PCT    8                       // 8% chance per cycle
#define ANOMALY_CHANCE_PCT  5                       // 5% chance per cycle

static const char* TAG = "GS-Demo";

// ============================================================================
// WiFi Event Handling
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
            ESP_LOGW(TAG, "Retrying WiFi connection (%d/%d)...", s_retry_num, MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "WiFi connection failed after %d retries", MAX_RETRY);
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

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

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

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected successfully");
        return true;
    }
    ESP_LOGE(TAG, "WiFi connection FAILED");
    return false;
}

// ============================================================================
// HTTP POST Helper
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
    if (!client) {
        ESP_LOGE(TAG, "HTTP client init failed");
        return false;
    }

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
        ESP_LOGI(TAG, "POST %s → %d OK", path, status);
        return true;
    }

    ESP_LOGW(TAG, "POST %s → %d", path, status);
    return false;
}

// ============================================================================
// Simulated Data Generators
// ============================================================================
static uint32_t random_range(uint32_t min_val, uint32_t max_val)
{
    return min_val + (esp_random() % (max_val - min_val + 1));
}

static void send_meter_reading(uint64_t meter_id, int cycle)
{
    // Simulate daily pattern based on cycle count
    int sim_hour = (cycle * READING_INTERVAL_S / 3600) % 24;
    float factor;
    if (sim_hour >= 6 && sim_hour <= 9)       factor = 1.35f;   // morning peak
    else if (sim_hour >= 10 && sim_hour <= 17) factor = 0.95f;   // daytime
    else if (sim_hour >= 18 && sim_hour <= 22) factor = 1.55f;   // evening peak
    else                                       factor = 0.45f;   // night

    // Add randomness
    float noise = ((float)(esp_random() % 200) - 100.0f) / 100.0f; // -1.0 to +1.0
    factor += noise * 0.15f;

    int base_energy = 1200;
    int energy_wh  = (int)(base_energy * factor) + (int)(esp_random() % 100) - 50;
    if (energy_wh < 0) energy_wh = 0;

    int voltage_mv = 220000 + (int)(esp_random() % 10000) - 5000;  // 215V-225V
    int current_ma = (energy_wh * 1000) / 220 + (int)(esp_random() % 200) - 100;
    if (current_ma < 0) current_ma = 0;

    int power_factor = 850 + (int)(esp_random() % 150);            // 850-1000
    int phase = esp_random() % 2;

    cJSON* json = cJSON_CreateObject();
    // cJSON doesn't support uint64, so use double for meter_id
    cJSON_AddNumberToObject(json, "meter_id", (double)meter_id);
    cJSON_AddNumberToObject(json, "energy_wh", energy_wh);
    cJSON_AddNumberToObject(json, "voltage_mv", voltage_mv);
    cJSON_AddNumberToObject(json, "current_ma", current_ma);
    cJSON_AddNumberToObject(json, "power_factor", power_factor);
    cJSON_AddNumberToObject(json, "phase", phase);

    char* str = cJSON_PrintUnformatted(json);
    ESP_LOGI(TAG, "[READING] energy=%d Wh, voltage=%.1f V, current=%d mA",
             energy_wh, voltage_mv / 1000.0f, current_ma);
    http_post_json("/api/meter-data", str);
    free(str);
    cJSON_Delete(json);
}

static void send_tamper_alert(uint64_t meter_id)
{
    static const char* tamper_types[] = {
        "CasingOpened", "MagneticInterference", "PowerCutAttempt",
        "PhysicalShock", "VibrationDetected", "TemperatureAnomaly"
    };
    int idx = esp_random() % 6;
    int severity = (esp_random() % 5);  // 0-4

    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "meter_id", (double)meter_id);
    cJSON_AddStringToObject(json, "tamper_type", tamper_types[idx]);
    cJSON_AddNumberToObject(json, "severity", severity);

    char* str = cJSON_PrintUnformatted(json);
    ESP_LOGW(TAG, "[ALERT] 🚨 %s (severity=%d)", tamper_types[idx], severity);
    http_post_json("/api/tamper-alert", str);
    free(str);
    cJSON_Delete(json);
}

static void send_anomaly(uint64_t meter_id)
{
    static const char* anomaly_types[] = {
        "UnexpectedDrop", "UnexpectedSpike", "ZeroConsumption", "PatternDeviation"
    };
    static const char* severity_levels[] = { "Low", "Medium", "High", "Critical" };

    int type_idx = esp_random() % 4;
    int sev_idx  = esp_random() % 4;

    float expected = 800.0f + (float)(esp_random() % 1200);
    float deviation_pct = 15.0f + (float)(esp_random() % 80);
    int direction = (esp_random() % 2) ? 1 : -1;
    float current_val = expected * (1.0f + direction * deviation_pct / 100.0f);
    int confidence = 40 + (int)(esp_random() % 60);

    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "meter_id", (double)meter_id);
    cJSON_AddStringToObject(json, "anomaly_type", anomaly_types[type_idx]);
    cJSON_AddStringToObject(json, "severity", severity_levels[sev_idx]);
    cJSON_AddNumberToObject(json, "current_value", current_val);
    cJSON_AddNumberToObject(json, "expected_value", expected);
    cJSON_AddNumberToObject(json, "deviation_percent", deviation_pct);
    cJSON_AddNumberToObject(json, "confidence", confidence);

    char* str = cJSON_PrintUnformatted(json);
    ESP_LOGW(TAG, "[ANOMALY] ⚠️  %s deviation=%.1f%% confidence=%d%%",
             anomaly_types[type_idx], deviation_pct, confidence);
    http_post_json("/api/anomalies", str);
    free(str);
    cJSON_Delete(json);
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, "  GridShield v3.0.0 — Demo Mode");
    ESP_LOGI(TAG, "  Real ESP32 → WiFi → Backend");
    ESP_LOGI(TAG, "  No sensors — simulated readings");
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, "Backend: http://%s:%d", BACKEND_HOST, BACKEND_PORT);
    ESP_LOGI(TAG, "Meter ID: 0x%llX", (unsigned long long)METER_ID);

    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Connect to WiFi
    if (!wifi_init_sta()) {
        ESP_LOGE(TAG, "Cannot start demo without WiFi. Restarting in 10s...");
        vTaskDelay(pdMS_TO_TICKS(10000));
        esp_restart();
    }

    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, "  Demo started! Sending data every %ds", READING_INTERVAL_S);
    ESP_LOGI(TAG, "==============================================");

    // Main loop — send data to backend
    int cycle = 0;
    while (true) {
        cycle++;

        // Always send a meter reading
        send_meter_reading(METER_ID, cycle);

        // Occasionally send a tamper alert (~8% chance)
        if ((esp_random() % 100) < ALERT_CHANCE_PCT) {
            send_tamper_alert(METER_ID);
        }

        // Occasionally send an anomaly (~5% chance)
        if ((esp_random() % 100) < ANOMALY_CHANCE_PCT) {
            send_anomaly(METER_ID);
        }

        ESP_LOGI(TAG, "--- Cycle %d complete, sleeping %ds ---", cycle, READING_INTERVAL_S);
        vTaskDelay(pdMS_TO_TICKS(READING_INTERVAL_S * 1000));
    }
}

#endif // !GS_QEMU_BUILD
