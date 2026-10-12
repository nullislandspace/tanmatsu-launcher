#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

// Hidden charging current setting. 0 mA means charging disabled: the device runs
// from USB power only and the charge controller keeps its 512 mA default.
#define CHARGER_CURRENT_DISABLED       0
#define CHARGER_CURRENT_DEFAULT_MA     512
#define CHARGER_CURRENT_RECOMMENDED_MA 1024

bool      charger_current_setting_available(void);
esp_err_t charger_current_get(uint16_t* out_current);
esp_err_t charger_current_set(uint16_t current);
esp_err_t charger_current_apply(void);

void menu_charger_current(void);
