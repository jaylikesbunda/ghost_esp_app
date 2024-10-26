// settings_ui.h
#pragma once
#include "settings_def.h"
#include <gui/modules/variable_item_list.h>

typedef struct {
    Settings* settings;
    void (*send_uart_command)(const char* command, void* context);
    void* context;
} SettingsUIContext;

// Initialize settings UI items in a variable item list
void settings_setup_gui(VariableItemList* list, SettingsUIContext* context);

