#include "menu.h"
#include <stdlib.h>
#include <string.h>
#include "uart_utils.h"
#include "settings_storage.h"
#include "settings_def.h"
#include "confirmation_view.h"

typedef struct {
    const char* label; // Display label in menu
    const char* command; // UART command to send
    const char* capture_prefix; // Prefix for capture files (NULL if none)
    const char* file_ext; // File extension for captures (NULL if none)
    const char* folder; // Folder for captures (NULL if none)
    bool needs_input; // Whether command requires text input
    const char* input_text; // Text to show in input box (NULL if none)
    bool needs_confirmation; // Whether command needs confirmation
    const char* confirm_header; // Confirmation dialog header
    const char* confirm_text; // Confirmation dialog text
    const char* details_header; // Header for details view
    const char* details_text; // Detailed description/info text
} MenuCommand;

typedef struct {
    const char* label;
    const char* command;
    const char* capture_prefix;
} SniffCommandDef;

typedef struct {
    const char* label;
    const char* command;
} BeaconSpamDef;

typedef struct {
    AppState* state;
    const MenuCommand* command;
} MenuCommandContext;

// Forward declarations of static functions
static void show_menu(
    AppState* state,
    const MenuCommand* commands,
    size_t command_count,
    const char* header,
    Submenu* menu,
    uint8_t view_id);
static void show_command_details(AppState* state, const MenuCommand* command);
static bool menu_input_handler(InputEvent* event, void* context);
static void text_input_result_callback(void* context);
static void confirmation_ok_callback(void* context);
static void confirmation_cancel_callback(void* context);
static void app_info_ok_callback(void* context);
static void execute_menu_command(AppState* state, const MenuCommand* command);
static void error_callback(void* context);

// Sniff command definitions
static const SniffCommandDef sniff_commands[] = {
    {"< Sniff WPS >", "capture -wps\n", "wps_capture"},
    {"< Sniff Raw Packets >", "capture -raw\n", "raw_capture"},
    {"< Sniff Probes >", "capture -p\n", "probe_capture"},
    {"< Sniff Deauth >", "capture -deauth\n", "deauth_capture"},
    {"< Sniff Beacons >", "capture -beacon\n", "beacon_capture"},
    {"< Sniff EAPOL >", "capture -eapol\n", "eapol_capture"},
    {"< Sniff Pwn >", "capture -pwn\n", "pwn_capture"},
};

// Beacon spam command definitions
static const BeaconSpamDef beacon_spam_commands[] = {
    {"< Beacon Spam (List) >", "beaconspam -l\n"},
    {"< Beacon Spam (Random) >", "beaconspam -r\n"},
    {"< Beacon Spam (Rickroll) >", "beaconspam -rr\n"},
    {"< Beacon Spam (Custom) >", "beaconspam"},
};

static size_t current_rgb_index = 0;

static const BeaconSpamDef rgbmode_commands[] = {
    {"< LED: Rainbow >", "rgbmode rainbow\n"},
    {"< LED: Police >", "rgbmode police\n"},
    {"< LED: Strobe >", "rgbmode strobe\n"},
    {"< LED: Off >", "rgbmode off\n"},
    {"< LED: Red >", "rgbmode red\n"},
    {"< LED: Green >", "rgbmode green\n"},
    {"< LED: Blue >", "rgbmode blue\n"},
    {"< LED: Yellow >", "rgbmode yellow\n"},
    {"< LED: Purple >", "rgbmode purple\n"},
    {"< LED: Cyan >", "rgbmode cyan\n"},
    {"< LED: Orange >", "rgbmode orange\n"},
    {"< LED: White >", "rgbmode white\n"},
    {"< LED: Pink >", "rgbmode pink\n"}};

static size_t current_sniff_index = 0;
static size_t current_beacon_index = 0;

// WiFi menu command definitions
static const MenuCommand wifi_commands[] = {
    // Scanning Operations
    {
        .label = "Scan WiFi APs",
        .command = "scanap\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "WiFi AP Scanner",
        .details_text = "Scans for WiFi APs:\n"
                        "- SSID names\n"
                        "- Signal levels\n"
                        "- Security type\n"
                        "- Channel info\n",
    },
    {
        .label = "Scan WiFi Stations",
        .command = "scansta\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "Station Scanner",
        .details_text = "Scans for clients:\n"
                        "- MAC addresses\n"
                        "- Network SSID\n"
                        "- Signal level\n"
                        "Range: ~50-100m\n",
    },
    {
        .label = "Scan All (AP+STA)",
        .command = "scanall\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "Scan All",
        .details_text = "Combined AP/Station scan\n"
                        "and display results.\n",
    },
    {
        .label = "List APs",
        .command = "list -a\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "List Access Points",
        .details_text = "Shows list of APs found\n"
                        "during last scan with:\n"
                        "- Network details\n"
                        "- Channel info\n"
                        "- Security type\n",
    },
    {
        .label = "List Stations",
        .command = "list -s\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "List Stations",
        .details_text = "Shows list of clients\n"
                        "found during last scan:\n"
                        "- Device MAC address\n"
                        "- Connected network\n"
                        "- Signal strength\n",
    },
    {
        .label = "Select AP",
        .command = "select -a",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = true,
        .input_text = "AP Number",
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "Select Access Point",
        .details_text = "Select an AP by number\n"
                        "from the scanned list\n"
                        "for targeting with\n"
                        "other commands.\n",
    },
    // Variable Sniff Command
    {
        .label = "< Sniff WPS >",
        .command = "capture -wps\n",
        .capture_prefix = "wps_capture",
        .file_ext = "pcap",
        .folder = GHOST_ESP_APP_FOLDER_PCAPS,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "Variable Sniff",
        .details_text = "Use Left/Right to change:\n"
                        "- WPS traffic\n"
                        "- Raw packets\n"
                        "- Probe requests\n"
                        "- Deauth frames\n"
                        "- Beacon frames\n"
                        "- EAPOL/Handshakes\n",
    },
    // Variable Beacon Spam Command
    {
        .label = "< Beacon Spam (List) >",
        .command = "beaconspam -l\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = "SSID Name",
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "Variable Beacon Spam",
        .details_text = "Use Left/Right to change:\n"
                        "- List mode\n"
                        "- Random names\n"
                        "- Rickroll mode\n"
                        "- Custom SSID\n"
                        "Range: ~50-100m\n",
    },
    // Attack Operations
    {
        .label = "Deauth",
        .command = "attack -d\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "Deauth Attack",
        .details_text = "Sends deauth frames to\n"
                        "disconnect clients from\n"
                        "selected network.\n"
                        "Range: ~50-100m\n",
    },

    // Portal & Network Operations
    {
        .label = "Evil Portal",
        .command = "startportal",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = true,
        .input_text = "<filepath> <SSID> <PSK (leave blank for open)>",
        .needs_confirmation = false,
        .details_header = "Evil Portal",
        .details_text = "Captive portal for\n"
                        "credential harvest.\n"
                        "Configure in WebUI:\n"
                        "- Portal settings\n"
                        "- Landing page\n",
    },
    {
        .label = "Set WebUI Creds",
        .command = "apcred",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = true,
        .input_text = "MySSID MyPassword",
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "Set AP Credentials",
        .details_text = "Set custom WebUI AP:\n"
                        "Format:\nMySSID MyPassword\n"
                        "Example: GhostNet,spooky123\n",
    },
    {
        .label = "Reset WebUI Creds",
        .command = "apcred -r\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = true,
        .confirm_header = "Reset AP Credentials",
        .confirm_text = "Reset WebUI AP to\n"
                        "default credentials?\n"
                        "SSID: GhostNet\n"
                        "Password: GhostNet\n",
        .details_header = "Reset AP Credentials",
        .details_text = "Restores default WebUI AP:\n"
                        "SSID: GhostNet\n"
                        "Password: GhostNet\n"
                        "Requires ESP reboot\n",
    },
    {
        .label = "Connect To WiFi",
        .command = "connect",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = true,
        .input_text = "SSID",
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "WiFi Connect",
        .details_text = "Connect ESP to WiFi:\n"
                        "Enter SSID followed by password.\n",
    },
    {
        .label = "Cast Random Video",
        .command = "dialconnect\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = true,
        .confirm_header = "Cast Video",
        .confirm_text =
            "Make sure you've connected\nto WiFi first via the\n'Connect to WiFi' option.\n",
        .details_header = "Video Cast",
        .details_text = "Casts random videos\n"
                        "to nearby Cast/DIAL\n"
                        "enabled devices.\n"
                        "Range: ~50m\n",
    },
    {
        .label = "Printer Power",
        .command = "powerprinter\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = true,
        .confirm_header = "Printer Power",
        .confirm_text = "You need to configure\n settings in the WebUI\n for this command.\n",
        .details_header = "WiFi Printer",
        .details_text = "Control power state\n"
                        "of network printers.\n"
                        "Configure in WebUI:\n"
                        "- Printer IP/Port\n"
                        "- Protocol type\n",
    },
    {
        .label = "Scan Local Network",
        .command = "scanlocal\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = true,
        .confirm_header = "Local Network Scan",
        .confirm_text =
            "Make sure you've connected\nto WiFi first via the\n'Connect to WiFi' option.\n",
        .details_header = "Network Scanner",
        .details_text = "Scans local network for:\n"
                        "- Printers\n"
                        "- Smart devices\n"
                        "- Cast devices\n"
                        "- Requires WiFi connection\n",
    },
    {
        .label = "Pineapple Detect",
        .command = "pineap\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "Pineapple Detection",
        .details_text = "Detects WiFi Pineapple devices\n",
    },
    {
        .label = "Channel Congestion",
        .command = "congestion\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "Channel Congestion",
        .details_text = "Display Wi-Fi channel\n"
                        "congestion chart.\n",
    },
    {
        .label = "Scan Ports",
        .command = "scanports",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = true,
        .input_text = "local or IP [options]",
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "Port Scanner",
        .details_text = "Scan ports on local net\n"
                        "or specific IP.\n"
                        "Options: -C, -A, range\n"
                        "Ex: local -C\n"
                        "Ex: 192.168.1.1 80-1000",
    },
    // Unified Stop Command for WiFi Operations
    {
        .label = "Stop All WiFi",
        .command = "stop\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "Stop WiFi Operations",
        .details_text = "Stops all active WiFi\n"
                        "operations including:\n"
                        "- Scanning\n"
                        "- Beacon Spam\n"
                        "- Deauth Attacks\n"
                        "- Packet Captures\n"
                        "- Evil Portal\n",
    },
    // New variable LED effects command (this becomes index 17)
    {
        .label = "< LED: Rainbow >",
        .command = "rgbmode rainbow\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = "LED Effects",
        .confirm_text = NULL,
        .details_header = "LED Effects",
        .details_text = "Control LED effects:\n"
                        "- rainbow, police, strobe, off, or fixed colors\n"
                        "Cycle with Left/Right to select an effect\n",
    },
    // Hardware/Settings Commands
    {
        .label = "Set RGB Pins",
        .command = "setrgbpins",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = true,
        .input_text = "<red> <green> <blue>",
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "Set RGB Pins",
        .details_text = "Change RGB LED pins.\n"
                        "Requires restart.\n"
                        "Use same value for all\n"
                        "pins for single-pin LED.",
    },
    {
        .label = "Show SD Pin Config",
        .command = "sd_config",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "SD Pin Config",
        .details_text = "Show current SD GPIO\n"
                        "pin configuration for\n"
                        "MMC and SPI modes.",
    },
    {
        .label = "Set SD Pins (MMC)",
        .command = "sd_pins_mmc",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = true,
        .input_text = "<clk> <cmd> <d0..d3>",
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "Set SD Pins (MMC)",
        .details_text = "Set GPIO pins for SDMMC.\n"
                        "Requires restart.\n"
                        "Only if firmware built\n"
                        "for SDMMC mode.",
    },
    {
        .label = "Set SD Pins (SPI)",
        .command = "sd_pins_spi",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = true,
        .input_text = "<cs> <clk> <miso> <mosi>",
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "Set SD Pins (SPI)",
        .details_text = "Set GPIO pins for SPI.\n"
                        "Requires restart.\n"
                        "Only if firmware built\n"
                        "for SPI mode.",
    },
    {
        .label = "Save SD Pin Config",
        .command = "sd_save_config",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = true,
        .confirm_header = "Save SD Config",
        .confirm_text = "Save current SD pin\n"
                        "config to SD card?\n"
                        "Requires SD mounted.",
        .details_header = "Save SD Pin Config",
        .details_text = "Save current SD pin\n"
                        "config (both modes) to\n"
                        "SD card (sd_config.conf).",
    },
};

// BLE menu command definitions
static const MenuCommand ble_commands[] = {
    {
        .label = "Skimmer Detection",
        .command = "capture -skimmer\n",
        .capture_prefix = "skimmer_scan",
        .file_ext = "pcap",
        .folder = GHOST_ESP_APP_FOLDER_PCAPS,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "Skimmer Scanner",
        .details_text = "Detects potential\n"
                        "card skimmers by\n"
                        "analyzing BLE\n"
                        "signatures and\n"
                        "known patterns.\n",
    },
    {
        .label = "Find the Flippers",
        .command = "blescan -f\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "Flipper Scanner",
        .details_text = "Scans for Flippers:\n"
                        "- Device name\n"
                        "- BT address\n"
                        "- Signal level\n"
                        "Range: ~50m\n",
    },
    {
        .label = "AirTag Scanner",
        .command = "blescan -a\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "AirTag Scanner",
        .details_text = "Detects nearby Apple\n"
                        "AirTags and shows:\n"
                        "- Device ID\n"
                        "- Signal strength\n"
                        "- Last seen time\n",
    },
    {
        .label = "BLE Raw Capture",
        .command = "capture -ble\n",
        .capture_prefix = "ble_raw_capture",
        .file_ext = "pcap",
        .folder = GHOST_ESP_APP_FOLDER_PCAPS,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "BLE Raw Capture",
        .details_text = "Captures raw BLE\n"
                        "traffic and data.\n"
                        "Range: ~10-30m\n",
    },
    {
        .label = "BLE Spam Detect",
        .command = "blescan -ds\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "BLE Spam Detector",
        .details_text = "Detects BLE spam\n"
                        "(e.g., advertising floods).\n",
    },
    // Unified Stop Command for BLE Operations
    {
        .label = "Stop All BLE",
        .command = "stop\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "Stop BLE Operations",
        .details_text = "Stops all active BLE\n"
                        "operations including:\n"
                        "- BLE Scanning\n"
                        "- Skimmer Detection\n"
                        "- Packet Captures\n"
                        "- Device Detection\n",
    },
};

// GPS menu command definitions
static const MenuCommand gps_commands[] = {
    {
        .label = "GPS Info",
        .command = "gpsinfo\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "GPS Information",
        .details_text = "Shows GPS details:\n"
                        "- Position (Lat/Long)\n"
                        "- Altitude & Speed\n"
                        "- Direction & Quality\n"
                        "- Satellite Status\n",
    },
    {
        .label = "Start Wardriving",
        .command = "startwd\n",
        .capture_prefix = "wardrive_scan",
        .file_ext = "csv",
        .folder = GHOST_ESP_APP_FOLDER_WARDRIVE,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "Wardrive Mode",
        .details_text = "Maps WiFi networks:\n"
                        "- Network info\n"
                        "- GPS location\n"
                        "- Signal levels\n"
                        "Saves as CSV\n",
    },
    {
        .label = "BLE Wardriving",
        .command = "blewardriving\n",
        .capture_prefix = "ble_wardrive",
        .file_ext = "csv",
        .folder = GHOST_ESP_APP_FOLDER_WARDRIVE,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "BLE Wardriving",
        .details_text = "Maps BLE devices:\n"
                        "- Device info\n"
                        "- GPS location\n"
                        "- Signal levels\n"
                        "Saves as CSV\n",
    },
    {
        .label = "GPS Track (GPX)",
        .command = "gpsinfo -t\n",
        .capture_prefix = "gps_track",
        .file_ext = "gpx",
        .folder = GHOST_ESP_APP_FOLDER_WARDRIVE,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "GPS Track (GPX)",
        .details_text = "Records GPS track\n"
                        "in GPX format for\n"
                        "mapping software.\n"
                        "Saves to .gpx file.\n",
    },
    // Unified Stop Command for GPS Operations
    {
        .label = "Stop All GPS",
        .command = "stop\n",
        .capture_prefix = NULL,
        .file_ext = NULL,
        .folder = NULL,
        .needs_input = false,
        .input_text = NULL,
        .needs_confirmation = false,
        .confirm_header = NULL,
        .confirm_text = NULL,
        .details_header = "Stop GPS Operations",
        .details_text = "Stops all active GPS\n"
                        "operations including:\n"
                        "- GPS Info Updates\n"
                        "- WiFi Wardriving\n"
                        "- BLE Wardriving\n"
                        "- GPX Tracking\n",
    },
};

void send_uart_command(const char* command, void* state) {
    AppState* app_state = (AppState*)state;
    uart_send(app_state->uart_context, (uint8_t*)command, strlen(command));
}

void send_uart_command_with_text(const char* command, char* text, AppState* state) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s %s\n", command, text);
    uart_send(state->uart_context, (uint8_t*)buffer, strlen(buffer));
}

void send_uart_command_with_bytes(
    const char* command,
    const uint8_t* bytes,
    size_t length,
    AppState* state) {
    send_uart_command(command, state);
    uart_send(state->uart_context, bytes, length);
}

static void confirmation_ok_callback(void* context) {
    MenuCommandContext* cmd_ctx = context;
    if(cmd_ctx && cmd_ctx->state && cmd_ctx->command) {
        bool file_opened = false;

        // Handle capture commands
        if(cmd_ctx->command->capture_prefix || cmd_ctx->command->file_ext ||
           cmd_ctx->command->folder) {
            FURI_LOG_I("Capture", "Attempting to open PCAP file before sending capture command.");
            file_opened = uart_receive_data(
                cmd_ctx->state->uart_context,
                cmd_ctx->state->view_dispatcher,
                cmd_ctx->state,
                cmd_ctx->command->capture_prefix ? cmd_ctx->command->capture_prefix : "",
                cmd_ctx->command->file_ext ? cmd_ctx->command->file_ext : "",
                cmd_ctx->command->folder ? cmd_ctx->command->folder : "");

            if(!file_opened) {
                FURI_LOG_E("Capture", "Failed to open PCAP file. Aborting capture command.");
                free(cmd_ctx);
                return;
            }

            // Send capture command
            send_uart_command(cmd_ctx->command->command, cmd_ctx->state);
            FURI_LOG_I("Capture", "Capture command sent to firmware.");
        } else {
            // For non-capture confirmation commands, send command and switch to text view
            send_uart_command(cmd_ctx->command->command, cmd_ctx->state);
            uart_receive_data(
                cmd_ctx->state->uart_context,
                cmd_ctx->state->view_dispatcher,
                cmd_ctx->state,
                "",
                "",
                ""); // No capture files needed
        }
    }
    free(cmd_ctx);
}

static void confirmation_cancel_callback(void* context) {
    MenuCommandContext* cmd_ctx = context;
    if(cmd_ctx && cmd_ctx->state) {
        switch(cmd_ctx->state->previous_view) {
        case 1:
            show_wifi_menu(cmd_ctx->state);
            break;
        case 2:
            show_ble_menu(cmd_ctx->state);
            break;
        case 3:
            show_gps_menu(cmd_ctx->state);
            break;
        default:
            show_main_menu(cmd_ctx->state);
            break;
        }
    }
    free(cmd_ctx);
}

// Add at top with other declarations:
static void app_info_ok_callback(void* context) {
    AppState* state = context;
    if(!state) return;

    view_dispatcher_switch_to_view(state->view_dispatcher, state->previous_view);
    state->current_view = state->previous_view;
}

static void show_command_details(AppState* state, const MenuCommand* command) {
    if(!command->details_header || !command->details_text) return;

    // Save current view before switching
    state->previous_view = state->current_view;

    // Setup confirmation view to show details
    confirmation_view_set_header(state->confirmation_view, command->details_header);
    confirmation_view_set_text(state->confirmation_view, command->details_text);

    // Set up callbacks for OK/Cancel to return to previous view
    confirmation_view_set_ok_callback(
        state->confirmation_view,
        app_info_ok_callback, // Reuse app info callback since it does the same thing
        state);
    confirmation_view_set_cancel_callback(state->confirmation_view, app_info_ok_callback, state);

    // Switch to confirmation view
    view_dispatcher_switch_to_view(state->view_dispatcher, 7);
    state->current_view = 7;
}

static void error_callback(void* context) {
    AppState* state = (AppState*)context;
    if(!state) return;
    view_dispatcher_switch_to_view(state->view_dispatcher, state->previous_view);
    state->current_view = state->previous_view;
}

// Text input callback implementation
static void text_input_result_callback(void* context) {
    AppState* input_state = (AppState*)context;
    if(input_state->connect_input_stage == 1) {
        size_t len = strlen(input_state->input_buffer);
        if(len >= sizeof(input_state->connect_ssid)) len = sizeof(input_state->connect_ssid) - 1;
        memcpy(input_state->connect_ssid, input_state->input_buffer, len);
        input_state->connect_ssid[len] = '\0';
        input_state->connect_input_stage = 2;
        text_input_reset(input_state->text_input);
        text_input_set_header_text(input_state->text_input, "PASSWORD");
        text_input_set_result_callback(
            input_state->text_input,
            text_input_result_callback,
            input_state,
            input_state->input_buffer,
            128,
            true);
        view_dispatcher_switch_to_view(input_state->view_dispatcher, 6);
        return;
    }
    if(input_state->connect_input_stage == 2) {
        char buffer[256];
        snprintf(
            buffer,
            sizeof(buffer),
            "connect \"%s\" \"%s\"\n",
            input_state->connect_ssid,
            input_state->input_buffer);
        uart_send(input_state->uart_context, (uint8_t*)buffer, strlen(buffer));
        input_state->connect_input_stage = 0;
        input_state->connect_ssid[0] = '\0';
    } else {
        send_uart_command_with_text(
            input_state->uart_command, input_state->input_buffer, input_state);
    }
    uart_receive_data(
        input_state->uart_context, input_state->view_dispatcher, input_state, "", "", "");
}

static void execute_menu_command(AppState* state, const MenuCommand* command) {
    if(!uart_is_esp_connected(state->uart_context)) {
        state->previous_view = state->current_view;
        confirmation_view_set_header(state->confirmation_view, "Connection Error");
        confirmation_view_set_text(
            state->confirmation_view,
            "No response from ESP!\nIs a command running?\nRestart the app.\nRestart ESP.\nCheck UART Pins.\nReflash if issues persist.\nYou can disable this check in the settings menu.\n\n");
        confirmation_view_set_ok_callback(state->confirmation_view, error_callback, state);
        confirmation_view_set_cancel_callback(state->confirmation_view, error_callback, state);

        view_dispatcher_switch_to_view(state->view_dispatcher, 7);
        state->current_view = 7;
        return;
    }

    if(command->needs_input && strcmp(command->command, "connect") == 0) {
        state->connect_input_stage = 1;
        state->uart_command = command->command;
        text_input_reset(state->text_input);
        text_input_set_header_text(state->text_input, "SSID");
        text_input_set_result_callback(
            state->text_input, text_input_result_callback, state, state->input_buffer, 128, true);
        view_dispatcher_switch_to_view(state->view_dispatcher, 6);
        return;
    }

    // For commands needing input
    if(command->needs_input) {
        state->uart_command = command->command;
        text_input_reset(state->text_input);
        text_input_set_header_text(state->text_input, command->input_text);
        text_input_set_result_callback(
            state->text_input, text_input_result_callback, state, state->input_buffer, 128, true);
        view_dispatcher_switch_to_view(state->view_dispatcher, 6);
        return;
    }

    // For commands needing confirmation
    if(command->needs_confirmation) {
        MenuCommandContext* cmd_ctx = malloc(sizeof(MenuCommandContext));
        cmd_ctx->state = state;
        cmd_ctx->command = command;
        confirmation_view_set_header(state->confirmation_view, command->confirm_header);
        confirmation_view_set_text(state->confirmation_view, command->confirm_text);
        confirmation_view_set_ok_callback(
            state->confirmation_view, confirmation_ok_callback, cmd_ctx);
        confirmation_view_set_cancel_callback(
            state->confirmation_view, confirmation_cancel_callback, cmd_ctx);

        view_dispatcher_switch_to_view(state->view_dispatcher, 7);
        return;
    }

    // Handle variable sniff command
    if(state->current_view == 1 && state->current_index == 6) {
        const SniffCommandDef* current_sniff = &sniff_commands[current_sniff_index];
        // Handle capture commands
        if(current_sniff->capture_prefix) {
            bool file_opened = uart_receive_data(
                state->uart_context,
                state->view_dispatcher,
                state,
                current_sniff->capture_prefix,
                "pcap",
                GHOST_ESP_APP_FOLDER_PCAPS);

            if(!file_opened) {
                FURI_LOG_E("Capture", "Failed to open capture file");
                return;
            }

            furi_delay_ms(10);
            send_uart_command(current_sniff->command, state);
            return;
        }

        uart_receive_data(state->uart_context, state->view_dispatcher, state, "", "", "");

        furi_delay_ms(5);
        send_uart_command(current_sniff->command, state);
        return;
    }

    // Handle variable beacon spam command
    if(state->current_view == 1 && state->current_index == 7) {
        const BeaconSpamDef* current_beacon = &beacon_spam_commands[current_beacon_index];

        // If it's custom mode (last index), handle text input
        if(current_beacon_index == COUNT_OF(beacon_spam_commands) - 1) {
            state->uart_command = current_beacon->command;
            text_input_reset(state->text_input);
            text_input_set_header_text(state->text_input, "SSID Name");
            text_input_set_result_callback(
                state->text_input,
                text_input_result_callback,
                state,
                state->input_buffer,
                128,
                true);
            view_dispatcher_switch_to_view(state->view_dispatcher, 6);
            return;
        }

        // For other modes, send command directly
        uart_receive_data(state->uart_context, state->view_dispatcher, state, "", "", "");
        furi_delay_ms(5);
        send_uart_command(current_beacon->command, state);
        return;
    }

    // Handle variable rgbmode command (new branch for index 17)
    if(state->current_view == 1 && state->current_index == 20) {
        const BeaconSpamDef* current_rgb = &rgbmode_commands[current_rgb_index];
        uart_receive_data(state->uart_context, state->view_dispatcher, state, "", "", "");
        furi_delay_ms(5);
        send_uart_command(current_rgb->command, state);
        return;
    }

    // Handle capture commands
    if(command->capture_prefix || command->file_ext || command->folder) {
        bool file_opened = uart_receive_data(
            state->uart_context,
            state->view_dispatcher,
            state,
            command->capture_prefix ? command->capture_prefix : "",
            command->file_ext ? command->file_ext : "",
            command->folder ? command->folder : "");

        if(!file_opened) {
            FURI_LOG_E("Capture", "Failed to open capture file");
            return;
        }

        furi_delay_ms(10);
        send_uart_command(command->command, state);
        return;
    }

    uart_receive_data(state->uart_context, state->view_dispatcher, state, "", "", "");

    furi_delay_ms(5);
    send_uart_command(command->command, state);
}

// Menu display function implementation
static void show_menu(
    AppState* state,
    const MenuCommand* commands,
    size_t command_count,
    const char* header,
    Submenu* menu,
    uint8_t view_id) {
    submenu_reset(menu);
    submenu_set_header(menu, header);

    for(size_t i = 0; i < command_count; i++) {
        submenu_add_item(menu, commands[i].label, i, submenu_callback, state);
    }

    // Set up view with input handler
    View* menu_view = submenu_get_view(menu);
    view_set_context(menu_view, state);
    view_set_input_callback(menu_view, menu_input_handler);

    // Restore last selection based on menu type
    uint32_t last_index = 0;
    switch(view_id) {
    case 1:
        last_index = state->last_wifi_index;
        break;
    case 2:
        last_index = state->last_ble_index;
        break;
    case 3:
        last_index = state->last_gps_index;
        break;
    }
    if(last_index < command_count) {
        submenu_set_selected_item(menu, last_index);
    }

    view_dispatcher_switch_to_view(state->view_dispatcher, view_id);
    state->current_view = view_id;
    state->previous_view = view_id;
}

// Menu display functions
void show_wifi_menu(AppState* state) {
    show_menu(
        state, wifi_commands, COUNT_OF(wifi_commands), "WiFi Commands:", state->wifi_menu, 1);
}

void show_ble_menu(AppState* state) {
    show_menu(state, ble_commands, COUNT_OF(ble_commands), "BLE Commands:", state->ble_menu, 2);
}

void show_gps_menu(AppState* state) {
    show_menu(state, gps_commands, COUNT_OF(gps_commands), "GPS Commands:", state->gps_menu, 3);
}

// Menu command handlers
void handle_wifi_menu(AppState* state, uint32_t index) {
    if(index < COUNT_OF(wifi_commands)) {
        state->last_wifi_index = index; // Save the selection
        execute_menu_command(state, &wifi_commands[index]);
    }
}

void handle_ble_menu(AppState* state, uint32_t index) {
    if(index < COUNT_OF(ble_commands)) {
        state->last_ble_index = index; // Save the selection
        execute_menu_command(state, &ble_commands[index]);
    }
}

void handle_gps_menu(AppState* state, uint32_t index) {
    if(index < COUNT_OF(gps_commands)) {
        state->last_gps_index = index; // Save the selection
        execute_menu_command(state, &gps_commands[index]);
    }
}

void submenu_callback(void* context, uint32_t index) {
    AppState* state = (AppState*)context;
    state->current_index = index; // Track current selection

    switch(state->current_view) {
    case 0: // Main Menu
        switch(index) {
        case 0:
            show_wifi_menu(state);
            break;
        case 1:
            show_ble_menu(state);
            break;
        case 2:
            show_gps_menu(state);
            break;
        case 3: // Settings button
            view_dispatcher_switch_to_view(state->view_dispatcher, 8);
            state->current_view = 8;
            state->previous_view = 8;
            break;
        }
        break;
    case 1:
        handle_wifi_menu(state, index);
        break;
    case 2:
        handle_ble_menu(state, index);
        break;
    case 3:
        handle_gps_menu(state, index);
        break;
    }
}

static void show_menu_help(void* context, uint32_t index) {
    UNUSED(index);
    AppState* state = context;

    // Save current view
    state->previous_view = state->current_view;

    // Define help text with essential actions only
    const char* help_text = "=== Controls ===\n"
                            "Hold [Ok]\n"
                            "    Show command details\n"
                            "Back button returns to\n"
                            "previous menu\n"
                            "\n"
                            "=== File Locations ===\n"
                            "PCAP files: /pcaps\n"
                            "GPS data: /wardrive\n"
                            "\n"
                            "=== Tips ===\n"
                            "- One capture at a time\n"
                            "  for best performance\n"
                            "- Hold OK on any command\n"
                            "  to see range & details\n"
                            "\n"
                            "=== Settings ===\n"
                            "Configure options in\n"
                            "SET menu including:\n"
                            "- Auto-stop behavior\n"
                            "- LED settings\n"
                            "\n"
                            "Join the Discord\n"
                            "for support and\n"
                            "to stay updated!\n";

    // Set header and help text in the confirmation view
    confirmation_view_set_header(state->confirmation_view, "Quick Help");
    confirmation_view_set_text(state->confirmation_view, help_text);

    // Set callbacks for user actions
    confirmation_view_set_ok_callback(state->confirmation_view, app_info_ok_callback, state);
    confirmation_view_set_cancel_callback(state->confirmation_view, app_info_ok_callback, state);

    // Switch to confirmation view to display help
    view_dispatcher_switch_to_view(state->view_dispatcher, 7);
    state->current_view = 7;
}

bool back_event_callback(void* context) {
    AppState* state = (AppState*)context;
    if(!state) return false;

    uint32_t current_view = state->current_view;

    // Allow confirmation view to handle its own back button
    if(current_view == 7) {
        return false;
    }

    // Handle text box view (view 5)
    if(current_view == 5) {
        FURI_LOG_D("Ghost ESP", "Handling text box view exit");

        // Cleanup text buffer
        if(state->textBoxBuffer) {
            free(state->textBoxBuffer);
            state->textBoxBuffer = malloc(1);
            if(state->textBoxBuffer) {
                state->textBoxBuffer[0] = '\0';
            }
            state->buffer_length = 0;
        }

        // Return to previous menu with selection restored
        switch(state->previous_view) {
        case 1:
            show_wifi_menu(state);
            submenu_set_selected_item(state->wifi_menu, state->last_wifi_index);
            break;
        case 2:
            show_ble_menu(state);
            submenu_set_selected_item(state->ble_menu, state->last_ble_index);
            break;
        case 3:
            show_gps_menu(state);
            submenu_set_selected_item(state->gps_menu, state->last_gps_index);
            break;
        case 8:
            view_dispatcher_switch_to_view(state->view_dispatcher, 8);
            break;
        default:
            show_main_menu(state);
            break;
        }
        state->current_view = state->previous_view;
    }
    // Handle settings menu (view 8)
    else if(current_view == 8) {
        show_main_menu(state);
        state->current_view = 0;
    }
    // Handle settings submenu (view 4)
    else if(current_view == 4) {
        view_dispatcher_switch_to_view(state->view_dispatcher, 8);
        state->current_view = 8;
    }
    // Handle submenu views (1-3)
    else if(current_view >= 1 && current_view <= 3) {
        show_main_menu(state);
        state->current_view = 0;
    }
    // Handle text input view (view 6)
    else if(current_view == 6) {
        // Clear any command setup state
        state->uart_command = NULL;
        // Return to previous menu with selection restored
        switch(state->previous_view) {
        case 1:
            show_wifi_menu(state);
            submenu_set_selected_item(state->wifi_menu, state->last_wifi_index);
            break;
        case 2:
            show_ble_menu(state);
            submenu_set_selected_item(state->ble_menu, state->last_ble_index);
            break;
        case 3:
            show_gps_menu(state);
            submenu_set_selected_item(state->gps_menu, state->last_gps_index);
            break;
        default:
            show_main_menu(state);
            break;
        }
        state->current_view = state->previous_view;
    }
    // Handle main menu (view 0)
    else if(current_view == 0) {
        view_dispatcher_stop(state->view_dispatcher);
    }

    return true;
}

void show_main_menu(AppState* state) {
    main_menu_reset(state->main_menu);
    main_menu_set_header(state->main_menu, "");
    main_menu_add_item(state->main_menu, "WiFi", 0, submenu_callback, state);
    main_menu_add_item(state->main_menu, "BLE", 1, submenu_callback, state);
    main_menu_add_item(state->main_menu, "GPS", 2, submenu_callback, state);
    main_menu_add_item(state->main_menu, " SET", 3, submenu_callback, state);

    // Set up help callback
    main_menu_set_help_callback(state->main_menu, show_menu_help, state);

    view_dispatcher_switch_to_view(state->view_dispatcher, 0);
    state->current_view = 0;
}

static bool menu_input_handler(InputEvent* event, void* context) {
    AppState* state = (AppState*)context;
    bool consumed = false;

    if(!state || !event) return false;

    const MenuCommand* commands = NULL;
    size_t commands_count = 0;
    Submenu* current_menu = NULL;

    // Determine current menu context
    switch(state->current_view) {
    case 1:
        current_menu = state->wifi_menu;
        commands = wifi_commands;
        commands_count = COUNT_OF(wifi_commands);
        break;
    case 2:
        current_menu = state->ble_menu;
        commands = ble_commands;
        commands_count = COUNT_OF(ble_commands);
        break;
    case 3:
        current_menu = state->gps_menu;
        commands = gps_commands;
        commands_count = COUNT_OF(gps_commands);
        break;
    default:
        return false;
    }

    if(!current_menu || !commands) return false;

    uint32_t current_index = submenu_get_selected_item(current_menu);

    switch(event->type) {
    case InputTypeShort:
        switch(event->key) {
        case InputKeyUp:
            if(current_index > 0) {
                submenu_set_selected_item(current_menu, current_index - 1);
            } else {
                // Wrap to bottom
                submenu_set_selected_item(current_menu, commands_count - 1);
            }
            consumed = true;
            break;

        case InputKeyDown:
            if(current_index < commands_count - 1) {
                submenu_set_selected_item(current_menu, current_index + 1);
            } else {
                // Wrap to top
                submenu_set_selected_item(current_menu, 0);
            }
            consumed = true;
            break;

        case InputKeyOk:
            if(current_index < commands_count) {
                state->current_index = current_index;
                execute_menu_command(state, &commands[current_index]);
                consumed = true;
            }
            break;

        case InputKeyBack:
            show_main_menu(state);
            state->current_view = 0;
            consumed = true;
            break;

        case InputKeyRight:
        case InputKeyLeft:
            // Handle sniff command cycling
            if(state->current_view == 1 && current_index == 6) {
                if(event->key == InputKeyRight) {
                    current_sniff_index = (current_sniff_index + 1) % COUNT_OF(sniff_commands);
                } else {
                    current_sniff_index = (current_sniff_index == 0) ?
                                              (size_t)(COUNT_OF(sniff_commands) - 1) :
                                              (current_sniff_index - 1);
                }
                submenu_change_item_label(
                    current_menu, current_index, sniff_commands[current_sniff_index].label);
                consumed = true;
            }
            // Handle beacon spam command cycling
            else if(state->current_view == 1 && current_index == 7) {
                if(event->key == InputKeyRight) {
                    current_beacon_index =
                        (current_beacon_index + 1) % COUNT_OF(beacon_spam_commands);
                } else {
                    current_beacon_index = (current_beacon_index == 0) ?
                                               (size_t)(COUNT_OF(beacon_spam_commands) - 1) :
                                               (current_beacon_index - 1);
                }
                submenu_change_item_label(
                    current_menu, current_index, beacon_spam_commands[current_beacon_index].label);
                consumed = true;
            }
            // Handle rgbmode command cycling (new branch for index 17)
            else if(state->current_view == 1 && current_index == 20) {
                if(event->key == InputKeyRight) {
                    current_rgb_index = (current_rgb_index + 1) % COUNT_OF(rgbmode_commands);
                } else {
                    current_rgb_index = (current_rgb_index == 0) ?
                                            (COUNT_OF(rgbmode_commands) - 1) :
                                            (current_rgb_index - 1);
                }
                submenu_change_item_label(
                    current_menu, current_index, rgbmode_commands[current_rgb_index].label);
                consumed = true;
            }
            break;
        case InputKeyMAX:
            break;
        }
        break;

    case InputTypeLong:
        switch(event->key) {
        case InputKeyUp:
        case InputKeyDown:
        case InputKeyRight:
        case InputKeyLeft:
        case InputKeyBack:
        case InputKeyMAX:
            break;

        case InputKeyOk:
            if(current_index < commands_count) {
                const MenuCommand* command = &commands[current_index];
                if(command->details_header && command->details_text) {
                    show_command_details(state, command);
                    consumed = true;
                }
            }
            break;
        }
        break;

    case InputTypeRepeat:
        switch(event->key) {
        case InputKeyUp:
            if(current_index > 0) {
                submenu_set_selected_item(current_menu, current_index - 1);
            } else {
                // Wrap to bottom
                submenu_set_selected_item(current_menu, commands_count - 1);
            }
            consumed = true;
            break;

        case InputKeyDown:
            if(current_index < commands_count - 1) {
                submenu_set_selected_item(current_menu, current_index + 1);
            } else {
                // Wrap to top
                submenu_set_selected_item(current_menu, 0);
            }
            consumed = true;
            break;

        case InputKeyRight:
        case InputKeyLeft:
        case InputKeyOk:
        case InputKeyBack:
        case InputKeyMAX:
            break;
        }
        break;

    case InputTypePress:
    case InputTypeRelease:
    case InputTypeMAX:
        break;
    }

    return consumed;
}

// 6675636B796F7564656B69
