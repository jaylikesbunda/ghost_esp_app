// settings_def.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

// Settings keys - add new settings here
typedef enum {
    SETTING_RGB_MODE,
    SETTING_CHANNEL_HOP_DELAY,
    SETTING_ENABLE_CHANNEL_HOPPING,
    SETTING_ENABLE_RANDOM_BLE_MAC,
    SETTING_STOP_ON_BACK,
    SETTING_REBOOT_ESP,
    // Add new settings above this line
    SETTINGS_COUNT
} SettingKey;

// Settings operations result
typedef enum {
    SETTINGS_OK,
    SETTINGS_INVALID_VALUE,
    SETTINGS_FILE_ERROR,
    SETTINGS_PARSE_ERROR
} SettingsResult;

// Setting value definitions
typedef enum {
    RGB_MODE_STEALTH,
    RGB_MODE_NORMAL,
    RGB_MODE_RAINBOW,
    RGB_MODE_COUNT
} RGBMode;

typedef enum {
    CHANNEL_HOP_500MS,
    CHANNEL_HOP_1000MS,
    CHANNEL_HOP_2000MS,
    CHANNEL_HOP_3000MS,
    CHANNEL_HOP_4000MS,
    CHANNEL_HOP_COUNT
} ChannelHopDelay;

typedef struct {
    uint8_t rgb_mode_index;
    uint8_t channel_hop_delay_index;
    uint8_t enable_channel_hopping_index;
    uint8_t enable_random_ble_mac_index;
    uint8_t stop_on_back_index;
    uint8_t reboot_esp_index;
} Settings;

// Setting metadata structure
typedef struct {
    const char* name;
    uint8_t max_value;
    const char** value_names;
    const char* uart_command;
} SettingMetadata;

// Value name arrays
#define SETTING_VALUE_NAMES_RGB_MODE (const char*[]){"Stealth", "Normal", "Rainbow"}
#define SETTING_VALUE_NAMES_CHANNEL_HOP (const char*[]){"500ms", "1000ms", "2000ms", "3000ms", "4000ms"}
#define SETTING_VALUE_NAMES_BOOL (const char*[]){"False", "True"}
#define SETTING_VALUE_NAMES_ACTION (const char*[]){"Press OK"}

// Function declarations
const SettingMetadata* settings_get_metadata(SettingKey key);

// Common definitions
#define GHOST_ESP_APP_FOLDER          "/ext/apps_data/ghost_esp"
#define GHOST_ESP_APP_FOLDER_PCAPS    "/ext/apps_data/ghost_esp/pcaps"
#define GHOST_ESP_APP_FOLDER_WARDRIVE "/ext/apps_data/ghost_esp/wardrive"
#define GHOST_ESP_APP_FOLDER_LOGS     "/ext/apps_data/ghost_esp/logs"
#define GHOST_ESP_APP_SETTINGS_FILE   "/ext/apps_data/ghost_esp/settings.ini"

#define SETTINGS_HEADER_MAGIC 0xDEADBEEF
#define SETTINGS_FILE_VERSION 1

// SettingsHeader structure
typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t settings_count;
} SettingsHeader;
