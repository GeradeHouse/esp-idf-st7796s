// main/decode_gif.c

#include "decode_gif.h"
#include "nsgif.h"
#include "st7796s.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_psram.h"
#include <inttypes.h>
#include <string.h>

static const char *TAG = "decode_gif";

// Structure to hold bitmap information
typedef struct {
    uint32_t width;
    uint32_t height;
    uint16_t **pixels;  // 2D array of RGB565 pixels, matches JPEG's approach
    uint8_t *data;      // Original RGBA data from GIF
    bool opaque;
} gif_bitmap_t;

// Create a new bitmap
static nsgif_bitmap_t *bitmap_create(int width, int height) {
    gif_bitmap_t *bitmap = heap_caps_calloc(1, sizeof(gif_bitmap_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!bitmap) {
        ESP_LOGE(TAG, "Failed to allocate memory for bitmap struct");
        return NULL;
    }
    bitmap->width = width;
    bitmap->height = height;
    bitmap->opaque = false;

    // Allocate 2D array for RGB565 pixels
    bitmap->pixels = heap_caps_calloc(height, sizeof(uint16_t *), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!bitmap->pixels) {
        ESP_LOGE(TAG, "Failed to allocate memory for pixels array");
        heap_caps_free(bitmap);
        return NULL;
    }
    for (int i = 0; i < height; i++) {
        bitmap->pixels[i] = heap_caps_malloc(width * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!bitmap->pixels[i]) {
            ESP_LOGE(TAG, "Failed to allocate memory for pixel row %d", i);
            // Free previously allocated rows
            for (int j = 0; j < i; j++) {
                heap_caps_free(bitmap->pixels[j]);
            }
            heap_caps_free(bitmap->pixels);
            heap_caps_free(bitmap);
            return NULL;
        }
        memset(bitmap->pixels[i], 0, width * sizeof(uint16_t));
    }

    // Allocate linear buffer for RGBA8888 data
    size_t data_size = width * height * 4; // 4 bytes per pixel for RGBA8888
    bitmap->data = heap_caps_malloc(data_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!bitmap->data) {
        ESP_LOGE(TAG, "Failed to allocate memory for bitmap data");
        // Free pixel rows
        for (int i = 0; i < height; i++) {
            heap_caps_free(bitmap->pixels[i]);
        }
        heap_caps_free(bitmap->pixels);
        heap_caps_free(bitmap);
        return NULL;
    }
    memset(bitmap->data, 0, data_size);
    return (nsgif_bitmap_t *)bitmap;
}

// Destroy a bitmap
static void bitmap_destroy(nsgif_bitmap_t *bitmap) {
    gif_bitmap_t *gif_bitmap = (gif_bitmap_t *)bitmap;
    if (gif_bitmap) {
        if (gif_bitmap->data) {
            heap_caps_free(gif_bitmap->data);
        }
        if (gif_bitmap->pixels) {
            for (int i = 0; i < gif_bitmap->height; i++) {
                if (gif_bitmap->pixels[i]) {
                    heap_caps_free(gif_bitmap->pixels[i]);
                }
            }
            heap_caps_free(gif_bitmap->pixels);
        }
        heap_caps_free(gif_bitmap);
    }
}

// Get a pointer to the pixel buffer (RGBA8888 data)
static uint8_t *bitmap_get_buffer(nsgif_bitmap_t *bitmap) {
    gif_bitmap_t *gif_bitmap = (gif_bitmap_t *)bitmap;
    return gif_bitmap->data;
}

// Set whether the bitmap is opaque
static void bitmap_set_opaque(nsgif_bitmap_t *bitmap, bool opaque) {
    gif_bitmap_t *gif_bitmap = (gif_bitmap_t *)bitmap;
    gif_bitmap->opaque = opaque;
}

// Test whether the bitmap is opaque
static bool bitmap_test_opaque(nsgif_bitmap_t *bitmap) {
    gif_bitmap_t *gif_bitmap = (gif_bitmap_t *)bitmap;
    return gif_bitmap->opaque;
}

// Notify that the bitmap has been modified
static void bitmap_modified(nsgif_bitmap_t *bitmap) {
    // No action needed in this implementation
}

// Get the row span (number of bytes in a row)
static uint32_t bitmap_get_rowspan(nsgif_bitmap_t *bitmap) {
    gif_bitmap_t *gif_bitmap = (gif_bitmap_t *)bitmap;
    return gif_bitmap->width * 4; // 4 bytes per pixel for RGBA8888
}

// Define the bitmap callback functions
static const nsgif_bitmap_cb_vt bitmap_callbacks = {
    .create = bitmap_create,
    .destroy = bitmap_destroy,
    .get_buffer = bitmap_get_buffer,
    .set_opaque = bitmap_set_opaque,
    .test_opaque = bitmap_test_opaque,
    .modified = bitmap_modified,
    .get_rowspan = bitmap_get_rowspan
};

// Fast RGB565 conversion without floating point operations
static inline uint16_t rgba_to_rgb565(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    // Simple transparency handling - blend with black background
    if (a < 255) {
        // For partially transparent pixels, apply alpha
        r = (r * a) >> 8;
        g = (g * a) >> 8;
        b = (b * a) >> 8;
    }

    // Convert to RGB565 format directly
    // R: bits 15-11 (5 bits)
    // G: bits 10-5  (6 bits)
    // B: bits 4-0   (5 bits)
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

esp_err_t decode_gif(TFT_t *dev, const char *file, int screenWidth, int screenHeight) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

    lcdSetFontDirection(dev, 0);
    // lcdFillScreen(dev, BLACK);

    // Initialize GIF decoder with corrected bitmap format
    nsgif_t *gif = NULL;
    nsgif_error res = nsgif_create(&bitmap_callbacks, NSGIF_BITMAP_FMT_ABGR8888, &gif);
    if (res != NSGIF_OK) {
        ESP_LOGE(TAG, "Error creating GIF decoder: %d (%s)", res, nsgif_strerror(res));
        return ESP_FAIL;
    }

    // Read and parse GIF file
    FILE* fp = fopen(file, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "Failed to open GIF file: %s", file);
        nsgif_destroy(gif);
        return ESP_ERR_NOT_FOUND;
    }

    fseek(fp, 0, SEEK_END);
    long gif_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    uint8_t *gif_data = heap_caps_malloc(gif_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!gif_data) {
        ESP_LOGE(TAG, "Failed to allocate memory for GIF data");
        fclose(fp);
        nsgif_destroy(gif);
        return ESP_ERR_NO_MEM;
    }

    size_t read_bytes = fread(gif_data, 1, gif_size, fp);
    fclose(fp);

    if (read_bytes != gif_size) {
        ESP_LOGE(TAG, "Failed to read entire GIF file");
        heap_caps_free(gif_data);
        nsgif_destroy(gif);
        return ESP_FAIL;
    }

    res = nsgif_data_scan(gif, gif_size, gif_data);
    nsgif_data_complete(gif);
    if (res != NSGIF_OK) {
        ESP_LOGE(TAG, "Error parsing GIF data: %d (%s)", res, nsgif_strerror(res));
        heap_caps_free(gif_data);
        nsgif_destroy(gif);
        return ESP_FAIL;
    }

    // Get GIF information and verify dimensions
    const nsgif_info_t *gif_info = nsgif_get_info(gif);
    ESP_LOGI(TAG, "GIF info - width: %" PRIu32 ", height: %" PRIu32 ", frame_count: %" PRIu32,
             gif_info->width, gif_info->height, gif_info->frame_count);

    // Verify dimensions match the screen
    if (gif_info->width != screenWidth || gif_info->height != screenHeight) {
        ESP_LOGW(TAG, "GIF dimensions (%" PRIu32 "x%" PRIu32 ") don't match screen (%dx%d)! GIF should be exactly %dx%d!",
                 gif_info->width, gif_info->height, screenWidth, screenHeight,
                 screenWidth, screenHeight);
    }

    // Allocate 2D pixel buffer for RGB565 pixels
    // Since we're using a 2D array, we don't need an additional linear buffer
    // The `pixels` array in `gif_bitmap_t` will hold the RGB565 data

    ESP_LOGI(TAG, "Starting frame drawing with dimensions: GIF(%" PRIu32 "x%" PRIu32 ") -> Display(%dx%d)",
             gif_info->width, gif_info->height, screenWidth, screenHeight);

    // Animation loop
    uint32_t frame_count = 0;
    bool animation_complete = false;
    uint32_t time_cs = 0;
    uint32_t next_frame_time = 0;

    while (!animation_complete) {
        vTaskDelay(1); // Feed watchdog timer to prevent timeout while processing frames

        time_cs = esp_timer_get_time() / 10000; // centiseconds
        if (time_cs < next_frame_time) continue;

        nsgif_rect_t frame_rect;
        uint32_t delay_cs;
        uint32_t frame;
        res = nsgif_frame_prepare(gif, &frame_rect, &delay_cs, &frame);

        if (res == NSGIF_ERR_ANIMATION_END) {
            animation_complete = true;
            break;
        }

        if (res != NSGIF_OK) {
            ESP_LOGW(TAG, "Error preparing frame: %d (%s)", res, nsgif_strerror(res));
            break;
        }

        nsgif_bitmap_t *bitmap;
        res = nsgif_frame_decode(gif, frame, &bitmap);
        if (res != NSGIF_OK) {
            ESP_LOGW(TAG, "Error decoding frame: %d (%s)", res, nsgif_strerror(res));
            break;
        }

        next_frame_time = delay_cs == NSGIF_INFINITE ? UINT32_MAX : time_cs + delay_cs;

        gif_bitmap_t *gif_bitmap = (gif_bitmap_t *)bitmap;
        uint8_t *frame_buffer = gif_bitmap->data;

        // Pre-convert the entire frame to RGB565 and store in the 2D pixels array
        for (int y = 0; y < gif_info->height; y++) {
            // Feed watchdog every few lines to prevent watchdog timeout
            if ((y % 32) == 0) {
                // vTaskDelay(1);
            }

            // Convert one line of pixels from R8G8B8A8 to RGB565
            for (int x = 0; x < gif_info->width; x++) {
                int pixel_index = (y * gif_info->width + x) * 4;
                uint8_t r = frame_buffer[pixel_index + 0];  // Red
                uint8_t g = frame_buffer[pixel_index + 1];  // Green
                uint8_t b = frame_buffer[pixel_index + 2];  // Blue
                uint8_t a = frame_buffer[pixel_index + 3];  // Alpha

                // Convert RGBA to RGB565 and store in 2D pixels array
                gif_bitmap->pixels[y][x] = rgba_to_rgb565(r, g, b, a);
            }
        }

        // Draw each row from the 2D pixels array
        for (int y = 0; y < gif_info->height; y++) {
            // Feed watchdog every few lines to prevent watchdog timeout
            if ((y % 160) == 0) {
                // vTaskDelay(1);
            }

            // Draw the line directly from the 2D pixels array
            lcdDrawMultiPixels(dev, 0, y, gif_info->width, gif_bitmap->pixels[y]);
        }

        frame_count++;
        if (frame_count % 12 == 0) {
            ESP_LOGI(TAG, "Processed %" PRIu32 " frames", frame_count);
        }

        // **Added Condition to Stop After 24 Frames**
        if (frame_count >= 24) {
            ESP_LOGI(TAG, "Reached 48 frames. Stopping GIF playback.");
            animation_complete = true;
            break;
        }
    }

    // Cleanup
    nsgif_destroy(gif);
    heap_caps_free(gif_data);

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(TAG, "GIF playback completed - Total frames: %" PRIu32 ", Elapsed time[ms]: %u", 
            frame_count, (unsigned int)(diffTick * portTICK_PERIOD_MS));

    return ESP_OK;
}
