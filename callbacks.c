// callbacks.c
#include "callbacks.h"
#include "uart_utils.h"
#include "settings_storage.h"
#include "settings_def.h"
#include <furi.h>
#include <stdio.h>   // For snprintf
#include <stdlib.h>  // For malloc
#include <string.h>  // For memset

void update_settings_and_write(AppState* app, Settings* settings) {
    (void)app;

    settings_storage_save(settings, GHOST_ESP_APP_SETTINGS_FILE);
}


// Event handlers implementation

void on_rgb_mode_changed(VariableItem* item) {
    uint8_t index = variable_item_get_current_value_index(item);
    AppState* app = variable_item_get_context(item);
    Settings settings;

    if(settings_storage_load(&settings, GHOST_ESP_APP_SETTINGS_FILE) != SETTINGS_OK) {
        // Initialize default settings if loading fails
        memset(&settings, 0, sizeof(Settings));
    }

    settings.rgb_mode_index = index;
    const SettingMetadata* metadata = settings_get_metadata(SETTING_RGB_MODE);
    variable_item_set_current_value_text(item, metadata->value_names[index]);

    char command[64];
    snprintf(command, sizeof(command), "%s %d\n", metadata->uart_command, index + 1);
    send_uart_command(command, app);

    update_settings_and_write(app, &settings);
}

void on_channelswitchdelay_changed(VariableItem* item) {
    uint8_t index = variable_item_get_current_value_index(item);
    AppState* app = variable_item_get_context(item);
    Settings settings;

    if(settings_storage_load(&settings, GHOST_ESP_APP_SETTINGS_FILE) != SETTINGS_OK) {
        memset(&settings, 0, sizeof(Settings));
    }

    settings.channel_hop_delay_index = index;
    const SettingMetadata* metadata = settings_get_metadata(SETTING_CHANNEL_HOP_DELAY);
    variable_item_set_current_value_text(item, metadata->value_names[index]);

    char command[64];
    snprintf(command, sizeof(command), "%s %d\n", metadata->uart_command, index + 1);
    send_uart_command(command, app);

    update_settings_and_write(app, &settings);
}

void on_togglechannelhopping_changed(VariableItem* item) {
    uint8_t index = variable_item_get_current_value_index(item);
    AppState* app = variable_item_get_context(item);
    Settings settings;

    if(settings_storage_load(&settings, GHOST_ESP_APP_SETTINGS_FILE) != SETTINGS_OK) {
        memset(&settings, 0, sizeof(Settings));
    }

    settings.enable_channel_hopping_index = index;
    const SettingMetadata* metadata = settings_get_metadata(SETTING_ENABLE_CHANNEL_HOPPING);
    variable_item_set_current_value_text(item, metadata->value_names[index]);

    char command[64];
    snprintf(command, sizeof(command), "%s %d\n", metadata->uart_command, index + 1);
    send_uart_command(command, app);

    update_settings_and_write(app, &settings);
}

void on_ble_mac_changed(VariableItem* item) {
    uint8_t index = variable_item_get_current_value_index(item);
    AppState* app = variable_item_get_context(item);
    Settings settings;

    if(settings_storage_load(&settings, GHOST_ESP_APP_SETTINGS_FILE) != SETTINGS_OK) {
        memset(&settings, 0, sizeof(Settings));
    }

    settings.enable_random_ble_mac_index = index;
    const SettingMetadata* metadata = settings_get_metadata(SETTING_ENABLE_RANDOM_BLE_MAC);
    variable_item_set_current_value_text(item, metadata->value_names[index]);

    char command[64];
    snprintf(command, sizeof(command), "%s %d\n", metadata->uart_command, index + 1);
    send_uart_command(command, app);

    update_settings_and_write(app, &settings);
}

void on_stop_on_back_changed(VariableItem* item) {
    uint8_t index = variable_item_get_current_value_index(item);
    AppState* app = variable_item_get_context(item);
    Settings settings;

    if(settings_storage_load(&settings, GHOST_ESP_APP_SETTINGS_FILE) != SETTINGS_OK) {
        memset(&settings, 0, sizeof(Settings));
    }

    settings.stop_on_back_index = index;
    const SettingMetadata* metadata = settings_get_metadata(SETTING_STOP_ON_BACK);
    variable_item_set_current_value_text(item, metadata->value_names[index]);

    update_settings_and_write(app, &settings);
}

void on_reboot_esp_changed(VariableItem* item) {
    AppState* app = variable_item_get_context(item);
    const SettingMetadata* metadata = settings_get_metadata(SETTING_REBOOT_ESP);

    if(metadata->uart_command) {
        char command[64];
        snprintf(command, sizeof(command), "%s", metadata->uart_command);
        send_uart_command(command, app);
    }

    // Keep showing "Press OK"
    variable_item_set_current_value_index(item, 0);
    variable_item_set_current_value_text(item, metadata->value_names[0]);
}
