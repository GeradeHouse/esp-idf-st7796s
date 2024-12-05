// main/decode_rgb565ani.c

// Set the local log level for this file to DEBUG
#define LOG_LOCAL_LEVEL ESP_LOG_INFO

#include "decode_rgb565ani.h"
#include "st7796s.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>  // Include for PRIu32 and PRIu16
#include "esp_task_wdt.h"  // Include for watchdog functions
#include "esp_system.h"    // Include for system functions
#include "esp_random.h"    // Include for esp_random()
#include <stdlib.h>        // Include for srand() and rand()
#include <time.h>          // Include for time()

static const char *TAG = "decode_rgb565ani";

// Function to swap bytes in the buffer for correct endianness
void swap_bytes(uint16_t *buffer, size_t num_pixels) {
    for (size_t i = 0; i < num_pixels; i++) {
        uint16_t pixel = buffer[i];
        buffer[i] = (pixel << 8) | (pixel >> 8);
    }
}

// esp_err_t play_rgb565ani function
esp_err_t play_rgb565ani(TFT_t *dev, const char *file, int screenWidth, int screenHeight) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

    // Register the current task with the watchdog timer
    esp_task_wdt_add(NULL);

    FILE *fp = fopen(file, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "Failed to open file %s", file);
        return ESP_FAIL;
    }

    // Read and validate header
    char magic[10];
    if (fread(magic, 1, 9, fp) != 9) {
        ESP_LOGE(TAG, "Failed to read magic number");
        fclose(fp);
        esp_task_wdt_delete(NULL);
        return ESP_FAIL;
    }
    magic[9] = '\0';
    if (strcmp(magic, "RGB565ANI") != 0) {
        ESP_LOGE(TAG, "Invalid magic number");
        fclose(fp);
        esp_task_wdt_delete(NULL);
        return ESP_FAIL;
    }

    // Read frame count
    uint32_t frame_count;
    if (fread(&frame_count, sizeof(uint32_t), 1, fp) != 1) {
        ESP_LOGE(TAG, "Failed to read frame count");
        fclose(fp);
        esp_task_wdt_delete(NULL);
        return ESP_FAIL;
    }

    // Read width and height
    uint16_t width, height;
    if (fread(&width, sizeof(uint16_t), 1, fp) != 1) {
        ESP_LOGE(TAG, "Failed to read width");
        fclose(fp);
        esp_task_wdt_delete(NULL);
        return ESP_FAIL;
    }
    if (fread(&height, sizeof(uint16_t), 1, fp) != 1) {
        ESP_LOGE(TAG, "Failed to read height");
        fclose(fp);
        esp_task_wdt_delete(NULL);
        return ESP_FAIL;
    }

    // Read format flag
    uint8_t format_flag;
    if (fread(&format_flag, sizeof(uint8_t), 1, fp) != 1) {
        ESP_LOGE(TAG, "Failed to read format flag");
        fclose(fp);
        esp_task_wdt_delete(NULL);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Frame count: %" PRIu32 ", width: %" PRIu16 ", height: %" PRIu16 ", format flag: %d", frame_count, width, height, format_flag);

    // Verify dimensions match the screen
    if (width != screenWidth || height != screenHeight) {
        ESP_LOGW(TAG, "Frame dimensions (%" PRIu16 "x%" PRIu16 ") don't match screen (%dx%d)! Frames should be exactly %dx%d!",
                 width, height, screenWidth, screenHeight, screenWidth, screenHeight);
    }

    // Prepare for playback
    uint32_t frame_number = 0;
    bool playback_complete = false;

    // Initialize random number generator
    srand((unsigned int)esp_random());

    // Calculate the size of a full frame buffer
    size_t frame_buffer_size = width * height * 2; // Each pixel is 2 bytes

    // Allocate two frame buffers in PSRAM for double buffering
    uint8_t *frame_buffer_a = heap_caps_malloc(frame_buffer_size, MALLOC_CAP_SPIRAM);
    uint8_t *frame_buffer_b = heap_caps_malloc(frame_buffer_size, MALLOC_CAP_SPIRAM);
    if (!frame_buffer_a || !frame_buffer_b) {
        ESP_LOGE(TAG, "Failed to allocate frame buffers in PSRAM");
        if (frame_buffer_a) heap_caps_free(frame_buffer_a);
        if (frame_buffer_b) heap_caps_free(frame_buffer_b);
        fclose(fp);
        esp_task_wdt_delete(NULL);
        return ESP_ERR_NO_MEM;
    }

    // Allocate a small DMA-capable buffer for transferring data to the display
    size_t dma_buffer_size = width * 160 * 2; // Adjust the number of lines per DMA transfer (e.g., 160 lines)
    uint8_t *dma_buffer = heap_caps_malloc(dma_buffer_size, MALLOC_CAP_DMA);
    if (!dma_buffer) {
        ESP_LOGE(TAG, "Failed to allocate DMA buffer");
        heap_caps_free(frame_buffer_a);
        heap_caps_free(frame_buffer_b);
        fclose(fp);
        esp_task_wdt_delete(NULL);
        return ESP_ERR_NO_MEM;
    }

    // Initialize pointers for double buffering
    uint8_t *current_frame_buffer = frame_buffer_a;
    uint8_t *next_frame_buffer = frame_buffer_b;

    // Main playback loop
    while (!playback_complete) {
        // Read frame duration
        uint32_t duration_ms;
        if (fread(&duration_ms, sizeof(uint32_t), 1, fp) != 1) {
            if (feof(fp)) {
                ESP_LOGI(TAG, "End of file reached");
                playback_complete = true;
                break;
            } else {
                ESP_LOGE(TAG, "Failed to read frame duration");
                break;
            }
        }

        // Read frame type flag
        uint8_t frame_type;
        if (fread(&frame_type, sizeof(uint8_t), 1, fp) != 1) {
            ESP_LOGE(TAG, "Failed to read frame type flag");
            break;
        }

        frame_number++;

        // Full frame
        // Read the entire frame data into the next frame buffer
        size_t bytes_read = fread(next_frame_buffer, 1, frame_buffer_size, fp);
        if (bytes_read != frame_buffer_size) {
            ESP_LOGE(TAG, "Failed to read full frame data, expected %zu bytes, got %zu bytes", frame_buffer_size, bytes_read);
            break;
        }

        // Swap bytes for correct endianness
        swap_bytes((uint16_t *)next_frame_buffer, width * height);

        // Transfer data from PSRAM buffer to display in chunks
        size_t lines_remaining = height;
        uint16_t current_line = 0;
        while (lines_remaining > 0) {
            size_t lines_to_transfer = (lines_remaining > 160) ? 160 : lines_remaining; // Adjust lines per transfer as needed
            size_t bytes_to_transfer = width * lines_to_transfer * 2;

            // Copy data from PSRAM to DMA-capable buffer
            memcpy(dma_buffer, next_frame_buffer + current_line * width * 2, bytes_to_transfer);

            // Display the chunk
            lcdDrawBitmap(dev, 0, current_line, width, lines_to_transfer, (uint16_t *)dma_buffer);

            // Update counters
            lines_remaining -= lines_to_transfer;
            current_line += lines_to_transfer;

            // Feed the watchdog to prevent reset
            esp_task_wdt_reset();
        }

        // Swap the buffers
        uint8_t *temp = current_frame_buffer;
        current_frame_buffer = next_frame_buffer;
        next_frame_buffer = temp;


        // Feed the watchdog to prevent reset
        esp_task_wdt_reset();

        // Check if we've reached the end of the frame count
        if (frame_number >= frame_count) {
            ESP_LOGI(TAG, "Reached end of animation frames");
            playback_complete = true;
            break;
        }
    }

    // Cleanup
    heap_caps_free(frame_buffer_a);
    heap_caps_free(frame_buffer_b);
    heap_caps_free(dma_buffer);
    fclose(fp);

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;

    // Calculate frames per second (FPS)
    float elapsed_time_sec = diffTick * portTICK_PERIOD_MS / 1000.0f; // Convert ticks to seconds
    float fps = frame_number / elapsed_time_sec;

    // Log playback statistics
    ESP_LOGI(TAG, "Playback completed - Total frames: %" PRIu32 ", Elapsed time: %.3f seconds, FPS: %.3f",
             frame_number, elapsed_time_sec, fps);

    // Unregister the current task from the watchdog timer
    esp_task_wdt_delete(NULL);

    return ESP_OK;
}
