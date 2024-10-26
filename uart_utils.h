#ifndef UART_UTILS_H
#define UART_UTILS_H

#include <furi_hal_serial.h>
#include <furi_hal_serial_types.h>
#include <furi_hal_serial_control.h>
#include <furi/core/thread.h>
#include <furi/core/stream_buffer.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/text_box.h>
#include "menu.h"
#include "uart_storage.h"

#define RX_BUF_SIZE                   256
#define GHOST_ESP_APP_FOLDER          "/ext/apps_data/ghost_esp"
#define GHOST_ESP_APP_FOLDER_PCAPS    "/ext/apps_data/ghost_esp/pcaps"
#define GHOST_ESP_APP_FOLDER_WARDRIVE "/ext/apps_data/ghost_esp/wardrive"
#define GHOST_ESP_APP_FOLDER_LOGS     "/ext/apps_data/ghost_esp/logs"
#define GHOST_ESP_APP_SETTINGS_FILE   "/ext/apps_data/ghost_esp/settings.ini"
#define ESP_CHECK_TIMEOUT_MS 100

typedef enum {
    WorkerEvtStop = (1 << 0),
    WorkerEvtRxDone = (1 << 1),
    WorkerEvtPcapDone = (1 << 2),
    WorkerEvtStorage = (1 << 3),
} WorkerEvtFlags;

typedef struct UartContext {
    FuriHalSerialHandle* serial_handle;
    FuriThread* rx_thread;
    FuriStreamBuffer* rx_stream;
    FuriStreamBuffer* pcap_stream;
    FuriStreamBuffer* storage_stream;
    bool pcap;
    uint8_t mark_test_buf[11];
    uint8_t mark_test_idx;
    uint8_t rx_buf[RX_BUF_SIZE + 1];
    void (*handle_rx_data_cb)(uint8_t* buf, size_t len, void* context);
    void (*handle_rx_pcap_cb)(uint8_t* buf, size_t len, void* context);
    void (*handle_storage_cb)(uint8_t* buf, size_t len, void* context);
    AppState* state;
    UartStorageContext* storageContext;
} UartContext;

// Function prototypes
UartContext* uart_init(AppState* state);
void uart_free(UartContext* uart);
void uart_stop_thread(UartContext* uart);
void uart_send(UartContext* uart, const uint8_t* data, size_t len);
void uart_receive_data(
    UartContext* uart,
    ViewDispatcher* view_dispatcher,
    AppState* current_view,
    const char* prefix,
    const char* extension,
    const char* TargetFolder);
bool uart_is_esp_connected(UartContext* uart);
#endif // UART_UTILS_H
