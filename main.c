#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_power.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_box.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/text_input.h>
#include <storage/storage.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "menu.h"
#include "uart_utils.h"
#include "settings_storage.h"
#include "settings_def.h"
#include "app_state.h"
#include "callbacks.h"
#include "confirmation_view.h"
#include "utils.h"


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
    memset(state, 0, sizeof(AppState));  // Zero all memory first

    // Initialize text buffers
    state->textBoxBuffer = malloc(1);
    state->textBoxBuffer[0] = '\0';
    state->buffer_length = 0;
    state->input_buffer = malloc(32);
    memset(state->input_buffer, 0, 32);

    // Initialize UI components
    state->view_dispatcher = view_dispatcher_alloc();
    state->main_menu = main_menu_alloc();
    state->wifi_menu = submenu_alloc();
    state->ble_menu = submenu_alloc();
    state->gps_menu = submenu_alloc();
    state->text_box = text_box_alloc();
    state->settings_menu = variable_item_list_alloc();
    state->text_input = text_input_alloc();
    state->confirmation_view = confirmation_view_alloc();

    // Set headers after allocation
    main_menu_set_header(state->main_menu, "Select a Utility");
    submenu_set_header(state->wifi_menu, "Select a Wifi Utility");
    submenu_set_header(state->ble_menu, "Select a Bluetooth Utility");
    submenu_set_header(state->gps_menu, "Select a GPS Utility");
    text_input_set_header_text(state->text_input, "Enter Your Text");

    // Initialize storage and load settings before UART
    settings_storage_init();
    if(settings_storage_load(&state->settings, GHOST_ESP_APP_SETTINGS_FILE) != SETTINGS_OK) {
        memset(&state->settings, 0, sizeof(Settings));
        state->settings.stop_on_back_index = 1;
        settings_storage_save(&state->settings, GHOST_ESP_APP_SETTINGS_FILE);
    }

    // Set up settings UI context
    state->settings_ui_context.settings = &state->settings;
    state->settings_ui_context.send_uart_command = send_uart_command;
    state->settings_ui_context.switch_to_view = NULL;
    state->settings_ui_context.show_confirmation_view = show_confirmation_view_wrapper;
    state->settings_ui_context.context = state;

    // Initialize settings menu
    settings_setup_gui(state->settings_menu, &state->settings_ui_context);

    // Initialize UART after settings are ready
    state->uart_context = uart_init(state);

    // Add views
    view_dispatcher_add_view(state->view_dispatcher, 0, main_menu_get_view(state->main_menu));
    view_dispatcher_add_view(state->view_dispatcher, 1, submenu_get_view(state->wifi_menu));
    view_dispatcher_add_view(state->view_dispatcher, 2, submenu_get_view(state->ble_menu));
    view_dispatcher_add_view(state->view_dispatcher, 3, submenu_get_view(state->gps_menu));
    view_dispatcher_add_view(state->view_dispatcher, 4, variable_item_list_get_view(state->settings_menu));
    view_dispatcher_add_view(state->view_dispatcher, 5, text_box_get_view(state->text_box));
    view_dispatcher_add_view(state->view_dispatcher, 6, text_input_get_view(state->text_input));
    view_dispatcher_add_view(state->view_dispatcher, 7, confirmation_view_get_view(state->confirmation_view));

    // Show main menu
    show_main_menu(state);

    // Set up GUI
    Gui* gui = furi_record_open("gui");
    view_dispatcher_attach_to_gui(state->view_dispatcher, gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_set_navigation_event_callback(state->view_dispatcher, back_event_callback);
    view_dispatcher_set_event_callback_context(state->view_dispatcher, state);
    view_dispatcher_run(state->view_dispatcher);
    furi_record_close("gui");

    // Cleanup views
    {
        size_t i;
        for(i = 0; i <= 7; i++) {
            view_dispatcher_remove_view(state->view_dispatcher, i);
        }
    }

    // Cleanup UART
    uart_free(state->uart_context);

    // Cleanup UI components
    text_input_free(state->text_input);
    text_box_free(state->text_box);
    main_menu_free(state->main_menu);
    submenu_free(state->wifi_menu);
    variable_item_list_free(state->settings_menu);
    submenu_free(state->ble_menu);
    submenu_free(state->gps_menu);
    view_dispatcher_free(state->view_dispatcher);
    confirmation_view_free(state->confirmation_view);

    // Cleanup buffers
    free(state->input_buffer);
    free(state->textBoxBuffer);
    free(state);

    // Handle OTG state
    if(furi_hal_power_is_otg_enabled() && !otg_was_enabled) {
        furi_hal_power_disable_otg();
    }

    return 0;
}