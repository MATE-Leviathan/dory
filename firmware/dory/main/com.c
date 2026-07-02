#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "esp_crc.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "board/buzzer.h"
#include "board/light.h"
#include "board/servo.h"

#define DORY_PACKET_VERSION 1
#define DORY_ESPNOW_QUEUE_SIZE 8
#define DORY_ESPNOW_MAXDELAY 512

static const char *TAG = "dory_com";

static QueueHandle_t s_espnow_queue = NULL;

typedef enum {
    DORY_CMD_SERVO_POWER = 1,
    DORY_CMD_LIGHT_SET = 2,
    DORY_CMD_BUZZER_TONE = 3,
    DORY_CMD_BUZZER_OFF = 4,
    DORY_CMD_BUZZER_LEAK_ALARM = 5,
} dory_command_t;

typedef struct __attribute__((packed)) {
    uint8_t version;
    uint8_t command;
    uint16_t seq;
    int32_t arg0;
    int32_t arg1;
    uint16_t crc;
} dory_packet_t;

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    uint8_t *data;
    int data_len;
} dory_espnow_event_t;

static void dory_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    dory_espnow_event_t evt;

    if (recv_info == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    memcpy(evt.mac_addr, recv_info->src_addr, ESP_NOW_ETH_ALEN);
    evt.data = malloc(len);
    if (evt.data == NULL) {
        ESP_LOGE(TAG, "Malloc receive data fail");
        return;
    }

    memcpy(evt.data, data, len);
    evt.data_len = len;

    if (xQueueSend(s_espnow_queue, &evt, DORY_ESPNOW_MAXDELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Send receive queue fail");
        free(evt.data);
    }
}

static bool dory_packet_parse(const uint8_t *data, int len, dory_packet_t *packet)
{
    uint16_t crc;
    uint16_t crc_cal;

    if (data == NULL || packet == NULL || len != sizeof(dory_packet_t)) {
        return false;
    }

    memcpy(packet, data, sizeof(dory_packet_t));

    crc = packet->crc;
    packet->crc = 0;
    crc_cal = esp_crc16_le(0, (const uint8_t *)packet, sizeof(dory_packet_t));

    if (crc_cal != crc) {
        return false;
    }

    return packet->version == DORY_PACKET_VERSION;
}

static void dory_packet_handle(const dory_packet_t *packet)
{
    switch ((dory_command_t)packet->command) {
    case DORY_CMD_SERVO_POWER:
        set_power(packet->arg0 != 0);
        break;
    case DORY_CMD_LIGHT_SET:
        light_set(packet->arg0 != 0);
        break;
    case DORY_CMD_BUZZER_TONE:
        if (packet->arg0 > 0) {
            buzzer_tone(packet->arg0);
        }
        break;
    case DORY_CMD_BUZZER_OFF:
        buzzer_off();
        break;
    case DORY_CMD_BUZZER_LEAK_ALARM:
        if (packet->arg0 != 0) {
            buzzer_start_leak_alarm();
        } else {
            buzzer_stop_leak_alarm();
        }
        break;
    default:
        ESP_LOGW(TAG, "Unknown command: %u", packet->command);
        break;
    }
}

static void dory_com_task(void *arg)
{
    dory_espnow_event_t evt;
    dory_packet_t packet;

    (void)arg;

    while (xQueueReceive(s_espnow_queue, &evt, portMAX_DELAY) == pdTRUE) {
        if (dory_packet_parse(evt.data, evt.data_len, &packet)) {
            dory_packet_handle(&packet);
        } else {
            ESP_LOGW(TAG, "Invalid packet from " MACSTR, MAC2STR(evt.mac_addr));
        }

        free(evt.data);
    }
}

void init_com(void) {
    // Init Wifi
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start());
    // Might add ability to change channel in menuconfig if needed
    ESP_ERROR_CHECK( esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));

    // Init ESPNOW
    s_espnow_queue = xQueueCreate(DORY_ESPNOW_QUEUE_SIZE, sizeof(dory_espnow_event_t));
    if (s_espnow_queue == NULL) {
        ESP_LOGE(TAG, "Create queue fail");
        return;
    }

    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(dory_espnow_recv_cb) );

    xTaskCreate(dory_com_task, "dory_com", 2048, NULL, 5, NULL);
}
