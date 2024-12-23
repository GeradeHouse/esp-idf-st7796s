// Set the local log level for this file to DEBUG
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

#include "decode_rgb565ani.h"
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
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

// Define MIN if not already defined
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

static const char *TAG = "decode_rgb565ani";

// Uncomment if you need endianness correction
// static void swap_bytes(uint16_t *buffer, size_t num_pixels) {
//     for (size_t i = 0; i < num_pixels; i++) {
//         uint16_t pixel = buffer[i];
//         buffer[i] = (uint16_t)((pixel << 8) | (pixel >> 8));
//     }
// }

/**
 * A simple structure to hold one frame’s metadata (duration) plus a pointer to the
 * RGB565 data in PSRAM.
 */
typedef struct {
    uint32_t duration_ms;
    uint8_t *data; // pointer to the frame buffer (size = screenWidth * screenHeight * 2)
} local_frame_t;

/**
 * @brief Play a .rgb565ani file on an esp_lcd-based ST7796 (or similar) panel.
 *
 * In this version, we attempt to load multiple frames into PSRAM at once,
 * up to the memory limit. We then play them from PSRAM. If the entire
 * animation doesn't fit at once, we split it into multiple "batches."
 */
esp_err_t play_rgb565ani(esp_lcd_panel_handle_t panel_handle,
                         const char *file,
                         int screenWidth,
                         int screenHeight)
{
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

    // Register current task with the watchdog timer
    esp_task_wdt_add(NULL);

    ESP_LOGI(TAG, "Opening file: %s", file);
    FILE *fp = fopen(file, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "Failed to open file %s", file);
        return ESP_FAIL;
    }

    // 1. Read and validate the magic header
    char magic[10];
    if (fread(magic, 1, 9, fp) != 9) {
        ESP_LOGE(TAG, "Failed to read magic number (expected 9 bytes)");
        fclose(fp);
        esp_task_wdt_delete(NULL);
        return ESP_FAIL;
    }
    magic[9] = '\0';
    if (strcmp(magic, "RGB565ANI") != 0) {
        ESP_LOGE(TAG, "Invalid magic number: '%s' (expected 'RGB565ANI')", magic);
        fclose(fp);
        esp_task_wdt_delete(NULL);
        return ESP_FAIL;
    }

    // 2. Read frame count
    uint32_t frame_count;
    if (fread(&frame_count, sizeof(uint32_t), 1, fp) != 1) {
        ESP_LOGE(TAG, "Failed to read frame count");
        fclose(fp);
        esp_task_wdt_delete(NULL);
        return ESP_FAIL;
    }

    // 3. Read width and height
    uint16_t aniWidth, aniHeight;
    if (fread(&aniWidth, sizeof(uint16_t), 1, fp) != 1) {
        ESP_LOGE(TAG, "Failed to read width");
        fclose(fp);
        esp_task_wdt_delete(NULL);
        return ESP_FAIL;
    }
    if (fread(&aniHeight, sizeof(uint16_t), 1, fp) != 1) {
        ESP_LOGE(TAG, "Failed to read height");
        fclose(fp);
        esp_task_wdt_delete(NULL);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Frame count: %" PRIu32 ", width: %" PRIu16 ", height: %" PRIu16,
             frame_count, aniWidth, aniHeight);

    // 4. Verify .rgb565ani dimension vs the screen
    if (aniWidth != screenWidth || aniHeight != screenHeight) {
        ESP_LOGW(TAG,
                 "Frame size (%" PRIu16 "x%" PRIu16 ") != screen (%dx%d). Displaying anyway.",
                 aniWidth, aniHeight, screenWidth, screenHeight);
    }

    // Size per frame in bytes
    size_t frame_buffer_size = screenWidth * screenHeight * 2;

    // We will load multiple frames into memory if we can
    // First, let's see how many frames *could* fit based on free PSRAM

    size_t free_mem = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    // Rough overhead: each frame also stores duration + pointer overhead, but that’s small
    // We'll focus on the main chunk: frame_buffer_size per frame

    // max_frames_in_ram is the maximum frames we can store at once
    // e.g. free_mem / frame_buffer_size
    uint32_t max_frames_in_ram = free_mem / frame_buffer_size;
    if (max_frames_in_ram == 0) {
        // Not enough memory to store even one frame, fallback to single-frame approach
        ESP_LOGW(TAG, "Not enough PSRAM to store a single frame. Will read-play each frame directly.");
        max_frames_in_ram = 1;
    }

    ESP_LOGI(TAG, "We can store up to %" PRIu32 " frames in PSRAM at once (assuming no other usage).",
             max_frames_in_ram);

    // Rewind file pointer to just after the width/height info
    // (We've already read: magic(9), frame_count(4), width(2), height(2) = 17 bytes)
    // but for safe measure let's compute the offset exactly
    fseek(fp, 9 + 4 + 2 + 2, SEEK_SET);

    uint32_t frame_number = 0;
    bool playback_complete = false;

    // We'll do "batches" of frames. Each batch has up to max_frames_in_ram frames
    // or fewer if we’re near the end of the file.
    // We'll read them all from SD into memory, then display them, then free them.
    while (!playback_complete) {

        // Step A: Determine how many frames we *can* load in this batch
        uint32_t frames_left = frame_count - frame_number; // how many frames remain total
        uint32_t batch_size = (frames_left < max_frames_in_ram) ? frames_left : max_frames_in_ram;
        if (batch_size == 0) {
            // no frames left
            break;
        }

        ESP_LOGI(TAG, "Loading up to %" PRIu32 " frames into PSRAM (batch) ...", batch_size);

        // We'll allocate an array of local_frame_t to store each frame's duration & data pointer
        local_frame_t *batch_frames = heap_caps_calloc(batch_size, sizeof(local_frame_t), MALLOC_CAP_SPIRAM);
        if (!batch_frames) {
            ESP_LOGE(TAG, "Failed to allocate array for frames in this batch");
            break;
        }

        uint32_t frames_loaded = 0; // how many frames we actually load

        // Actually load each frame from file, store it in memory
        for (uint32_t i = 0; i < batch_size; i++) {
            uint32_t this_duration = 0;
            if (fread(&this_duration, sizeof(uint32_t), 1, fp) != 1) {
                if (feof(fp)) {
                    ESP_LOGI(TAG, "End of file reached (no more frames).");
                } else {
                    ESP_LOGE(TAG, "Failed to read frame duration in batch loading");
                }
                playback_complete = true;
                break;
            }

            batch_frames[i].duration_ms = this_duration;

            // allocate data
            batch_frames[i].data = heap_caps_malloc(frame_buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
            if (!batch_frames[i].data) {
                ESP_LOGE(TAG, "Not enough memory to allocate frame data in batch. Stopping here.");
                playback_complete = true;
                break;
            }

            // read the frame data
            size_t total_read   = 0;
            while (total_read < frame_buffer_size) {
                size_t chunk = frame_buffer_size - total_read;
                if (chunk > 32768) {
                    chunk = 32768;
                }
                size_t ret = fread(batch_frames[i].data + total_read, 1, chunk, fp);
                if (ret != chunk) {
                    if (feof(fp)) {
                        ESP_LOGE(TAG, "Unexpected EOF reading frame data (batch load)");
                    } else {
                        ESP_LOGE(TAG, "Error reading frame data chunk (batch load)");
                    }
                    playback_complete = true;
                    break;
                }
                total_read += ret;
                esp_task_wdt_reset();
            }

            if (total_read != frame_buffer_size) {
                break;
            }
            frames_loaded++;
            frame_number++;

            if (frame_number >= frame_count) {
                playback_complete = true;
                break;
            }
        } // end for i < batch_size

        if (frames_loaded == 0) {
            // no frames loaded in this batch => break
            free(batch_frames);
            break;
        }

        ESP_LOGI(TAG, "Batch loaded %" PRIu32 " frames (Now playing them).", frames_loaded);

        // Step B: Display each loaded frame from memory
        for (uint32_t i = 0; i < frames_loaded; i++) {

            // draw the frame in line chunks
            const uint16_t lines_per_chunk = 160; // bigger chunk
            uint16_t y_start = 0;
            while (y_start < screenHeight) {
                uint16_t block_h = MIN(lines_per_chunk, screenHeight - y_start);
                size_t offset = y_start * screenWidth * 2;
                esp_lcd_panel_draw_bitmap(
                    panel_handle,
                    0, y_start, screenWidth, y_start + block_h,
                    batch_frames[i].data + offset
                );
                y_start += block_h;
            }

            // If you want a small delay or use the stored duration:
            // vTaskDelay(pdMS_TO_TICKS(batch_frames[i].duration_ms));
            // ^ commented out if you want max speed
            esp_task_wdt_reset();
        }

        // Step C: Free the batch from memory
        for (uint32_t i = 0; i < frames_loaded; i++) {
            if (batch_frames[i].data) {
                heap_caps_free(batch_frames[i].data);
            }
        }
        free(batch_frames);

    } // end while (!playback_complete)

    fclose(fp);

    // Summarize
    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    float elapsed_sec = (float)(diffTick * portTICK_PERIOD_MS) / 1000.0f;
    float fps = 0.0f;
    if (elapsed_sec > 0) {
        fps = frame_number / elapsed_sec;
    }
    ESP_LOGI(TAG, "RGB565ANI playback done, total frames=%" PRIu32
                  ", time=%.3f s, FPS=%.2f",
             frame_number, elapsed_sec, fps);

    // Unregister from watchdog
    esp_task_wdt_delete(NULL);

    return ESP_OK;
}
