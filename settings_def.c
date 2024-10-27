// settings_def.c
#include "settings_def.h"
#include <stddef.h>

// Define the constant arrays
const char* const SETTING_VALUE_NAMES_RGB_MODE[] = {"Stealth", "Normal", "Rainbow"};
const char* const SETTING_VALUE_NAMES_CHANNEL_HOP[] = {"500ms", "1000ms", "2000ms", "3000ms", "4000ms"};
const char* const SETTING_VALUE_NAMES_BOOL[] = {"False", "True"};
const char* const SETTING_VALUE_NAMES_ACTION[] = {"Press OK"};

const SettingMetadata SETTING_METADATA[SETTINGS_COUNT] = {
    [SETTING_RGB_MODE] = {
        .name = "RGB Mode",
        .max_value = RGB_MODE_COUNT - 1,
        .value_names = SETTING_VALUE_NAMES_RGB_MODE,
        .uart_command = "setsetting -i 1 -v"  // Command formatting handled in UI
    },
    [SETTING_CHANNEL_HOP_DELAY] = {
        .name = "Channel Switch Delay",
        .max_value = CHANNEL_HOP_COUNT - 1,
        .value_names = SETTING_VALUE_NAMES_CHANNEL_HOP,
        .uart_command = "setsetting -i 2 -v"  // Command formatting handled in UI
    },
    [SETTING_ENABLE_CHANNEL_HOPPING] = {
        .name = "Enable Channel Hopping",
        .max_value = 1,
        .value_names = SETTING_VALUE_NAMES_BOOL,
        .uart_command = "setsetting -i 3 -v"  // Command formatting handled in UI
    },
    [SETTING_ENABLE_RANDOM_BLE_MAC] = {
        .name = "Enable Random BLE Mac",
        .max_value = 1,
        .value_names = SETTING_VALUE_NAMES_BOOL,
        .uart_command = "setsetting -i 4 -v"  // Command formatting handled in UI
    },
    [SETTING_STOP_ON_BACK] = {
        .name = "Stop On Back",
        .max_value = 1,
        .value_names = SETTING_VALUE_NAMES_BOOL,
        .uart_command = NULL  // This setting doesn't need a UART command
    },
    [SETTING_REBOOT_ESP] = {
        .name = "Reboot ESP",
        .max_value = 0,
        .value_names = SETTING_VALUE_NAMES_ACTION,
        .uart_command = "handle_reboot"  // Command formatting handled in UI
    },
    [SETTING_CLEAR_LOGS] = {
        .name = "Clear Log Files",
        .max_value = 0,
        .value_names = SETTING_VALUE_NAMES_ACTION,
        .uart_command = NULL  // No UART command needed
    },
    [SETTING_CLEAR_NVS] = {
        .name = "Clear NVS",
        .max_value = 0,
        .value_names = SETTING_VALUE_NAMES_ACTION,
        .uart_command = "handle_clearnvs"  // Command formatting handled in UI
    }
};

const SettingMetadata* settings_get_metadata(SettingKey key) {
    if(key >= SETTINGS_COUNT) {
        return NULL;
    }
    return &SETTING_METADATA[key];
}