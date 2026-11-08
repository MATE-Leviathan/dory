#include "bar02_sensor.h"

#include "board_pins.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "ms5837.h"

#define WATER_DENSITY_KG_M3 1000.0f // Ideal Fresh water
#define GRAVITY_M_S2 9.80665f

static const char *TAG = "bar02_sensor";

static i2c_master_bus_handle_t s_bus;
static ms5837_t s_sensor;
static SemaphoreHandle_t s_read_mutex;
static float s_surface_pressure_pa;

esp_err_t bar02_sensor_init(void)
{
    if (s_sensor.device != NULL) {
        return ESP_OK;
    }

    s_read_mutex = xSemaphoreCreateMutex();
    if (s_read_mutex == NULL) {
        return ESP_ERR_NO_MEM;
    }

    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = SDA1,
        .scl_io_num = SCL1,
        .clk_source = I2C_CLK_SRC_DEFAULT,
    };
    esp_err_t err = i2c_new_master_bus(&bus_config, &s_bus);
    if (err != ESP_OK) {
        vSemaphoreDelete(s_read_mutex);
        s_read_mutex = NULL;
        return err;
    }

    const i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MS5837_I2C_ADDRESS,
        .scl_speed_hz = 100000,
    };
    i2c_master_dev_handle_t device = NULL;
    err = i2c_master_bus_add_device(s_bus, &device_config, &device);
    if (err != ESP_OK) {
        goto fail;
    }
    err = ms5837_init(&s_sensor, device);
    if (err != ESP_OK) {
        goto fail;
    }

    // Get reference of the surface
    ms5837_sample_t surface;
    err = ms5837_read(&s_sensor, &surface);
    if (err != ESP_OK) {
        goto fail;
    }
    s_surface_pressure_pa = surface.pressure_pa;
    ESP_LOGI(TAG, "Bar02 initialized; surface pressure %.1f Pa", s_surface_pressure_pa);
    return ESP_OK;

fail:
    s_sensor = (ms5837_t){0};
    if (device != NULL) {
        ESP_ERROR_CHECK(i2c_master_bus_rm_device(device));
    }
    ESP_ERROR_CHECK(i2c_del_master_bus(s_bus));
    s_bus = NULL;
    vSemaphoreDelete(s_read_mutex);
    s_read_mutex = NULL;
    return err;
}

esp_err_t bar02_sensor_read(bar02_sensor_sample_t *sample)
{
    if (sample == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_sensor.device == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    ms5837_sample_t measurement;
    // Serial debug and ESP-NOW must not interleave sensor conversions.
    xSemaphoreTake(s_read_mutex, portMAX_DELAY);
    esp_err_t err = ms5837_read(&s_sensor, &measurement);
    xSemaphoreGive(s_read_mutex);
    if (err != ESP_OK) {
        return err;
    }
    *sample = (bar02_sensor_sample_t){
        .temperature_c = measurement.temperature_c,
        .pressure_pa = measurement.pressure_pa,
        .depth_m = (measurement.pressure_pa - s_surface_pressure_pa) /
                   (WATER_DENSITY_KG_M3 * GRAVITY_M_S2),
    };
    return ESP_OK;
}
