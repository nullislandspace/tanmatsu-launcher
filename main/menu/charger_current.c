#include "charger_current.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bsp/input.h"
#include "bsp/power.h"
#include "common/display.h"
#include "common/theme.h"
#include "esp_err.h"
#include "gui_menu.h"
#include "gui_style.h"
#include "icons.h"
#include "menu/menu_helpers.h"
#include "menu/message_dialog.h"
#include "nvs_settings_helpers.h"
#include "nvs_settings_owner.h"
#include "pax_types.h"

#define CHARGER_CURRENT_NVS_KEY  "charger.current"
#define CHARGER_CURRENT_NICKNAME "cavac"

static const uint16_t charger_current_options[] = {0, 512, 1024, 1536, 2048};

bool charger_current_setting_available(void) {
#if defined(CONFIG_BSP_TARGET_TANMATSU)
    char nickname[64] = {0};
    nvs_settings_get_owner_nickname(nickname, sizeof(nickname), "");
    return strcmp(nickname, CHARGER_CURRENT_NICKNAME) == 0;
#else
    return false;
#endif
}

esp_err_t charger_current_get(uint16_t* out_current) {
    return nvs_settings_get_u16(CHARGER_CURRENT_NVS_KEY, CHARGER_CURRENT_DEFAULT_MA, out_current);
}

esp_err_t charger_current_set(uint16_t current) {
    bool disable = (current == CHARGER_CURRENT_DISABLED);
    return bsp_power_configure_charging(disable, disable ? CHARGER_CURRENT_DEFAULT_MA : current);
}

esp_err_t charger_current_apply(void) {
    uint16_t  current = CHARGER_CURRENT_DEFAULT_MA;
    esp_err_t res     = charger_current_get(&current);
    if (res != ESP_OK) {
        // Not configured, leave the charge controller at its default
        return res;
    }
    return charger_current_set(current);
}

static void option_label(char* out_label, size_t max_length, uint16_t current) {
    if (current == CHARGER_CURRENT_DISABLED) {
        snprintf(out_label, max_length, "%u mA (USB only)", CHARGER_CURRENT_DISABLED);
    } else {
        snprintf(out_label, max_length, "%u mA", current);
    }
}

static bool on_action(void* action_arg, void* user_ctx) {
    menu_t*  menu         = (menu_t*)user_ctx;
    uint16_t current      = (uint16_t)(uintptr_t)action_arg;
    char     message[256] = {0};

    if (current == CHARGER_CURRENT_DISABLED) {
        snprintf(message, sizeof(message),
                 "Disable charging and run the device from USB power only?\n\n"
                 "The charge controller keeps its %u mA default, but charging is turned off.",
                 CHARGER_CURRENT_DEFAULT_MA);
    } else {
        snprintf(message, sizeof(message), "Set the charging current to %u mA?", current);
        if (current > CHARGER_CURRENT_RECOMMENDED_MA) {
            strncat(message, "\n\nWarning: above 1024 mA is not recommended for the stock battery.",
                    sizeof(message) - strlen(message) - 1);
        }
    }

    if (adv_dialog_yes_no(get_icon(ICON_BATTERY_BOLT), "Charge controller", message) != MSG_DIALOG_RETURN_OK) {
        return false;
    }

    esp_err_t res = nvs_settings_set_u16(CHARGER_CURRENT_NVS_KEY, current);
    if (res != ESP_OK) {
        message_dialog(get_icon(ICON_ERROR), "Error", "Failed to store the charging current", "OK");
        return false;
    }

    res = charger_current_set(current);
    if (res != ESP_OK) {
        message_dialog(get_icon(ICON_ERROR), "Error", "Failed to configure the charge controller", "OK");
        return false;
    }

    for (size_t i = 0; i < menu_get_length(menu); i++) {
        uint16_t option = (uint16_t)(uintptr_t)menu_get_callback_args(menu, i);
        menu_set_value(menu, i, (option == current) ? "Active" : "");
    }

    message_dialog(get_icon(ICON_BATTERY_BOLT), "Charge controller", "Charging current configured", "OK");
    return false;
}

void menu_charger_current(void) {
    menu_t menu = {0};
    menu_initialize(&menu);

    bool     disabled = false;
    uint16_t active   = CHARGER_CURRENT_DEFAULT_MA;
    if (bsp_power_get_charging_configuration(&disabled, &active) != ESP_OK) {
        active = CHARGER_CURRENT_DISABLED;
    }
    if (disabled) {
        active = CHARGER_CURRENT_DISABLED;
    }

    for (size_t i = 0; i < sizeof(charger_current_options) / sizeof(charger_current_options[0]); i++) {
        char label[32] = {0};
        option_label(label, sizeof(label), charger_current_options[i]);
        menu_insert_item_value(&menu, label, (charger_current_options[i] == active) ? "Active" : "", NULL,
                               (void*)(uintptr_t)charger_current_options[i], -1);
    }

    menu_run_list(&menu, ((gui_element_icontext_t[]){{get_icon(ICON_BATTERY_BOLT), "Charging current"}}), 1,
                  MENU_FOOTER_BACK, ((gui_element_icontext_t[]){{NULL, "↑ / ↓ | ⏎ Select"}}), 1, on_action, &menu,
                  false);
}
