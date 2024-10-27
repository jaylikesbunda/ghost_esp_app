#pragma once
#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/text_box.h>
#include <gui/modules/text_input.h>
#include "gui_modules/mainmenu.h"
#include "settings_def.h"
#include "app_types.h"
#include "settings_ui_types.h"


struct AppState {
    // Views
    ViewDispatcher* view_dispatcher;
    MainMenu* main_menu;
    Submenu* wifi_menu;
    Submenu* ble_menu;
    Submenu* gps_menu;
    VariableItemList* settings_menu;
    TextBox* text_box;
    TextInput* text_input;
    ConfirmationView* confirmation_view;  // Add this

    // UART Context
    UartContext* uart_context;

    // Settings
    Settings settings;
    SettingsUIContext settings_ui_context;
    
    // State
    uint8_t current_view;
    uint8_t previous_view;
    char* input_buffer;
    const char* uart_command;
    char* textBoxBuffer;
    size_t buffer_length;
};