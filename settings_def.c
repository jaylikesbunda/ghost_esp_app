// settings_def.c
#include "settings_def.h"
#include <stddef.h>  // Add this line

const SettingMetadata SETTING_METADATA[SETTINGS_COUNT] = {
    [SETTING_RGB_MODE] = {
        .name = "RGB Mode",
        .max_value = RGB_MODE_COUNT - 1,
        .value_names = SETTING_VALUE_NAMES_RGB_MODE,
        .uart_command = "setsetting -i 1 -v"
    },
    [SETTING_CHANNEL_HOP_DELAY] = {
        .name = "Channel Switch Delay",
        .max_value = CHANNEL_HOP_COUNT - 1,
        .value_names = SETTING_VALUE_NAMES_CHANNEL_HOP,
        .uart_command = "setsetting -i 2 -v"
    },
    [SETTING_ENABLE_CHANNEL_HOPPING] = {
        .name = "Enable Channel Hopping",
        .max_value = 1,
        .value_names = SETTING_VALUE_NAMES_BOOL,
        .uart_command = "setsetting -i 3 -v"
    },
    [SETTING_ENABLE_RANDOM_BLE_MAC] = {
        .name = "Enable Random BLE Mac",
        .max_value = 1,
        .value_names = SETTING_VALUE_NAMES_BOOL,
        .uart_command = "setsetting -i 4 -v"
    },
    [SETTING_STOP_ON_BACK] = {
        .name = "Stop On Back",
        .max_value = 1,
        .value_names = SETTING_VALUE_NAMES_BOOL,
        .uart_command = NULL  // This setting doesn't need a UART command
    },
    [SETTING_REBOOT_ESP] = {
        .name = "Reboot ESP",
        .max_value = 0,  // Only one option
        .value_names = SETTING_VALUE_NAMES_ACTION,
        .uart_command = "reboot"  // The command to send
    }
};

const SettingMetadata* settings_get_metadata(SettingKey key) {
    if(key >= SETTINGS_COUNT) {
        return NULL;
    }
    return &SETTING_METADATA[key];
}
