#pragma once

#include <stdbool.h>
#include <stdint.h>

#define DORY_PACKET_VERSION 1
#define DORY_MAX_PAYLOAD_LEN 1400
#define DORY_MAX_DEPTH_SAMPLES 300

typedef enum {
    DORY_MSG_COMMAND = 1,
    DORY_MSG_MISSION = 2,
    DORY_MSG_RESULT = 3,
} com_msg_type_t;

typedef enum {
    DORY_CMD_SERVO_POWER = 1,
    DORY_CMD_LIGHT_SET = 2,
    DORY_CMD_BUZZER_TONE = 3,
    DORY_CMD_BUZZER_OFF = 4,
    DORY_CMD_BUZZER_LEAK_ALARM = 5,
} com_command_t;

typedef struct __attribute__((packed)) {
    uint8_t version;
    uint8_t type;
    uint16_t payload_len;
    uint8_t payload[DORY_MAX_PAYLOAD_LEN];
    uint16_t crc;
} com_packet_t;

typedef struct __attribute__((packed)) {
    uint8_t command;
    int32_t arg0;
    int32_t arg1;
} command_payload_t;

typedef struct __attribute__((packed)) {
    uint8_t step_count;
    int16_t depth_offset_mm;
} mission_payload_t;

typedef struct __attribute__((packed)) {
    uint16_t time_s;
    int16_t depth_mm;
} depth_sample_t;

typedef struct __attribute__((packed)) {
    uint16_t mission_time_s;
    int16_t final_depth_mm;
    int16_t min_depth_mm;
    int16_t max_depth_mm;
    uint16_t sample_count;
    depth_sample_t samples[DORY_MAX_DEPTH_SAMPLES];
} result_payload_t;

void init_com(void);
bool send_result(const result_payload_t *result);