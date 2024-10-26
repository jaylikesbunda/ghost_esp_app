#pragma once
#include "settings_def.h"
#include "app_state.h"
#include <gui/modules/variable_item_list.h>
#include <stdbool.h>
#include <furi.h>
#include <storage/storage.h> 


// Function declarations
void update_settings_and_write(AppState* app, Settings* settings);
void on_rgb_mode_changed(VariableItem* item);
void on_channelswitchdelay_changed(VariableItem* item);
void on_togglechannelhopping_changed(VariableItem* item);
void on_ble_mac_changed(VariableItem* item);
void on_stop_on_back_changed(VariableItem* item);
void on_reboot_esp_changed(VariableItem* item);