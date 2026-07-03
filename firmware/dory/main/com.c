#include <assert.h>
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
#include "com.h"

#define ESPNOW_QUEUE_SIZE 8

static const char *TAG = "com";

static QueueHandle_t s_espnow_queue = NULL;

static uint8_t s_base_mac[ESP_NOW_ETH_ALEN];

typedef struct {
    uint8_t *data;
    int data_len;
} espnow_event_t;

typedef struct {
    packet_header_t header;
    uint8_t payload[DORY_MAX_PAYLOAD_LEN];
} packet_t;

static uint8_t from_hex_char(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    assert(false);
    return 0;
}

static void parse_base_mac(void)
{
    const char *mac_str = CONFIG_DORY_ESPNOW_BASE_MAC;

    assert(strlen(mac_str) == 17);

    for (size_t i = 0; i < ESP_NOW_ETH_ALEN; i++) {
        s_base_mac[i] = (from_hex_char(mac_str[i * 3]) << 4) | from_hex_char(mac_str[i * 3 + 1]);
    }

    ESP_LOGI(TAG, "Base station MAC: " MACSTR, MAC2STR(s_base_mac));
}

static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    espnow_event_t evt;

    if (recv_info == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    if (memcmp(recv_info->src_addr, s_base_mac, ESP_NOW_ETH_ALEN) != 0) {
        ESP_LOGW(TAG, "Ignoring packet from unknown device " MACSTR, MAC2STR(recv_info->src_addr));
        return;
    }

    evt.data = malloc(len);
    if (evt.data == NULL) {
        ESP_LOGE(TAG, "Malloc receive data fail");
        return;
    }

    memcpy(evt.data, data, len);
    evt.data_len = len;

    if (xQueueSend(s_espnow_queue, &evt, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Send receive queue fail");
        free(evt.data);
    }
}

static bool parse_packet(const uint8_t *data, int len, packet_t *packet)
{
    uint16_t payload_len;
    uint16_t crc;
    uint16_t crc_cal;
    size_t crc_offset;
    size_t packet_len;

    if (data == NULL || packet == NULL || len < sizeof(packet_header_t) + sizeof(uint16_t)) {
        return false;
    }

    memcpy(&packet->header, data, sizeof(packet->header));
    payload_len = packet->header.payload_len;

    if (packet->header.version != DORY_PACKET_VERSION ||
        (packet->header.type != DORY_MSG_COMMAND &&
         packet->header.type != DORY_MSG_MISSION) ||
        payload_len > DORY_MAX_PAYLOAD_LEN) {
        return false;
    }

    packet_len = sizeof(packet_header_t) + payload_len + sizeof(uint16_t);
    if ((size_t)len != packet_len) {
        return false;
    }

    memcpy(packet->payload, data + sizeof(packet_header_t), payload_len);

    crc_offset = sizeof(packet_header_t) + payload_len;
    memcpy(&crc, data + crc_offset, sizeof(crc));

    crc_cal = esp_crc16_le(0, data, crc_offset);

    if (crc_cal != crc) {
        return false;
    }

    return true;
}

static void handle_command(const command_payload_t *command)
{
    switch ((com_command_t)command->command) {
    case DORY_CMD_SERVO_POWER:
        set_power(command->arg0 != 0);
        break;
    case DORY_CMD_LIGHT_SET:
        light_set(command->arg0 != 0);
        break;
    case DORY_CMD_BUZZER_TONE:
        if (command->arg0 > 0) {
            buzzer_tone(command->arg0);
        }
        break;
    case DORY_CMD_BUZZER_OFF:
        buzzer_off();
        break;
    case DORY_CMD_BUZZER_LEAK_ALARM:
        if (command->arg0 != 0) {
            buzzer_start_leak_alarm();
        } else {
            buzzer_stop_leak_alarm();
        }
        break;
    default:
        ESP_LOGW(TAG, "Unknown command: %u", command->command);
        break;
    }
}

static void handle_mission(const mission_payload_t *mission)
{
    ESP_LOGI(TAG, "Received mission: steps=%u depth_offset_mm=%d", mission->step_count, mission->depth_offset_mm);
}

static void handle_packet(const packet_t *packet)
{
    switch (packet->header.type) {
    case DORY_MSG_COMMAND:
    {
        command_payload_t command;
        if (packet->header.payload_len != sizeof(command)) {
            ESP_LOGW(TAG, "Invalid command payload len: %u", packet->header.payload_len);
            return;
        }
        memcpy(&command, packet->payload, sizeof(command));
        ESP_LOGI(TAG, "Received command: command=%u arg0=%ld arg1=%ld",
                 command.command, (long)command.arg0, (long)command.arg1);
        handle_command(&command);
        break;
    }
    case DORY_MSG_MISSION:
    {
        mission_payload_t mission;
        if (packet->header.payload_len != sizeof(mission)) {
            ESP_LOGW(TAG, "Invalid mission payload len: %u", packet->header.payload_len);
            return;
        }
        memcpy(&mission, packet->payload, sizeof(mission));
        handle_mission(&mission);
        break;
    }
    default:
        ESP_LOGW(TAG, "Unknown message type: %u", packet->header.type);
        break;
    }
}

static void com_task(void *arg)
{
    espnow_event_t evt;
    packet_t packet;

    (void)arg;

    while (xQueueReceive(s_espnow_queue, &evt, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "Received ESP-NOW packet: len=%d", evt.data_len);

        if (parse_packet(evt.data, evt.data_len, &packet)) {
            handle_packet(&packet);
        } else {
            ESP_LOGW(TAG, "Invalid packet from base station");
        }

        free(evt.data);
    }
}

void init_com(void) {
    uint8_t mac[ESP_NOW_ETH_ALEN];

    parse_base_mac();

    // Init Wifi
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start());
    ESP_ERROR_CHECK( esp_wifi_get_mac(WIFI_IF_STA, mac));
    ESP_LOGI(TAG, "Dory STA MAC: " MACSTR, MAC2STR(mac));
    // Might add ability to change channel in menuconfig if needed
    ESP_ERROR_CHECK( esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));

    // Init ESPNOW
    s_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(espnow_event_t));
    if (s_espnow_queue == NULL) {
        ESP_LOGE(TAG, "Create queue fail");
        return;
    }

    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(espnow_recv_cb) );

    xTaskCreate(com_task, "com", 4096, NULL, 5, NULL);
}

static bool ensure_base_peer(void)
{
    if (esp_now_is_peer_exist(s_base_mac)) {
        return true;
    }

    esp_now_peer_info_t peer = {0};

    memcpy(peer.peer_addr, s_base_mac, ESP_NOW_ETH_ALEN);
    peer.channel = 1;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;

    return esp_now_add_peer(&peer) == ESP_OK;
}

bool send_result(const result_payload_t *result)
{
    uint8_t data[sizeof(packet_header_t) + sizeof(*result) + sizeof(uint16_t)];
    packet_header_t header = {
        .version = DORY_PACKET_VERSION,
        .type = DORY_MSG_RESULT,
        .payload_len = sizeof(*result),
    };
    uint16_t crc;

    if (result == NULL || result->sample_count > DORY_MAX_DEPTH_SAMPLES || !ensure_base_peer()) {
        return false;
    }

    memcpy(data, &header, sizeof(header));
    memcpy(data + sizeof(header), result, sizeof(*result));
    crc = esp_crc16_le(0, data, sizeof(header) + sizeof(*result));
    memcpy(data + sizeof(header) + sizeof(*result), &crc, sizeof(crc));

    return esp_now_send(s_base_mac, data, sizeof(data)) == ESP_OK;
}
