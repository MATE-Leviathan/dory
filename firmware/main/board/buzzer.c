#include "buzzer.h"

#include <stdbool.h>

#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_pins.h"

#define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))

static const char *TAG = "buzzer";

static const int samsung_boot[][2] = {
    {988, 180},
    {1319, 180},
    {1661, 200},
    {1976, 450},
};

static const int starting_beeps[][2] = {
    {4000, 350},
    {0, 150},
    {4000, 350},
    {0, 150},
    {4000, 350},
};

static const int leak_alarm[][2] = {
    {2800, 90},
    {0, 60},
    {2800, 90},
    {0, 60},
    {2800, 90},
    {0, 60},
    {2800, 90},
    {0, 60},
    {2800, 250},
    {0, 250},
};

static TaskHandle_t leak_alarm_task_handle = NULL;
static bool s_leak_alarm_on;

void buzzer_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 4000,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .gpio_num = BUZZER_PIN,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel));
    buzzer_off();
    ESP_LOGI(TAG, "Buzzer initialized");
}

void buzzer_off(void)
{
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

void buzzer_tone(int hz)
{
    ESP_LOGI(TAG, "Buzzer tone: %d Hz", hz);
    ESP_ERROR_CHECK(ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, hz));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}


static void buzzer_play_pattern(const int pattern[][2], int length)
{
    for (int i = 0; i < length; i++) {
        int hz = pattern[i][0];
        int duration_ms = pattern[i][1];

        if (hz > 0) {
            buzzer_tone(hz);
        } else {
            buzzer_off();
        }

        vTaskDelay(pdMS_TO_TICKS(duration_ms));
    }

    buzzer_off();
}

void buzzer_play_samsung_boot(void)
{
    ESP_LOGI(TAG, "Playing Samsung boot pattern");
    buzzer_play_pattern(samsung_boot, ARRAY_LEN(samsung_boot));
}

void buzzer_play_starting_beeps(void)
{
    ESP_LOGI(TAG, "Playing starting beeps");
    buzzer_play_pattern(starting_beeps, ARRAY_LEN(starting_beeps));
}

static void leak_alarm_task(void *arg)
{
    (void)arg;

    while (1) {
        buzzer_play_pattern(leak_alarm, ARRAY_LEN(leak_alarm));
    }
}

void buzzer_start_leak_alarm(void)
{
    if (leak_alarm_task_handle != NULL) {
        return;
    }

    xTaskCreate(leak_alarm_task, "leak_alarm", 2048, NULL, 5, &leak_alarm_task_handle);
    s_leak_alarm_on = true;
    ESP_LOGI(TAG, "Leak alarm started");
}

void buzzer_stop_leak_alarm(void)
{
    if (leak_alarm_task_handle == NULL) {
        return;
    }

    vTaskDelete(leak_alarm_task_handle);
    leak_alarm_task_handle = NULL;
    s_leak_alarm_on = false;

    buzzer_off();
    ESP_LOGI(TAG, "Leak alarm stopped");
}

bool buzzer_leak_alarm_is_on(void)
{
    return s_leak_alarm_on;
}
