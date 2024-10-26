// main.c
#include "menu.h"
#include <stdlib.h>
#include <string.h>
#include "uart_utils.h"
#include "settings_storage.h"
#include "settings_def.h"
#include "app_state.h"
#include "callbacks.h"
#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_box.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/text_input.h>
#include <stdio.h>
#include <stdbool.h>

// Helper function to set up a settings item with current value
static void setup_settings_item(
    VariableItem* item, 
    const SettingMetadata* metadata,
    uint8_t current_value) {
    
    variable_item_set_current_value_index(item, current_value);
    variable_item_set_current_value_text(item, metadata->value_names[current_value]);
}

int32_t ghost_esp_app(void* p) {
    UNUSED(p);

    uint8_t attempts = 0;
    bool otg_was_enabled = furi_hal_power_is_otg_enabled();
    while(!furi_hal_power_is_otg_enabled() && attempts++ < 5) {
        furi_hal_power_enable_otg();
        furi_delay_ms(10);
    }
    furi_delay_ms(200);

    // Set up UI
    AppState* state = malloc(sizeof(AppState));
    state->view_dispatcher = view_dispatcher_alloc();
    state->main_menu = main_menu_alloc();
    state->wifi_menu = submenu_alloc();
    state->ble_menu = submenu_alloc();
    state->gps_menu = submenu_alloc();
    state->text_box = text_box_alloc();
    state->settings_menu = variable_item_list_alloc();
    state->text_input = text_input_alloc();
    state->input_buffer = malloc(32);

    main_menu_set_header(state->main_menu, "Select a Utility");
    submenu_set_header(state->wifi_menu, "Select a Wifi Utility");
    submenu_set_header(state->ble_menu, "Select a Bluetooth Utility");
    submenu_set_header(state->gps_menu, "Select a GPS Utility");
    text_input_set_header_text(state->text_input, "Enter Your Text");

    // Initialize UART
    state->uart_context = uart_init(state);

    // Initialize settings menu items
    VariableItem* settings_items[SETTINGS_COUNT];
    Settings settings;
    settings_storage_init();
    
    // Load settings using settings_storage_load
    if(settings_storage_load(&settings, GHOST_ESP_APP_SETTINGS_FILE) != SETTINGS_OK) {
        // Initialize default settings
        memset(&settings, 0, sizeof(Settings));
        settings.stop_on_back_index = 1;  // Default value

        // Save default settings
        settings_storage_save(&settings, GHOST_ESP_APP_SETTINGS_FILE);
    }


    // Initialize settings menu
    for(SettingKey key = 0; key < SETTINGS_COUNT; key++) {
        const SettingMetadata* metadata = settings_get_metadata(key);
        VariableItemChangeCallback callback = NULL;
        
        // Select the appropriate callback based on setting type
        switch(key) {
            case SETTING_RGB_MODE:
                callback = on_rgb_mode_changed;
                break;
            case SETTING_CHANNEL_HOP_DELAY:
                callback = on_channelswitchdelay_changed;
                break;
            case SETTING_ENABLE_CHANNEL_HOPPING:
                callback = on_togglechannelhopping_changed;
                break;
            case SETTING_ENABLE_RANDOM_BLE_MAC:
                callback = on_ble_mac_changed;
                break;
            case SETTING_STOP_ON_BACK:
                callback = on_stop_on_back_changed;
                break;
            case SETTING_REBOOT_ESP:
                callback = on_reboot_esp_changed;
                break;
            default:
                continue;  // Skip unknown settings
        }
        
        if(callback) {
            settings_items[key] = variable_item_list_add(
                state->settings_menu,
                metadata->name,
                metadata->max_value + 1,
                callback,
                state
            );

            // Set up the settings item with current value
            uint8_t current_value = *((uint8_t*)&settings + key);
            setup_settings_item(settings_items[key], metadata, current_value);
        }
    }

    // Add views
    view_dispatcher_add_view(state->view_dispatcher, 0, main_menu_get_view(state->main_menu));
    view_dispatcher_add_view(state->view_dispatcher, 1, submenu_get_view(state->wifi_menu));
    view_dispatcher_add_view(state->view_dispatcher, 2, submenu_get_view(state->ble_menu));
    view_dispatcher_add_view(state->view_dispatcher, 3, submenu_get_view(state->gps_menu));
    view_dispatcher_add_view(
        state->view_dispatcher, 4, variable_item_list_get_view(state->settings_menu));
    view_dispatcher_add_view(state->view_dispatcher, 5, text_box_get_view(state->text_box));
    view_dispatcher_add_view(state->view_dispatcher, 6, text_input_get_view(state->text_input));

    // Show the main menu
    show_main_menu(state);

    // Open GUI
    Gui* gui = furi_record_open(RECORD_GUI);
    view_dispatcher_attach_to_gui(state->view_dispatcher, gui, ViewDispatcherTypeFullscreen);

    view_dispatcher_set_navigation_event_callback(state->view_dispatcher, back_event_callback);
    view_dispatcher_set_event_callback_context(state->view_dispatcher, state);

    view_dispatcher_run(state->view_dispatcher);

    furi_record_close(RECORD_GUI);

    // Cleanup views
    for(int i = 0; i <= 6; i++) {
        view_dispatcher_remove_view(state->view_dispatcher, i);
    }

    uart_free(state->uart_context);

    // Clean up
    text_input_free(state->text_input);
    text_box_free(state->text_box);
    main_menu_free(state->main_menu);
    submenu_free(state->wifi_menu);
    variable_item_list_free(state->settings_menu);
    submenu_free(state->ble_menu);
    submenu_free(state->gps_menu);
    view_dispatcher_free(state->view_dispatcher);
    free(state->input_buffer);
    free(state);

    // Have to put at bottom otherwise nullptr dereference
    if(furi_hal_power_is_otg_enabled() && !otg_was_enabled) {
        furi_hal_power_disable_otg();
    }

    return 0;
}
