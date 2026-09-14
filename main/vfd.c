// SPDX-License-Identifier: MIT

#include "vfd.h"
#include "sdkconfig.h"

#if defined(CONFIG_BSP_TARGET_TANMATSU)

#include <stdbool.h>
#include <stdint.h>
#include "bsp/catt.h"
#include "bsp/i2c.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static char const TAG[] = "vfd";

#define VFD_ADDRESS    0x13  // 7-bit address (default 0x10 + jumpers 1 and 2)
#define VFD_SPEED_HZ   100000
#define VFD_TIMEOUT_MS 50  // per-transfer timeout
#define VFD_PROBE_MS   20  // per-bus probe timeout

#define VFD_REG_CONTROL     0  // bit 0 = enable, bit 1 = test, bit 2 = LED
#define VFD_REG_SCROLL_MODE 3  // bits 3-0: 0 off / 1 left / 2 right; bit 4 loop

// Probe the VFD on one bus and switch it off. The internal bus is shared
// with the coprocessor, so it is claimed around every transfer there.
static bool vfd_power_off_on_bus(i2c_master_bus_handle_t bus, bool shared, char const* name) {
    if (shared) bsp_i2c_primary_bus_claim();
    esp_err_t res = i2c_master_probe(bus, VFD_ADDRESS, VFD_PROBE_MS);
    if (shared) bsp_i2c_primary_bus_release();
    if (res != ESP_OK) {
        return false;
    }

    i2c_device_config_t const dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = VFD_ADDRESS,
        .scl_speed_hz    = VFD_SPEED_HZ,
    };
    i2c_master_dev_handle_t dev = NULL;
    res                         = i2c_master_bus_add_device(bus, &dev_cfg, &dev);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add VFD device on %s: %s", name, esp_err_to_name(res));
        return true;  // Found, just not reachable through the driver
    }

    // Stop a scroll a previous app may have left running, then switch the tube off
    uint8_t const scroll_off[] = {VFD_REG_SCROLL_MODE, 0};
    uint8_t const disable[]    = {VFD_REG_CONTROL, 0};
    if (shared) bsp_i2c_primary_bus_claim();
    res = i2c_master_transmit(dev, scroll_off, sizeof(scroll_off), VFD_TIMEOUT_MS);
    if (res == ESP_OK) {
        res = i2c_master_transmit(dev, disable, sizeof(disable), VFD_TIMEOUT_MS);
    }
    if (shared) bsp_i2c_primary_bus_release();

    if (res == ESP_OK) {
        ESP_LOGI(TAG, "VFD at 0x%02X on %s switched off", VFD_ADDRESS, name);
    } else {
        ESP_LOGW(TAG, "Failed to switch off VFD at 0x%02X on %s: %s", VFD_ADDRESS, name, esp_err_to_name(res));
    }

    i2c_master_bus_rm_device(dev);
    return true;
}

void vfd_power_off(void) {
    i2c_master_bus_handle_t bus = NULL;

    // The BSP leaves the CATT bus down when an add-on holds the lines
    if (bsp_catt_i2c_bus_get_handle(&bus) == ESP_OK && bus != NULL) {
        if (vfd_power_off_on_bus(bus, false, "CATT I2C")) {
            return;
        }
    }

    bus = NULL;
    if (bsp_i2c_primary_bus_get_handle(&bus) == ESP_OK && bus != NULL) {
        if (vfd_power_off_on_bus(bus, true, "internal I2C")) {
            return;
        }
    }

    ESP_LOGI(TAG, "No VFD at 0x%02X", VFD_ADDRESS);
}

#else

void vfd_power_off(void) {
}

#endif
