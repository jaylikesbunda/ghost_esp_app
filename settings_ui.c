#include "settings_ui.h"
#include "settings_def.h"
#include <furi.h>
#include <gui/modules/variable_item_list.h>

typedef struct {
    SettingsUIContext* settings_ui_context;
    SettingKey key;
} VariableItemContext;


bool settings_set(Settings* settings, SettingKey key, uint8_t value) {
    if(key >= SETTINGS_COUNT) return false;
    
    switch(key) {
        case SETTING_RGB_MODE:
            settings->rgb_mode_index = value;
            break;
        case SETTING_CHANNEL_HOP_DELAY:
            settings->channel_hop_delay_index = value;
            break;
        case SETTING_ENABLE_CHANNEL_HOPPING:
            settings->enable_channel_hopping_index = value;
            break;
        case SETTING_ENABLE_RANDOM_BLE_MAC:
            settings->enable_random_ble_mac_index = value;
            break;
        case SETTING_STOP_ON_BACK:
            settings->stop_on_back_index = value;
            break;
        default:
            return false;
    }
    return true;
}

uint8_t settings_get(const Settings* settings, SettingKey key) {
    switch(key) {
        case SETTING_RGB_MODE:
            return settings->rgb_mode_index;
        case SETTING_CHANNEL_HOP_DELAY:
            return settings->channel_hop_delay_index;
        case SETTING_ENABLE_CHANNEL_HOPPING:
            return settings->enable_channel_hopping_index;
        case SETTING_ENABLE_RANDOM_BLE_MAC:
            return settings->enable_random_ble_mac_index;
        case SETTING_STOP_ON_BACK:
            return settings->stop_on_back_index;
        default:
            return 0;
    }
}

static void settings_item_change_callback(VariableItem* item) {
    VariableItemContext* item_context = variable_item_get_context(item);
    SettingsUIContext* context = item_context->settings_ui_context;
    SettingKey key = item_context->key;

    uint8_t value = variable_item_get_current_value_index(item);
    const SettingMetadata* metadata = settings_get_metadata(key);

    if(settings_set(context->settings, key, value)) {
        variable_item_set_current_value_text(item, metadata->value_names[value]);

        if(metadata->uart_command && context->send_uart_command) {
            char command[64];
            snprintf(command, sizeof(command), "%s %d\n", metadata->uart_command, value + 1);
            context->send_uart_command(command, context->context);
        }
    }
}


void settings_setup_gui(VariableItemList* list, SettingsUIContext* context) {
    for(SettingKey key = 0; key < SETTINGS_COUNT; key++) {
        const SettingMetadata* metadata = settings_get_metadata(key);
        
        VariableItemContext* item_context = malloc(sizeof(VariableItemContext));
        item_context->settings_ui_context = context;
        item_context->key = key;

        VariableItem* item = variable_item_list_add(
            list,
            metadata->name,
            metadata->max_value + 1,
            settings_item_change_callback,
            item_context);

        uint8_t current_value = settings_get(context->settings, key);
        variable_item_set_current_value_index(item, current_value);
        variable_item_set_current_value_text(item, metadata->value_names[current_value]);
    }
}
