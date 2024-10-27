#include "uart_utils.h"
#include "log_manager.h"
#include <furi.h>
#include <stdlib.h>
#include <string.h>
#include <storage/storage.h>
#include "sequential_file.h"


#define COMMAND_BUFFER_SIZE 128

void uart_storage_rx_callback(uint8_t *buf, size_t len, void *context) {
    UartContext *app = context;
    if(app->storageContext->HasOpenedFile) {
        storage_file_write(app->storageContext->current_file, buf, len);
    }
}

UartStorageContext *uart_storage_init(UartContext *parentContext) {
    UartStorageContext *ctx = malloc(sizeof(UartStorageContext));
    if(!ctx) return NULL;

    // Fast initialization with zero
    memset(ctx, 0, sizeof(UartStorageContext));
    ctx->parentContext = parentContext;
    ctx->storage_api = furi_record_open(RECORD_STORAGE);
    
    // Single allocation for both files
    ctx->current_file = storage_file_alloc(ctx->storage_api);
    ctx->log_file = storage_file_alloc(ctx->storage_api);

    // Create directories in one pass if needed
    storage_simply_mkdir(ctx->storage_api, GHOST_ESP_APP_FOLDER);
    storage_simply_mkdir(ctx->storage_api, GHOST_ESP_APP_FOLDER_LOGS);
    storage_simply_mkdir(ctx->storage_api, GHOST_ESP_APP_FOLDER_WARDRIVE);
    storage_simply_mkdir(ctx->storage_api, GHOST_ESP_APP_FOLDER_PCAPS);

    // Get latest log path (just to know about it, not to append)
    char latest_log_path[256];
    get_latest_log_file(ctx->storage_api, GHOST_ESP_APP_FOLDER_LOGS, "ghost_logs", latest_log_path);
    
    // Always create new file
    sequential_file_open(
        ctx->storage_api,
        ctx->log_file,
        GHOST_ESP_APP_FOLDER_LOGS,
        "ghost_logs",
        "txt");

    return ctx;
}
void uart_storage_reset_logs(UartStorageContext *ctx) {
    if(ctx->log_file) {
        storage_file_close(ctx->log_file);
        storage_file_free(ctx->log_file);
        ctx->log_file = storage_file_alloc(ctx->storage_api);
    }

    // Create new log file
    sequential_file_open(
        ctx->storage_api,
        ctx->log_file,
        GHOST_ESP_APP_FOLDER_LOGS,
        "ghost_logs",
        "txt");
}
void uart_storage_free(UartStorageContext *ctx) {
    if(!ctx) return;

    if(ctx->current_file) storage_file_free(ctx->current_file);
    if(ctx->log_file) storage_file_free(ctx->log_file);
    if(ctx->storage_api) furi_record_close(RECORD_STORAGE);
    free(ctx);
}