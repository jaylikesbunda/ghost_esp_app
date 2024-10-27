#include "callbacks.h"
#include "app_state.h"
#include "menu.h"
#include "settings_def.h"
#include "uart_utils.h"




void on_rgb_mode_changed(VariableItem* item) {
    AppState* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    app->settings.rgb_mode_index = index;
    variable_item_set_current_value_text(item, SETTING_VALUE_NAMES_RGB_MODE[index]);
    char command[32];
    snprintf(command, sizeof(command), "setsetting -i 1 -v %d\n", index + 1);
    send_uart_command(command, app);
}

void on_channelswitchdelay_changed(VariableItem* item) {
    AppState* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    app->settings.channel_hop_delay_index = index;
    variable_item_set_current_value_text(item, SETTING_VALUE_NAMES_CHANNEL_HOP[index]);
    char command[32];
    snprintf(command, sizeof(command), "setsetting -i 2 -v %d\n", index + 1);
    send_uart_command(command, app);
}

void on_togglechannelhopping_changed(VariableItem* item) {
    AppState* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    app->settings.enable_channel_hopping_index = index;
    variable_item_set_current_value_text(item, SETTING_VALUE_NAMES_BOOL[index]);
    char command[32];
    snprintf(command, sizeof(command), "setsetting -i 3 -v %d\n", index + 1);
    send_uart_command(command, app);
}

void on_ble_mac_changed(VariableItem* item) {
    AppState* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    app->settings.enable_random_ble_mac_index = index;
    variable_item_set_current_value_text(item, SETTING_VALUE_NAMES_BOOL[index]);
    char command[32];
    snprintf(command, sizeof(command), "setsetting -i 4 -v %d\n", index + 1);
    send_uart_command(command, app);
}

void on_stop_on_back_changed(VariableItem* item) {
    AppState* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    app->settings.stop_on_back_index = index;
    variable_item_set_current_value_text(item, SETTING_VALUE_NAMES_BOOL[index]);
}

void on_reboot_esp_changed(VariableItem* item) {
    AppState* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, SETTING_VALUE_NAMES_ACTION[index]);
    send_uart_command("handle_reboot\n", app);
}

void on_clear_logs_changed(VariableItem* item) {
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, SETTING_VALUE_NAMES_ACTION[index]);
    // Note: Actual log clearing is handled in settings_ui.c
}

void on_clear_nvs_changed(VariableItem* item) {
    AppState* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, SETTING_VALUE_NAMES_ACTION[index]);

    if(index == 0) {
        show_confirmation_dialog_ex(  // Changed to _ex version
            app,
            "Clear NVS",
            "Are you sure you want to clear NVS?\nThis will reset all ESP settings.",
            nvs_clear_confirmed_callback,
            nvs_clear_cancelled_callback);
    }
}

// Remove static from these implementations:
void nvs_clear_confirmed_callback(void* context) {
    AppState* app = context;
    send_uart_command("handle_clearnvs\n", app);
    view_dispatcher_switch_to_view(app->view_dispatcher, app->previous_view);
}

void nvs_clear_cancelled_callback(void* context) {
    AppState* app = context;
    view_dispatcher_switch_to_view(app->view_dispatcher, app->previous_view);
}