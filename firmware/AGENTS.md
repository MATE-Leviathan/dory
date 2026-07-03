# Dory Firmware Agent Handoff

## Project

This repo is the ESP-IDF firmware for Dory, a small underwater float/robot. The firmware lives in:

`/Users/evank/code/robotics/dory/firmware`

The related base station / web UI project is:

`/Users/evank/code/websites/crush`

`crush` currently runs an ESP32 AP and serves a Svelte UI. Its settings schema includes mission profiles (`target`, `hold_time`), depth offset, servo calibration, and connection settings.

## User Goals

The user is building a simple control and mission protocol between a base station and the Dory float.

Important physical constraint: RF does not work underwater. Communication should happen while the float is above water:

- Before the dive: base station sends a complete mission packet to the float.
- During the dive: the float runs autonomously and logs depth locally.
- After resurfacing: the float sends depth result data back to the base station.

Keep the system KISS. Avoid feature bloat, multi-packet protocols, retries, encryption, full logs, or mission execution details until the user explicitly asks for them.

## Current Firmware Shape

Main entrypoint:

- `main/main.c`
- Initializes board modules.
- If `CONFIG_DORY_WIRELESS` is enabled, initializes NVS and calls `init_com()`.

Board APIs currently used by communication:

- `set_power(bool on)` from `board/servo.h`
- `light_set(bool on)` from `board/light.h`
- `buzzer_tone(int hz)` from `board/buzzer.h`
- `buzzer_off(void)`
- `buzzer_start_leak_alarm(void)`
- `buzzer_stop_leak_alarm(void)`

Communication files:

- `main/com.h`
- `main/com.c`

`com.c` uses ESP-NOW with the Espressif callback -> queue -> task pattern. The receive callback only validates/copies incoming bytes and pushes them to a FreeRTOS queue. The `com_task` parses and handles packets later.

`main/CMakeLists.txt` only builds optional source files when enabled:

- `debug_serial.c` is appended only when `CONFIG_DORY_DEBUG_SERIAL` is enabled.
- `com.c` is appended only when `CONFIG_DORY_WIRELESS` is enabled.

## Current Protocol

The protocol uses dynamically sized ESP-NOW packets. It does not send the whole 1400-byte payload buffer for small commands.

Public protocol definitions are in `main/com.h`:

- `DORY_PACKET_VERSION`
- `DORY_MAX_PAYLOAD_LEN = 1400`
- `DORY_MAX_DEPTH_SAMPLES = 300`

Message types:

- `DORY_MSG_COMMAND`
- `DORY_MSG_MISSION`
- `DORY_MSG_RESULT`

Wire format:

```
packet_header_t header
payload_len bytes of payload
uint16_t crc
```

`packet_header_t` is:

```c
typedef struct __attribute__((packed)) {
    uint8_t version;
    uint8_t type;
    uint16_t payload_len;
} packet_header_t;
```

CRC is calculated over `header + payload` only. The CRC is appended after the payload and is not included in its own calculation.

`com.c` uses a private parsed packet type with a max-size payload buffer:

```c
typedef struct {
    packet_header_t header;
    uint8_t payload[DORY_MAX_PAYLOAD_LEN];
} packet_t;
```

This private `packet_t` is not the wire format. It is only Dory's parsed receive buffer.

Payloads:

- `command_payload_t`
- `mission_payload_t`
- `result_payload_t`

Manual command payloads still support the old manual controls:

- servo power
- light set
- buzzer tone
- buzzer off
- leak alarm start/stop

Mission payload currently stores:

- `step_count`
- `depth_offset_mm`

Result payload currently supports:

- mission time
- final/min/max depth
- up to 300 downsampled depth samples

Exact depth sensor/I2C implementation and exact mission runner are not implemented yet and should not be invented unless asked.

## Base Station MAC Restriction

The user wants Dory to only accept ESP-NOW packets from one configured base station.

Current code in `com.c` rejects packets unless:

```c
memcmp(recv_info->src_addr, s_base_mac, ESP_NOW_ETH_ALEN) == 0
```

Current Kconfig state uses one string:

```kconfig
config DORY_ESPNOW_BASE_MAC
    string "Base station MAC address"
    depends on DORY_WIRELESS
    default "00:00:00:00:00:00"
```

`parse_base_mac()` converts `CONFIG_DORY_ESPNOW_BASE_MAC` into `s_base_mac[6]` at startup. The format is `aa:bb:cc:dd:ee:ff`.

Dory logs its own STA MAC after Wi-Fi starts:

```c
ESP_LOGI(TAG, "Dory STA MAC: " MACSTR, MAC2STR(mac));
```

Use that MAC as the Arduino/base-station target MAC.

## Kconfig

`main/Kconfig.projbuild` currently has:

- `DORY_DEBUG_SERIAL`
- `DORY_WIRELESS`
- `DORY_ESPNOW_BASE_MAC`

Real hardware will reject packets until the base station MAC is configured to the sender's STA MAC.

## Important Implementation Notes

- Do not call hardware functions from the ESP-NOW receive callback.
- Keep the callback fast: validate pointers, filter MAC, malloc/copy packet bytes, queue event, free on queue failure.
- The queued data is freed in `com_task` after parsing/handling.
- `send_result()` sends one outbound `DORY_MSG_RESULT` packet to the configured base station MAC.
- Result sending registers the configured base station as an ESP-NOW peer if needed.
- Incoming packets currently accept command and mission messages only. Result messages are outbound from Dory.
- Wi-Fi channel is currently hardcoded to channel 1.
- `ninja` is not available in this environment; previous verification was static/dry-run only.

## User Preferences

- The user is learning C/ESP-NOW concepts and often asks for simple explanations.
- Keep explanations direct, small, and concrete.
- The user prefers KISS designs and explicitly asked to avoid feature bloat.
- Avoid adding large abstractions, multi-packet reliability, encryption, or complex mission management unless requested.
- If planning, keep the plan compact and decision-complete.

## Likely Next Steps

1. Update any Arduino/base-station sender to build dynamic packets: `packet_header_t + payload + crc`.
2. Later, implement the base station sender in `/Users/evank/code/websites/crush` so it matches `main/com.h`.
3. Later, add actual mission runner and depth/I2C sensor logging.
