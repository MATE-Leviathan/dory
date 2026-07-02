# Dory Firmware Agent Handoff

## Project

This repo is the ESP-IDF firmware for Dory, a small underwater float/robot. The firmware lives in:

`/Users/evank/code/robotics/dory/firmware/dory`

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

`com.c` uses ESP-NOW with the Espressif callback -> queue -> task pattern. The receive callback only validates/copies incoming bytes and pushes them to a FreeRTOS queue. The `dory_com_task` parses and handles packets later.

## Current Protocol

The protocol was refactored to use ESP-NOW v2-sized packets.

Public protocol definitions are in `main/com.h`:

- `DORY_PACKET_VERSION`
- `DORY_MAX_PAYLOAD_LEN = 1400`
- `DORY_MAX_MISSION_STEPS = 64`
- `DORY_MAX_DEPTH_SAMPLES = 300`

Message types:

- `DORY_MSG_COMMAND`
- `DORY_MSG_MISSION`
- `DORY_MSG_RESULT`

Packet wrapper:

```c
typedef struct __attribute__((packed)) {
    uint8_t version;
    uint8_t type;
    uint16_t seq;
    uint16_t payload_len;
    uint8_t payload[DORY_MAX_PAYLOAD_LEN];
    uint16_t crc;
} dory_packet_t;
```

Current implementation sends/parses the whole fixed-size `dory_packet_t` wrapper. This is intentionally simple and fits under `ESP_NOW_MAX_DATA_LEN_V2` (`1470` bytes in the local ESP-IDF header).

Payloads:

- `dory_command_payload_t`
- `dory_mission_payload_t`
- `dory_result_payload_t`

Manual command payloads still support the old manual controls:

- servo power
- light set
- buzzer tone
- buzzer off
- leak alarm start/stop

Mission payload currently stores:

- `step_count`
- `depth_offset_mm`
- up to 64 steps of `target_depth_mm` + `hold_time_s`

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

Current Kconfig state uses six byte values:

- `CONFIG_DORY_ESPNOW_BASE_MAC_0`
- `CONFIG_DORY_ESPNOW_BASE_MAC_1`
- `CONFIG_DORY_ESPNOW_BASE_MAC_2`
- `CONFIG_DORY_ESPNOW_BASE_MAC_3`
- `CONFIG_DORY_ESPNOW_BASE_MAC_4`
- `CONFIG_DORY_ESPNOW_BASE_MAC_5`

The user asked whether there is a better way. The better plan is to replace these six fields with a single Kconfig `string`, for example:

```kconfig
config DORY_ESPNOW_BASE_MAC
    string "Base station MAC address"
    depends on DORY_WIRELESS
    default "00:00:00:00:00:00"
```

Then parse `CONFIG_DORY_ESPNOW_BASE_MAC` in C at startup. This has a nicer menuconfig UI but requires C-side validation of the `aa:bb:cc:dd:ee:ff` format.

This replacement has not been implemented yet as of this handoff.

## Kconfig

`main/Kconfig.projbuild` currently has:

- `DORY_DEBUG_SERIAL`
- `DORY_WIRELESS`
- base station MAC byte configs

`sdkconfig` currently contains placeholder MAC bytes of `0x00`. Real hardware will reject packets until the base station MAC is configured.

## Important Implementation Notes

- Do not call hardware functions from the ESP-NOW receive callback.
- Keep the callback fast: validate pointers, filter MAC, malloc/copy packet bytes, queue event, free on queue failure.
- The queued data is freed in `dory_com_task` after parsing/handling.
- `dory_com_get_latest_mission()` returns the most recently received mission.
- `dory_com_send_result()` sends one `DORY_MSG_RESULT` packet to the configured base station MAC.
- Result sending registers the configured base station as an ESP-NOW peer if needed.
- Wi-Fi channel is currently hardcoded to channel 1.
- `ninja` is not available in this environment; previous verification was static/dry-run only.

## User Preferences

- The user is learning C/ESP-NOW concepts and often asks for simple explanations.
- Keep explanations direct, small, and concrete.
- The user prefers KISS designs and explicitly asked to avoid feature bloat.
- Avoid adding large abstractions, multi-packet reliability, encryption, or complex mission management unless requested.
- If planning, keep the plan compact and decision-complete.

## Likely Next Steps

1. Replace the six MAC byte Kconfig options with one `DORY_ESPNOW_BASE_MAC` string.
2. Add a small parser in `com.c` that converts the string into `s_base_mac[6]`.
3. Consider rejecting startup or wireless init if the MAC string is malformed.
4. Later, implement the base station sender in `/Users/evank/code/websites/crush` so it can build `dory_packet_t` packets matching `main/com.h`.
5. Later, add actual mission runner and depth/I2C sensor logging.
