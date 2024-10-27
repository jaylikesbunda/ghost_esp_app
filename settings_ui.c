#include "settings_ui.h"
#include "settings_ui_types.h"
#include "settings_def.h"
#include "app_state.h"
#include "sequential_file.h"
#include "uart_utils.h"
#include <furi.h>
#include <gui/modules/variable_item_list.h>
#include <storage/storage.h>
#include "settings_storage.h"
#include "utils.h"
#include "callbacks.h"  // Added for access to the callbacks


typedef struct {

    SettingsUIContext* settings_ui_context;

    SettingKey key;

} VariableItemContext;



// Function to clear log files

static void clear_log_files(void* context) {

    SettingsUIContext* settings_context = (SettingsUIContext*)context;

    FURI_LOG_I("ClearLogs", "Starting log cleanup");

    

    // First close current log file if it exists

    if(settings_context && settings_context->context) {

        AppState* app = (AppState*)settings_context->context;

        if(app->uart_context && app->uart_context->storageContext && 

           app->uart_context->storageContext->log_file) {

            FURI_LOG_D("ClearLogs", "Closing current log file");

            storage_file_close(app->uart_context->storageContext->log_file);

        }

    }



    Storage* storage = furi_record_open(RECORD_STORAGE);

    File* dir = storage_file_alloc(storage);

    FileInfo file_info;

    char* filename = malloc(256);

    int deleted_count = 0;



    FURI_LOG_I("ClearLogs", "Opening logs directory: %s", GHOST_ESP_APP_FOLDER_LOGS);

    if(storage_dir_open(dir, GHOST_ESP_APP_FOLDER_LOGS)) {

        while(storage_dir_read(dir, &file_info, filename, 256)) {

            if(!(file_info.flags & FSF_DIRECTORY)) {

                char full_path[512];

                snprintf(full_path, sizeof(full_path), "%s/%s", GHOST_ESP_APP_FOLDER_LOGS, filename);

                FURI_LOG_I("ClearLogs", "Attempting to delete: %s", full_path);

                

                if(storage_file_exists(storage, full_path)) {

                    if(storage_simply_remove(storage, full_path)) {

                        FURI_LOG_I("ClearLogs", "Successfully deleted: %s", filename);

                        deleted_count++;

                    } else {

                        FURI_LOG_E("ClearLogs", "Failed to delete: %s", filename);

                    }

                }

            }

        }

    }



    free(filename);

    storage_dir_close(dir);

    storage_file_free(dir);

    

    FURI_LOG_I("ClearLogs", "Deleted %d files", deleted_count);



    // Create new log file

    if(settings_context && settings_context->context) {

        AppState* app = (AppState*)settings_context->context;

        if(app->uart_context && app->uart_context->storageContext) {

            FURI_LOG_I("ClearLogs", "Creating new log file");

            sequential_file_open(

                app->uart_context->storageContext->storage_api,

                app->uart_context->storageContext->log_file,

                GHOST_ESP_APP_FOLDER_LOGS,

                "ghost_logs",

                "txt");

        }

    }



    furi_record_close(RECORD_STORAGE);

    FURI_LOG_I("ClearLogs", "Log cleanup complete");

}



bool settings_set(Settings* settings, SettingKey key, uint8_t value, void* context) {
    if(key >= SETTINGS_COUNT) return false;
    bool changed = false;
   
    switch(key) {
        case SETTING_RGB_MODE:
            if(settings->rgb_mode_index != value) {
                settings->rgb_mode_index = value;
                changed = true;
            }
            break;

        case SETTING_CHANNEL_HOP_DELAY:
            if(settings->channel_hop_delay_index != value) {
                settings->channel_hop_delay_index = value;
                changed = true;
            }
            break;

        case SETTING_ENABLE_CHANNEL_HOPPING:
            if(settings->enable_channel_hopping_index != value) {
                settings->enable_channel_hopping_index = value;
                changed = true;
            }
            break;

        case SETTING_ENABLE_RANDOM_BLE_MAC:
            if(settings->enable_random_ble_mac_index != value) {
                settings->enable_random_ble_mac_index = value;
                changed = true;
            }
            break;

        case SETTING_STOP_ON_BACK:
            if(settings->stop_on_back_index != value) {
                settings->stop_on_back_index = value;
                changed = true;
            }
            break;

        case SETTING_REBOOT_ESP:
            if(value == 0) { // Execute on press
                SettingsUIContext* settings_context = (SettingsUIContext*)context;
                if(settings_context && settings_context->send_uart_command) {
                    settings_context->send_uart_command("handle_reboot\n", settings_context->context);
                }
            }
            break;

        case SETTING_CLEAR_LOGS:
            if(value == 0) { // Execute on press
                clear_log_files(context);
            }
            break;

        case SETTING_CLEAR_NVS:
            if(value == 0) { // Execute on press
                SettingsUIContext* settings_context = (SettingsUIContext*)context;
                if(settings_context && settings_context->context) {
                    show_confirmation_dialog_ex(
                        settings_context->context,
                        "Clear NVS",
                        "Are you sure you want to clear NVS?\nThis will reset all ESP settings.",
                        nvs_clear_confirmed_callback,
                        nvs_clear_cancelled_callback);
                }
            }
            break;

        default:
            return false;
    }

    if(changed) {
        // Save settings to storage
        settings_storage_save(settings, GHOST_ESP_APP_SETTINGS_FILE);
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

        case SETTING_REBOOT_ESP:

        case SETTING_CLEAR_LOGS:

        case SETTING_CLEAR_NVS:

            return 0;

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



    if(settings_set(context->settings, key, value, context)) {  // Pass context here

        variable_item_set_current_value_text(item, metadata->value_names[value]);

        if(metadata->uart_command && context->send_uart_command && 

           key != SETTING_REBOOT_ESP && key != SETTING_CLEAR_NVS) {  // Skip for action buttons

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