// main/decode_rgb565ani.h

#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"  // for esp_lcd_panel_handle_t

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Play a .rgb565ani file on an esp_lcd-based display (ST7796 or similar).
 *
 * This function does partial drawing in chunks so we don't exceed the i80 driver’s limit.
 *
 * @param panel_handle   The panel handle returned by esp_lcd_new_panel_st7796
 * @param file           Path to the .rgb565ani file
 * @param screenWidth    Width of the display in pixels
 * @param screenHeight   Height of the display in pixels
 * @return esp_err_t     ESP_OK on success, or an error code
 */
esp_err_t play_rgb565ani(esp_lcd_panel_handle_t panel_handle,
                         const char *file,
                         int screenWidth,
                         int screenHeight);

#ifdef __cplusplus
}
#endif
