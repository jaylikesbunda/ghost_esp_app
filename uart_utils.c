#include "uart_utils.h"
#include <furi.h>
#include <stdlib.h>
#include <string.h>
#include "sequential_file.h"
#include "gui/modules/text_box.h"

#define WORKER_ALL_RX_EVENTS (WorkerEvtStop | WorkerEvtRxDone | WorkerEvtPcapDone)

void handle_uart_rx_data(uint8_t *buf, size_t len, void *context) {
    AppState *state = (AppState *)context;
    const size_t MAX_BUFFER_SIZE = 2 * 1024;

    if(!state || !buf || len == 0) return;

    // Ensure proper null termination
    if(len > 0) {
        buf[len] = '\0';
    }

    size_t new_total_len = state->buffer_length + len + 1;

    // Handle buffer overflow
    if(new_total_len > MAX_BUFFER_SIZE) {
        // Keep last portion of existing content
        size_t keep_size = MAX_BUFFER_SIZE / 2;
        if(state->textBoxBuffer && state->buffer_length > keep_size) {
            memmove(state->textBoxBuffer, 
                   state->textBoxBuffer + state->buffer_length - keep_size,
                   keep_size);
            state->buffer_length = keep_size;
            new_total_len = keep_size + len + 1;
        } else {
            // If current buffer is small, just clear it
            free(state->textBoxBuffer);
            state->textBoxBuffer = NULL;
            state->buffer_length = 0;
            new_total_len = len + 1;
        }
    }

    // Allocate or reallocate buffer
    char *new_buffer = realloc(state->textBoxBuffer, new_total_len);
    if(!new_buffer) {
        FURI_LOG_E("UART", "Failed to allocate memory for text concatenation");
        return;
    }

    // Copy new data
    memcpy(new_buffer + state->buffer_length, buf, len);
    new_buffer[new_total_len - 1] = '\0';
    
    state->textBoxBuffer = new_buffer;
    state->buffer_length = new_total_len - 1;

    // Update text box and scroll to end
    text_box_set_text(state->text_box, state->textBoxBuffer);
    text_box_set_focus(state->text_box, TextBoxFocusEnd);

    // Write to log file if enabled
    if(state->uart_context->storageContext->log_file) {
        storage_file_write(state->uart_context->storageContext->log_file, buf, len);
    }
}

// UART receive callback function
static void
uart_rx_callback(FuriHalSerialHandle *handle, FuriHalSerialRxEvent event, void *context)
{
    UartContext *uart = (UartContext *)context;

    if (event == FuriHalSerialRxEventData)
    {
        uint8_t data = furi_hal_serial_async_rx(handle);

        const char *mark_begin = "[BUF/BEGIN]";
        const char *mark_close = "[BUF/CLOSE]";

        if (uart->mark_test_idx != 0)
        {
            if (data == mark_begin[uart->mark_test_idx] ||
                data == mark_close[uart->mark_test_idx])
            {
                uart->mark_test_buf[uart->mark_test_idx++] = data;

                if (uart->mark_test_idx == sizeof(uart->mark_test_buf))
                {
                    if (!memcmp(
                            uart->mark_test_buf, (void *)mark_begin, sizeof(uart->mark_test_buf)))
                    {
                        uart->pcap = true;
                    }
                    else if (!memcmp(
                                 uart->mark_test_buf,
                                 (void *)mark_close,
                                 sizeof(uart->mark_test_buf)))
                    {
                        uart->pcap = false;
                    }
                    else
                    {
                    }

                    uart->mark_test_idx = 0;
                }
                return;
            }
            else
            {
                if (uart->pcap)
                {
                    furi_stream_buffer_send(
                        uart->pcap_stream, uart->mark_test_buf, uart->mark_test_idx, 0);
                    furi_thread_flags_set(furi_thread_get_id(uart->rx_thread), WorkerEvtPcapDone);
                }
                else
                {
                    furi_stream_buffer_send(
                        uart->rx_stream, uart->mark_test_buf, uart->mark_test_idx, 0);
                    furi_thread_flags_set(furi_thread_get_id(uart->rx_thread), WorkerEvtRxDone);
                }
                uart->mark_test_idx = 0;
            }
        }

        if (data == mark_begin[0])
        {
            uart->mark_test_buf[uart->mark_test_idx++] = data;
        }
        else
        {
            if (uart->pcap)
            {
                furi_stream_buffer_send(uart->pcap_stream, &data, 1, 0);
                furi_thread_flags_set(furi_thread_get_id(uart->rx_thread), WorkerEvtPcapDone);
            }
            else
            {
                furi_stream_buffer_send(uart->rx_stream, &data, 1, 0);
                furi_thread_flags_set(furi_thread_get_id(uart->rx_thread), WorkerEvtRxDone);
            }
        }
    }
    else
    {
    }
}

// UART worker thread function
static int32_t uart_worker(void *context)
{
    UartContext *uart = (void *)context;

    while (1)
    {
        uint32_t events =
            furi_thread_flags_wait(WORKER_ALL_RX_EVENTS, FuriFlagWaitAny, FuriWaitForever);
        furi_check((events & FuriFlagError) == 0);
        if (events & WorkerEvtStop)
            break;
        if (events & WorkerEvtRxDone)
        {
            size_t len = furi_stream_buffer_receive(uart->rx_stream, uart->rx_buf, RX_BUF_SIZE, 0);
            if (len > 0)
            {
                if (uart->handle_rx_data_cb)
                    uart->handle_rx_data_cb(uart->rx_buf, len, uart->state);
            }
        }
        if (events & WorkerEvtPcapDone)
        {
            size_t len =
                furi_stream_buffer_receive(uart->pcap_stream, uart->rx_buf, RX_BUF_SIZE, 0);
            if (len > 0)
            {
                if (uart->handle_rx_pcap_cb)
                    uart->handle_rx_pcap_cb(uart->rx_buf, len, uart);
            }
        }
    }

    furi_stream_buffer_free(uart->rx_stream);
    furi_stream_buffer_free(uart->pcap_stream);

    return 0;
}

// Initialize UART context and set up UART
UartContext *uart_init(AppState *state)
{
    UartContext *uart = malloc(sizeof(UartContext));
    uart->serial_handle = furi_hal_serial_control_acquire(FuriHalSerialIdUsart);
    furi_hal_serial_init(uart->serial_handle, 115200);
    uart->rx_stream = furi_stream_buffer_alloc(RX_BUF_SIZE, 1);
    uart->pcap_stream = furi_stream_buffer_alloc(RX_BUF_SIZE, 1);
    uart->storage_stream = furi_stream_buffer_alloc(RX_BUF_SIZE, 1);
    uart->rx_thread = furi_thread_alloc();
    furi_thread_set_name(uart->rx_thread, "UART_Receive");
    furi_thread_set_stack_size(uart->rx_thread, 1024);
    furi_thread_set_context(uart->rx_thread, uart);
    furi_thread_set_callback(uart->rx_thread, uart_worker);
    furi_thread_start(uart->rx_thread);
    furi_hal_serial_async_rx_start(uart->serial_handle, uart_rx_callback, uart, false);
    uart->storageContext = uart_storage_init(uart);

    uart->state = state;
    uart->handle_rx_data_cb = handle_uart_rx_data;
    uart->handle_rx_pcap_cb = uart_storage_rx_callback;

    return uart;
}

// Cleanup UART resources and stop UART thread
void uart_free(UartContext *uart)
{
    if (!uart)
        return;
    furi_thread_flags_set(furi_thread_get_id(uart->rx_thread), WorkerEvtStop);
    furi_thread_join(uart->rx_thread);
    furi_thread_free(uart->rx_thread);
    furi_hal_serial_deinit(uart->serial_handle);
    furi_hal_serial_control_release(uart->serial_handle);
    uart_storage_free(uart->storageContext);
    free(uart);
}

// Stop the UART thread (typically when exiting)
void uart_stop_thread(UartContext *uart)
{
    if (uart && uart->rx_thread)
    {
        furi_thread_flags_set(furi_thread_get_id(uart->rx_thread), WorkerEvtStop);
    }
}

// Send data over UART
void uart_send(UartContext *uart, const uint8_t *data, size_t len)
{
    if (uart && uart->serial_handle)
    {
        furi_hal_serial_tx(uart->serial_handle, data, len);
    }
}

void uart_receive_data(
    UartContext *uart,
    ViewDispatcher *view_dispatcher,
    AppState *state,
    const char *prefix,
    const char *extension,
    const char *TargetFolder)
{
    UNUSED(uart);
    text_box_set_text(state->text_box, "");
    text_box_set_focus(state->text_box, TextBoxFocusEnd);
    if (strlen(prefix) > 1)
    {
        uart->storageContext->HasOpenedFile = sequential_file_open(
            uart->storageContext->storage_api,
            uart->storageContext->current_file,
            TargetFolder,
            prefix,
            extension);
    }
    view_dispatcher_switch_to_view(view_dispatcher, 5);
    state->current_view = 5;
}

bool uart_is_esp_connected(UartContext* uart) {
    if(!uart || !uart->serial_handle) return false;

    // Clear any existing data
    if(uart->state && uart->state->textBoxBuffer) {
        free(uart->state->textBoxBuffer);
        uart->state->textBoxBuffer = malloc(1);
        uart->state->textBoxBuffer[0] = '\0';
        uart->state->buffer_length = 0;
    }

    // Send a simple test command
    const char* test_cmd = "info\n";
    uart_send(uart, (uint8_t*)test_cmd, strlen(test_cmd));

    // Wait for response
    uint32_t start_time = furi_get_tick();
    while(furi_get_tick() - start_time < ESP_CHECK_TIMEOUT_MS) {
        if(uart->state && uart->state->textBoxBuffer && uart->state->buffer_length > 0) {
            return true;
        }
        furi_delay_ms(10);
    }

    return false;
}