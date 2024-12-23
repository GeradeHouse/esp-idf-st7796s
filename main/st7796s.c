// main/st7796s.c

/*
 * st7796s.c
 * 
 * ESP-IDF driver for the ST7796S 3.5 Inch 320x480 display.
 * Originally used SPI protocol, now adapted to use PARALLEL protocol.
 * 
 * This driver provides functions to initialize, configure, and control the ST7796S LCD display,
 * including:
 * - Display initialization and configuration
 * - Parallel communication handling (previously SPI)
 * - Graphics rendering (pixels, lines, shapes)
 * - Text rendering and font support
 * - Screen rotation and inversion
 * - Backlight control
 * - Efficient display updates
 * - Resource management and de-initialization
 * 
 * Note:
 * Make sure that all instances of %u with %" PRIu32 " in ESP_LOGI and ESP_LOGD statements where the corresponding argument is of type uint32_t.
 * Ensure that the PRIu32 macro from <inttypes.h> is used for proper formatting of uint32_t types.
 *
 * Author: GeradeHouse Productions
 * Date: November 23, 2024
 */

#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>  // Add this include for strtof
#include <stdbool.h> // Added this include to ensure bool is defined
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <driver/gpio.h>
#include "esp_log.h"

#include "st7796s.h"  // Updated to the correct header file
#include "sdkconfig.h" // Include configuration header

#define TAG "ST7796S"
#define _DEBUG_ 0

// Initialize ColorTweaks with values from menuconfig
ColorTweaks color_tweaks = {
    .shadows_cyan_red = CONFIG_SHADOWS_CYAN_RED,
    .shadows_magenta_green = CONFIG_SHADOWS_MAGENTA_GREEN,
    .shadows_yellow_blue = CONFIG_SHADOWS_YELLOW_BLUE,

    .midtones_cyan_red = CONFIG_MIDTONES_CYAN_RED,
    .midtones_magenta_green = CONFIG_MIDTONES_MAGENTA_GREEN,
    .midtones_yellow_blue = CONFIG_MIDTONES_YELLOW_BLUE,

    .highlights_cyan_red = CONFIG_HIGHLIGHTS_CYAN_RED,
    .highlights_magenta_green = CONFIG_HIGHLIGHTS_MAGENTA_GREEN,
    .highlights_yellow_blue = CONFIG_HIGHLIGHTS_YELLOW_BLUE,
};

// Declare static variables for gamma correction
static float gamma_red = 2.2f;
static float gamma_green = 2.2f;
static float gamma_blue = 2.2f;

// Declare static variables for brightness and contrast
static int brightness_percent = 0;
static int contrast_percent = 0;

// Function to initialize gamma correction values
void init_gamma_values(void) {
    ESP_LOGI(TAG, "Initializing gamma correction values");
    // Parse gamma correction values from configuration with sanity checks
    gamma_red = strtof(CONFIG_GAMMA_R, NULL);
    if (gamma_red < 1.0f || gamma_red > 4.0f) {
        ESP_LOGW(TAG, "Invalid GAMMA_R value '%s'; using default 2.2", CONFIG_GAMMA_R);
        gamma_red = 2.2f;
    } else {
        ESP_LOGI(TAG, "Gamma Red set to %.2f", gamma_red);
    }

    gamma_green = strtof(CONFIG_GAMMA_G, NULL);
    if (gamma_green < 1.0f || gamma_green > 4.0f) {
        ESP_LOGW(TAG, "Invalid GAMMA_G value '%s'; using default 2.2", CONFIG_GAMMA_G);
        gamma_green = 2.2f;
    } else {
        ESP_LOGI(TAG, "Gamma Green set to %.2f", gamma_green);
    }

    gamma_blue = strtof(CONFIG_GAMMA_B, NULL);
    if (gamma_blue < 1.0f || gamma_blue > 4.0f) {
        ESP_LOGW(TAG, "Invalid GAMMA_B value '%s'; using default 2.2", CONFIG_GAMMA_B);
        gamma_blue = 2.2f;
    } else {
        ESP_LOGI(TAG, "Gamma Blue set to %.2f", gamma_blue);
    }
}

// Function to initialize brightness and contrast values
void init_brightness_contrast_values(void) {
    ESP_LOGI(TAG, "Initializing brightness and contrast values");
    brightness_percent = CONFIG_BRIGHTNESS;
    if (brightness_percent < -100 || brightness_percent > 100) {
        ESP_LOGW(TAG, "Invalid BRIGHTNESS value '%d'; using default 0", brightness_percent);
        brightness_percent = 0;
    } else {
        ESP_LOGI(TAG, "Brightness set to %d%%", brightness_percent);
    }

    contrast_percent = CONFIG_CONTRAST;
    if (contrast_percent < -100 || contrast_percent > 100) {
        ESP_LOGW(TAG, "Invalid CONTRAST value '%d'; using default 0", contrast_percent);
        contrast_percent = 0;
    } else {
        ESP_LOGI(TAG, "Contrast set to %d%%", contrast_percent);
    }
}

// The following parallel communication functions replace the original SPI-based functions.
// The logic remains the same, but data is now sent via parallel GPIO lines instead of SPI.

// Helper function: set data pins (D0-D7) from a byte
static void parallel_set_data_pins(uint8_t val) {
    // -------------------------------------------
    // BEGIN MODIFICATIONS FOR CONDITIONAL LOGGING
    // -------------------------------------------

    // Static variables to remember the last logged value, how many times it repeated,
    // a time window to limit logs per second, and how many logs we've done in the current window.
    static uint8_t last_logged_val = 0xFF;
    static uint32_t repeated_count = 0;

    // We'll track the last time we reset our "one-second" window (in milliseconds).
    static uint64_t window_start_ms = 0;
    static uint32_t log_count_in_window = 0;

    // Current time in microseconds
    uint64_t now_us = esp_timer_get_time();
    // Convert to milliseconds
    uint64_t now_ms = now_us / 1000ULL;

    // If more than 1000 ms have passed since we started the window, reset.
    if ((now_ms - window_start_ms) > 1000ULL) {
        window_start_ms = now_ms;
        log_count_in_window = 0;
    }

    // Decide if we are allowed to log this message.
    bool can_log = true;

    // Check if this value is the same as last time we logged.
    if (val == last_logged_val) {
        repeated_count++;
        // If we already logged the same value 3 times, skip logging.
        if (repeated_count > 3) {
            can_log = false;
        }
    } else {
        // It's a different value: reset repeated count and update last_logged_val
        last_logged_val = val;
        repeated_count = 1;
    }

    // Also check the per-second rate limit of 3 logs per second.
    if (log_count_in_window >= 3) {
        can_log = false;
    }

    // If logging is allowed, do it and increment log_count_in_window.
    if (can_log) {
        ESP_LOGD(TAG, "Setting data pins to val=0x%02X", val);
        log_count_in_window++;
    }

    // -----------------------------------------
    // END MODIFICATIONS FOR CONDITIONAL LOGGING
    // -----------------------------------------

    // Write each bit of val to the respective D0-D7 GPIO lines.
    // Using the configured pins from Kconfig.
    gpio_set_level(CONFIG_D0_GPIO, (val >> 0) & 0x01);
    gpio_set_level(CONFIG_D1_GPIO, (val >> 1) & 0x01);
    gpio_set_level(CONFIG_D2_GPIO, (val >> 2) & 0x01);
    gpio_set_level(CONFIG_D3_GPIO, (val >> 3) & 0x01);
    gpio_set_level(CONFIG_D4_GPIO, (val >> 4) & 0x01);
    gpio_set_level(CONFIG_D5_GPIO, (val >> 5) & 0x01);
    gpio_set_level(CONFIG_D6_GPIO, (val >> 6) & 0x01);
    gpio_set_level(CONFIG_D7_GPIO, (val >> 7) & 0x01);
}


// Helper function: pulse the WR line to latch data
static void parallel_pulse_wr(int16_t wr_gpio) {
    // ESP_LOGD(TAG, "Pulsing WR");
    gpio_set_level(wr_gpio, 0);
    // A short delay may be considered if needed. For now, assume the timing is sufficient.
    gpio_set_level(wr_gpio, 1);
}

// Parallel master init function replaces spi_master_init()
// It initializes the parallel interface GPIO lines for data and control.
void parallel_master_init(TFT_t *dev,
                          int16_t GPIO_MOSI, int16_t GPIO_SCLK, int16_t GPIO_CS,
                          int16_t GPIO_DC, int16_t GPIO_RESET, int16_t GPIO_BL,
                          int16_t GPIO_WR, int16_t GPIO_RD)
{
    ESP_LOGI(TAG, "Initializing GPIO_DC=%d", GPIO_DC);
    gpio_reset_pin(GPIO_DC);
    gpio_set_direction(GPIO_DC, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_DC, 0);
    ESP_LOGI(TAG, "GPIO_DC=%d set to LOW", GPIO_DC);

    // --- BEGIN CHANGE: remove references to dev->_cs, just drive CS if given ---
    if (GPIO_CS >= 0) {
        ESP_LOGI(TAG, "Initializing GPIO_CS=%d", GPIO_CS);
        gpio_reset_pin(GPIO_CS);
        gpio_set_direction(GPIO_CS, GPIO_MODE_OUTPUT);
        // Drive CS low to activate the LCD
        gpio_set_level(GPIO_CS, 0);
        ESP_LOGI(TAG, "GPIO_CS=%d set to LOW (active)", GPIO_CS);
    } else {
        ESP_LOGW(TAG, "GPIO_CS not defined (value: %d)", GPIO_CS);
    }
    // --- END CHANGE ---

    // Initialize WR pin (write strobe)
    ESP_LOGI(TAG, "Initializing GPIO_WR=%d", GPIO_WR);
    gpio_reset_pin(GPIO_WR);
    gpio_set_direction(GPIO_WR, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_WR, 1);
    ESP_LOGI(TAG, "GPIO_WR=%d set to HIGH", GPIO_WR);

    // Initialize RD pin (read strobe) - if we do not use reading, we can set it high
    ESP_LOGI(TAG, "Initializing GPIO_RD=%d", GPIO_RD);
    if (GPIO_RD >= 0) {
        gpio_reset_pin(GPIO_RD);
        gpio_set_direction(GPIO_RD, GPIO_MODE_OUTPUT);
        gpio_set_level(GPIO_RD, 1);
        ESP_LOGI(TAG, "GPIO_RD=%d set to HIGH", GPIO_RD);
    } else {
        ESP_LOGW(TAG, "GPIO_RD not defined (value: %d)", GPIO_RD);
    }

    // Initialize RESET pin
    ESP_LOGI(TAG, "Initializing GPIO_RESET=%d", GPIO_RESET);
    if (GPIO_RESET >= 0) {
        gpio_reset_pin(GPIO_RESET);
        gpio_set_direction(GPIO_RESET, GPIO_MODE_OUTPUT);
        gpio_set_level(GPIO_RESET, 1);  // Ensure the reset pin starts high
        ESP_LOGI(TAG, "GPIO_RESET=%d set to HIGH", GPIO_RESET);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(GPIO_RESET, 0);  // Pulse reset pin
        ESP_LOGI(TAG, "GPIO_RESET=%d set to LOW (pulse)", GPIO_RESET);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(GPIO_RESET, 1);  // Set back to high
        ESP_LOGI(TAG, "GPIO_RESET=%d set to HIGH (release)", GPIO_RESET);
        vTaskDelay(pdMS_TO_TICKS(100));
    } else {
        ESP_LOGW(TAG, "GPIO_RESET not defined (value: %d)", GPIO_RESET);
    }

    // Initialize data pins D0-D7 as outputs
    ESP_LOGI(TAG, "Initializing data pins D0-D7");
    gpio_reset_pin(CONFIG_D0_GPIO); gpio_set_direction(CONFIG_D0_GPIO, GPIO_MODE_OUTPUT);
    gpio_reset_pin(CONFIG_D1_GPIO); gpio_set_direction(CONFIG_D1_GPIO, GPIO_MODE_OUTPUT);
    gpio_reset_pin(CONFIG_D2_GPIO); gpio_set_direction(CONFIG_D2_GPIO, GPIO_MODE_OUTPUT);
    gpio_reset_pin(CONFIG_D3_GPIO); gpio_set_direction(CONFIG_D3_GPIO, GPIO_MODE_OUTPUT);
    gpio_reset_pin(CONFIG_D4_GPIO); gpio_set_direction(CONFIG_D4_GPIO, GPIO_MODE_OUTPUT);
    gpio_reset_pin(CONFIG_D5_GPIO); gpio_set_direction(CONFIG_D5_GPIO, GPIO_MODE_OUTPUT);
    gpio_reset_pin(CONFIG_D6_GPIO); gpio_set_direction(CONFIG_D6_GPIO, GPIO_MODE_OUTPUT);
    gpio_reset_pin(CONFIG_D7_GPIO); gpio_set_direction(CONFIG_D7_GPIO, GPIO_MODE_OUTPUT);

    // Initialize Backlight pin
    ESP_LOGI(TAG, "Initializing GPIO_BL=%d", GPIO_BL);
    if (GPIO_BL >= 0) {
        gpio_reset_pin(GPIO_BL);
        gpio_set_direction(GPIO_BL, GPIO_MODE_OUTPUT);
        gpio_set_level(GPIO_BL, 0);  // Set GPIO_BL low initially
        ESP_LOGI(TAG, "GPIO_BL=%d set to LOW (initial state)", GPIO_BL);
        ESP_LOGI(TAG, "Backlight pin initialized on GPIO_BL=%d", GPIO_BL);
    } else {
        ESP_LOGW(TAG, "GPIO_BL not defined (value: %d)", GPIO_BL);
    }

    // Store GPIO numbers in device structure
    dev->_dc = GPIO_DC;
    dev->_bl = GPIO_BL;
    dev->_reset = GPIO_RESET;
    dev->_wr = GPIO_WR;
    dev->_rd = GPIO_RD;

    ESP_LOGI(TAG, "Parallel bus initialized successfully");
}


bool parallel_master_write_byte(TFT_t *dev, const uint8_t *Data, size_t DataLength) {
    // For the parallel bus, we write each byte and pulse WR for each.
    // This replaces the SPI transaction logic.
    if (DataLength > 0) {
        for (size_t i = 0; i < DataLength; i++) {
            // ESP_LOGD(TAG, "Writing byte 0x%02X", Data[i]);
            parallel_set_data_pins(Data[i]);
            parallel_pulse_wr(dev->_wr);
        }
    } else {
        ESP_LOGW(TAG, "parallel_master_write_byte called with DataLength=0");
    }
    return true;
}

bool parallel_master_write_command(TFT_t *dev, uint8_t cmd) {
    // ESP_LOGD(TAG, "Writing command 0x%02X", cmd);
    gpio_set_level(dev->_dc, 0);
    uint8_t Byte = cmd;
    return parallel_master_write_byte(dev, &Byte, 1);
}

bool parallel_master_write_data_byte(TFT_t *dev, uint8_t data) {
    // ESP_LOGD(TAG, "Writing data byte 0x%02X", data);
    gpio_set_level(dev->_dc, 1);
    uint8_t Byte = data;
    return parallel_master_write_byte(dev, &Byte, 1);
}

bool parallel_master_write_data_word(TFT_t *dev, uint16_t data) {
    // ESP_LOGD(TAG, "Writing data word 0x%04X", data);
    uint8_t Byte[2];
    Byte[0] = (data >> 8) & 0xFF;
    Byte[1] = data & 0xFF;
    gpio_set_level(dev->_dc, 1);
    return parallel_master_write_byte(dev, Byte, 2);
}

bool parallel_master_write_addr(TFT_t *dev, uint16_t addr1, uint16_t addr2) {
    // ESP_LOGD(TAG, "Writing address from 0x%04X to 0x%04X", addr1, addr2);
    uint8_t Byte[4];
    Byte[0] = (addr1 >> 8) & 0xFF;
    Byte[1] = addr1 & 0xFF;
    Byte[2] = (addr2 >> 8) & 0xFF;
    Byte[3] = addr2 & 0xFF;
    gpio_set_level(dev->_dc, 1);
    return parallel_master_write_byte(dev, Byte, 4);
}

bool parallel_master_write_color(TFT_t *dev, uint16_t color, uint32_t size) {
    // ESP_LOGD(TAG, "Writing color 0x%04X repeated %" PRIu32 " times", color, size);
    gpio_set_level(dev->_dc, 1);
    uint8_t high = (color >> 8) & 0xFF;
    uint8_t low = color & 0xFF;
    for (uint32_t i = 0; i < size; i++) {
        parallel_set_data_pins(high);
        parallel_pulse_wr(dev->_wr);
        parallel_set_data_pins(low);
        parallel_pulse_wr(dev->_wr);
    }
    return true;
}

bool parallel_master_write_colors(TFT_t *dev, uint16_t *colors, uint16_t size) {
    // ESP_LOGD(TAG, "Writing %" PRIu16 " colors", size);
    gpio_set_level(dev->_dc, 1);
    for (uint16_t i = 0; i < size; i++) {
        uint8_t high = (colors[i] >> 8) & 0xFF;
        uint8_t low = colors[i] & 0xFF;
        parallel_set_data_pins(high);
        parallel_pulse_wr(dev->_wr);
        parallel_set_data_pins(low);
        parallel_pulse_wr(dev->_wr);
    }
    return true;
}

void delayMS(int ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

// The following code for lcdInit, lcdDrawPixel, etc. remains the same,
// except that where previously SPI commands were written using spi_master_write_*(),
// we now use the parallel_master_write_*() functions.

// Initialize the LCD display with specified dimensions, offsets, and orientation
void lcdInit(TFT_t *dev, int width, int height, int offsetx, int offsety, uint8_t orientation) {
    // Set the display dimensions and offsets
    dev->_width = width;            // Set the display width in pixels
    dev->_height = height;          // Set the display height in pixels
    dev->_offsetx = offsetx;        // Set the horizontal offset (useful for displays with non-zero origin)
    dev->_offsety = offsety;        // Set the vertical offset (useful for displays with non-zero origin)
    dev->_font_direction = DIRECTION0; // Initialize text direction to default (e.g., left-to-right)
    dev->_font_fill = false;        // Disable font fill (background color for text)
    dev->_font_underline = false;   // Disable font underline

    ESP_LOGI(TAG, "Initializing ST7796S LCD");

    // Check if RESET pin is used (non-negative value indicates the pin is defined)
    if (dev->_reset >= 0) {
        // Perform a hardware reset sequence
        gpio_set_level(dev->_reset, 0); // Set RESET pin low to reset the display
        delayMS(20);                    // Wait for 20 milliseconds for the reset to take effect
        gpio_set_level(dev->_reset, 1); // Set RESET pin high to complete the reset
        delayMS(120);                   // Wait for 120 milliseconds for the display to stabilize
    }

    // Send software reset command to the display
    ESP_LOGI(TAG, "Sending Software Reset");
    parallel_master_write_command(dev, 0x01); // Send the Software Reset command (0x01)
    delayMS(150);                        // Wait for 150 milliseconds for the reset process to complete

    // Initialize gamma correction values
    init_gamma_values();

    // Initialize brightness and contrast values
    init_brightness_contrast_values();

    // Exit sleep mode to turn on the display
    ESP_LOGI(TAG, "Exiting Sleep Mode");
    parallel_master_write_command(dev, 0x11); // Send the Sleep Out command (0x11)
    delayMS(120);                        // Wait for 120 milliseconds for the display to wake up

    // Set the memory data access control (defines the orientation of the display)
    ESP_LOGI(TAG, "Setting Memory Data Access Control");
    parallel_master_write_command(dev, 0x36);    // Send the Memory Data Access Control command (0x36)
    parallel_master_write_data_byte(dev, orientation); // Set the display orientation
    delayMS(10);                            // Wait for 10 milliseconds after setting orientation

    // Set the pixel format to 16 bits per pixel (RGB565 format)
    ESP_LOGI(TAG, "Setting Interface Pixel Format");
    parallel_master_write_command(dev, 0x3A);    // Send the Interface Pixel Format command (0x3A)
    parallel_master_write_data_byte(dev, 0x55);  // Set pixel format to 16 bits per pixel (0x55 = RGB565)
    delayMS(10);                            // Wait for 10 milliseconds after setting pixel format

    // Configure porch settings (timing for non-visible periods at the start/end of frames)
    ESP_LOGI(TAG, "Setting Porch Control");
    parallel_master_write_command(dev, 0xB2);    // Send the Porch Setting command (0xB2)
    parallel_master_write_data_byte(dev, 0x0C);  // Front porch setting
    parallel_master_write_data_byte(dev, 0x0C);  // Back porch setting
    parallel_master_write_data_byte(dev, 0x00);  // Shift line setting
    parallel_master_write_data_byte(dev, 0x33);  // Additional porch settings
    parallel_master_write_data_byte(dev, 0x33);  // Additional porch settings
    delayMS(10);                            // Wait after setting porch control

    // Set the VCOM voltage (controls display flickering and overall brightness)
    ESP_LOGI(TAG, "Setting VCOM");
    parallel_master_write_command(dev, 0xBB);    // Send the VCOM Setting command (0xBB)
    parallel_master_write_data_byte(dev, 0x35);  // Set VCOM voltage level
    delayMS(10);

    // Configure the LCD control settings (various display parameters)
    ESP_LOGI(TAG, "Setting LCM Control");
    parallel_master_write_command(dev, 0xC0);    // Send the LCM Control command (0xC0)
    parallel_master_write_data_byte(dev, 0x2C);  // Set default LCM control setting
    delayMS(10);

    // Enable writing to VDV and VRH registers (voltage settings)
    ESP_LOGI(TAG, "Enabling VDV and VRH Commands");
    parallel_master_write_command(dev, 0xC2);    // VDV and VRH Command Enable (0xC2)
    parallel_master_write_data_byte(dev, 0x01);  // Enable command writing to VDV and VRH
    delayMS(10);

    // Set VRH voltage
    ESP_LOGI(TAG, "Setting VRH");
    parallel_master_write_command(dev, 0xC3);    // VRH Set command (0xC3)
    parallel_master_write_data_byte(dev, 0x12);  // VRH voltage level
    delayMS(10);

    // Set VDV voltage
    ESP_LOGI(TAG, "Setting VDV");
    parallel_master_write_command(dev, 0xC4);    // VDV Set command (0xC4)
    parallel_master_write_data_byte(dev, 0x20);  // VDV voltage level
    delayMS(10);

    // Set the frame rate to 60Hz
    ESP_LOGI(TAG, "Setting Frame Rate Control");
    parallel_master_write_command(dev, 0xC6);    // Frame Rate Control command (0xC6)
    parallel_master_write_data_byte(dev, 0x0F);  // 60Hz
    delayMS(10);

    // Configure power settings (power control for internal circuits)
    ESP_LOGI(TAG, "Setting Power Control 1");
    parallel_master_write_command(dev, 0xD0);    // Power Control 1 (0xD0)
    parallel_master_write_data_byte(dev, 0xA4);  // default setting
    parallel_master_write_data_byte(dev, 0xA1);  // default setting
    delayMS(10);

    // Setting Positive Voltage Gamma Control
    ESP_LOGI(TAG, "Setting Positive Voltage Gamma Control");
    parallel_master_write_command(dev, 0xE0);    // Positive Gamma Control
    uint8_t positive_gamma[] = {
        0xCF, 0x07, 0x10, 0x07, 0x0B, 0x14,
        0x38, 0x32, 0x4F, 0x35, 0x12, 0x13,
        0x28, 0x2C
    };
    for (int i = 0; i < (int)sizeof(positive_gamma); i++) {
        parallel_master_write_data_byte(dev, positive_gamma[i]);
    }
    delayMS(10);

    // Setting Negative Voltage Gamma Control
    ESP_LOGI(TAG, "Setting Negative Voltage Gamma Control");
    parallel_master_write_command(dev, 0xE1);    // Negative Gamma Control
    uint8_t negative_gamma[] = {
        0xCF, 0x07, 0x0F, 0x07, 0x05, 0x05,
        0x38, 0x43, 0x50, 0x0A, 0x15, 0x13,
        0x2E, 0x30
    };
    for (int i = 0; i < (int)sizeof(negative_gamma); i++) {
        parallel_master_write_data_byte(dev, negative_gamma[i]);
    }
    delayMS(10);

    // Conditionally Enable display inversion for better color reproduction (inverts display colors)
#if CONFIG_INVERSION
    ESP_LOGI(TAG, "Enabling Display Inversion");
    parallel_master_write_command(dev, 0x21);    // INVON: Display Inversion On
    delayMS(10);
#endif

    // Set the display to normal mode
    ESP_LOGI(TAG, "Setting Normal Display Mode");
    parallel_master_write_command(dev, 0x13);    // Normal Display Mode On (0x13)
    delayMS(10);

    // Turn on the display
    ESP_LOGI(TAG, "Turning Display On");
    parallel_master_write_command(dev, 0x29);    // Display On (0x29)
    delayMS(120);

    // Turn on the backlight if defined
    if (dev->_bl >= 0) {
        gpio_set_level(dev->_bl, 1);
        ESP_LOGI(TAG, "Backlight turned on via lcdInit");
    }
}

// Draw pixel
// x:X coordinate
// y:Y coordinate
// color:color
void lcdDrawPixel(TFT_t *dev, uint16_t x, uint16_t y, uint16_t color) {
    if (x >= dev->_width || y >= dev->_height)
        return;

    uint16_t _x = x + dev->_offsetx;
    uint16_t _y = y + dev->_offsety;

    parallel_master_write_command(dev, 0x2A); // Column Address Set
    parallel_master_write_addr(dev, _x, _x);

    parallel_master_write_command(dev, 0x2B); // Row Address Set
    parallel_master_write_addr(dev, _y, _y);

    parallel_master_write_command(dev, 0x2C); // Memory Write
    parallel_master_write_data_word(dev, color);
}

// Draw multi pixel
// x:X coordinate
// y:Y coordinate
// size:Number of colors
// colors:colors
void lcdDrawMultiPixels(TFT_t *dev, uint16_t x, uint16_t y, uint16_t size, uint16_t *colors) {
    if (x + size > dev->_width)
        return;
    if (y >= dev->_height)
        return;

    uint16_t _x1 = x + dev->_offsetx;
    uint16_t _x2 = _x1 + (size - 1);
    uint16_t _y1 = y + dev->_offsety;
    uint16_t _y2 = _y1;

    parallel_master_write_command(dev, 0x2A);  // CASET
    parallel_master_write_addr(dev, _x1, _x2);
    parallel_master_write_command(dev, 0x2B);  // RASET
    parallel_master_write_addr(dev, _y1, _y2);
    parallel_master_write_command(dev, 0x2C);  // RAMWR
    parallel_master_write_colors(dev, colors, size);
}

// Draw rectangle of filling
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End X coordinate
// y2:End Y coordinate
// color:color
void lcdDrawFillRect(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    if (x1 >= dev->_width)
        return;
    if (x2 >= dev->_width)
        x2 = dev->_width - 1;
    if (y1 >= dev->_height)
        return;
    if (y2 >= dev->_height)
        y2 = dev->_height - 1;

    uint16_t _x1 = x1 + dev->_offsetx;
    uint16_t _x2 = x2 + dev->_offsetx;
    uint16_t _y1 = y1 + dev->_offsety;
    uint16_t _y2 = y2 + dev->_offsety;

    parallel_master_write_command(dev, 0x2A);  // CASET
    parallel_master_write_addr(dev, _x1, _x2);
    parallel_master_write_command(dev, 0x2B);  // RASET
    parallel_master_write_addr(dev, _y1, _y2);
    parallel_master_write_command(dev, 0x2C);  // RAMWR

    uint32_t size = (_x2 - _x1 + 1) * (_y2 - _y1 + 1);
    parallel_master_write_color(dev, color, size);
}

// Draw a bitmap to the display
// x: X coordinate (start position)
// y: Y coordinate (start position)
// w: Width of the bitmap
// h: Height of the bitmap
// data: Pointer to the bitmap data (RGB565 format)
// For lcdDrawBitmap, previously used asynchronous SPI calls.
// Now we send data in parallel directly.
void lcdDrawBitmap(TFT_t *dev, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *data) {
    if ((x + w) > dev->_width || (y + h) > dev->_height)
        return;

    uint16_t x1 = x + dev->_offsetx;
    uint16_t y1 = y + dev->_offsety;
    uint16_t x2 = x1 + w - 1;
    uint16_t y2 = y1 + h - 1;

    // No changes to existing comments or logic
    ESP_LOGD(TAG, "lcdDrawBitmap x=%u y=%u w=%u h=%u", x, y, w, h);

    parallel_master_write_command(dev, 0x2A);
    parallel_master_write_addr(dev, x1, x2);

    parallel_master_write_command(dev, 0x2B);
    parallel_master_write_addr(dev, y1, y2);

    parallel_master_write_command(dev, 0x2C);

    gpio_set_level(dev->_dc, 1);
    for (uint32_t i = 0; i < (w * h); i++) {
        uint16_t color = data[i];
        uint8_t high = (color >> 8) & 0xFF;
        uint8_t low = color & 0xFF;
        // Changed %u to %" PRIu32 " to correctly format uint32_t 'i'
        ESP_LOGD(TAG, "Pixel[%" PRIu32 "]=0x%04X (H=0x%02X, L=0x%02X)", i, color, high, low);

        parallel_set_data_pins(high);
        parallel_pulse_wr(dev->_wr);
        parallel_set_data_pins(low);
        parallel_pulse_wr(dev->_wr);
    }
}


// Draw a small rectangle bitmap (optimized for small regions)
// x: X coordinate (start)
// y: Y coordinate (start)
// w: Width
// h: Height
// data: Pixel data (RGB565)
void lcdDrawBitmapRect(TFT_t *dev, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *data) {
    if ((x + w) > dev->_width || (y + h) > dev->_height)
        return;

    uint16_t x1 = x + dev->_offsetx;
    uint16_t y1 = y + dev->_offsety;
    uint16_t x2 = x1 + w - 1;
    uint16_t y2 = y1 + h - 1;

    parallel_master_write_command(dev, 0x2A);
    parallel_master_write_addr(dev, x1, x2);

    parallel_master_write_command(dev, 0x2B);
    parallel_master_write_addr(dev, y1, y2);

    parallel_master_write_command(dev, 0x2C);

    gpio_set_level(dev->_dc, 1);
    uint32_t data_size = w * h;
    for (uint32_t i = 0; i < data_size; i++) {
        uint16_t color = data[i];
        uint8_t high = (color >> 8) & 0xFF;
        uint8_t low = color & 0xFF;
        // Changed %u to %" PRIu32 " to correctly format uint32_t 'i'
        ESP_LOGD(TAG, "Rect Pixel[%" PRIu32 "]=0x%04X (H=0x%02X, L=0x%02X)", i, color, high, low);

        parallel_set_data_pins(high);
        parallel_pulse_wr(dev->_wr);
        parallel_set_data_pins(low);
        parallel_pulse_wr(dev->_wr);
    }
}


// Display OFF
void lcdDisplayOff(TFT_t *dev) {
    ESP_LOGI(TAG, "Display OFF");
    parallel_master_write_command(dev, 0x28);
}

// Display ON
void lcdDisplayOn(TFT_t *dev) {
    ESP_LOGI(TAG, "Display ON");
    parallel_master_write_command(dev, 0x29);
}

// Fill screen
// color:color
void lcdFillScreen(TFT_t *dev, uint16_t color) {
    lcdDrawFillRect(dev, 0, 0, dev->_width - 1, dev->_height - 1, color);
}

// Draw line
// x1:Start X
// y1:Start Y
// x2:End X
// y2:End Y
// color:color
void lcdDrawLine(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    int dx, dy, sx, sy, err, e2;

    dx = abs(x2 - x1);
    dy = -abs(y2 - y1);

    sx = (x1 < x2) ? 1 : -1;
    sy = (y1 < y2) ? 1 : -1;

    err = dx + dy;

    while (1) {
        lcdDrawPixel(dev, x1, y1, color);
        if (x1 == x2 && y1 == y2)
            break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x1 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y1 += sy;
        }
    }
}

// Draw rectangle
// x1:Start X
// y1:Start Y
// x2:End X
// y2:End Y
// color:color
void lcdDrawRect(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    lcdDrawLine(dev, x1, y1, x2, y1, color);
    lcdDrawLine(dev, x2, y1, x2, y2, color);
    lcdDrawLine(dev, x2, y2, x1, y2, color);
    lcdDrawLine(dev, x1, y2, x1, y1, color);
}

// Draw rectangle with angle
// xc:Center X
// yc:Center Y
// w:Width
// h:Height
// angle:Angle
// color:color
void lcdDrawRectAngle(TFT_t *dev, uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint16_t color) {
    double xd, yd, rd;
    int x1, y1;
    int x2, y2;
    int x3, y3;
    int x4, y4;
    rd = -angle * M_PI / 180.0;
    xd = 0.0 - w / 2;
    yd = h / 2;
    x1 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
    y1 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

    yd = 0.0 - yd;
    x2 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
    y2 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

    xd = w / 2;
    yd = h / 2;
    x3 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
    y3 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

    yd = 0.0 - yd;
    x4 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
    y4 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

    lcdDrawLine(dev, x1, y1, x2, y2, color);
    lcdDrawLine(dev, x1, y1, x3, y3, color);
    lcdDrawLine(dev, x2, y2, x4, y4, color);
    lcdDrawLine(dev, x3, y3, x4, y4, color);
}

// Draw triangle
// xc:Center X
// yc:Center Y
// w:Width
// h:Height
// angle:Angle
// color:color
void lcdDrawTriangle(TFT_t *dev, uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint16_t color) {
    double xd, yd, rd;
    int x1, y1;
    int x2, y2;
    int x3, y3;
    rd = -angle * M_PI / 180.0;
    xd = 0.0;
    yd = h / 2;
    x1 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
    y1 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

    xd = w / 2;
    yd = 0.0 - yd;
    x2 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
    y2 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

    xd = 0.0 - w / 2;
    x3 = (int)(xd * cos(rd) - yd * sin(rd) + xc);
    y3 = (int)(xd * sin(rd) + yd * cos(rd) + yc);

    lcdDrawLine(dev, x1, y1, x2, y2, color);
    lcdDrawLine(dev, x1, y1, x3, y3, color);
    lcdDrawLine(dev, x2, y2, x3, y3, color);
}

// Draw circle
// x0:Center X
// y0:Center Y
// r:radius
// color:color
void lcdDrawCircle(TFT_t *dev, uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
    int x;
    int y;
    int err;
    int old_err;

    x = 0;
    y = -r;
    err = 2 - 2 * r;
    do {
        lcdDrawPixel(dev, x0 - x, y0 + y, color);
        lcdDrawPixel(dev, x0 - y, y0 - x, color);
        lcdDrawPixel(dev, x0 + x, y0 - y, color);
        lcdDrawPixel(dev, x0 + y, y0 + x, color);
        if ((old_err = err) <= x)
            err += ++x * 2 + 1;
        if (old_err > y || err > x)
            err += ++y * 2 + 1;
    } while (y < 0);
}

// Draw filled circle
// x0:Center X
// y0:Center Y
// r:radius
// color:color
void lcdDrawFillCircle(TFT_t *dev, uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
    int x;
    int y;
    int err;
    int old_err;
    int ChangeX;

    x = 0;
    y = -r;
    err = 2 - 2 * r;
    ChangeX = 1;
    do {
        if (ChangeX) {
            lcdDrawLine(dev, x0 - x, y0 - y, x0 - x, y0 + y, color);
            lcdDrawLine(dev, x0 + x, y0 - y, x0 + x, y0 + y, color);
        }
        ChangeX = (old_err = err) <= x;
        if (ChangeX)
            err += ++x * 2 + 1;
        if (old_err > y || err > x)
            err += ++y * 2 + 1;
    } while (y <= 0);
}

// Draw rectangle with rounded corners
// x1:Start X
// y1:Start Y
// x2:End X
// y2:End Y
// r:radius
// color:color
void lcdDrawRoundRect(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t r, uint16_t color) {
    int x;
    int y;
    int err;
    int old_err;
    uint16_t temp;

    if (x1 > x2) {
        temp = x1;
        x1 = x2;
        x2 = temp;
    }

    if (y1 > y2) {
        temp = y1;
        y1 = y2;
        y2 = temp;
    }

    if (x2 - x1 < r)
        return;
    if (y2 - y1 < r)
        return;

    x = 0;
    y = -r;
    err = 2 - 2 * r;

    do {
        if (x) {
            lcdDrawPixel(dev, x1 + r - x, y1 + r + y, color);
            lcdDrawPixel(dev, x2 - r + x, y1 + r + y, color);
            lcdDrawPixel(dev, x1 + r - x, y2 - r - y, color);
            lcdDrawPixel(dev, x2 - r + x, y2 - r - y, color);
        }
        if ((old_err = err) <= x)
            err += ++x * 2 + 1;
        if (old_err > y || err > x)
            err += ++y * 2 + 1;
    } while (y < 0);

    lcdDrawLine(dev, x1 + r, y1, x2 - r, y1, color);
    lcdDrawLine(dev, x1 + r, y2, x2 - r, y2, color);
    lcdDrawLine(dev, x1, y1 + r, x1, y2 - r, color);
    lcdDrawLine(dev, x2, y1 + r, x2, y2 - r, color);
}

// Draw arrow
// x1:Start X
// y1:Start Y
// x2:End X
// y2:End Y
// w:Width of the bottom
// color:color
void lcdDrawArrow(TFT_t *dev, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t w, uint16_t color) {
    double Vx = x1 - x0;
    double Vy = y1 - y0;
    double v = sqrt(Vx * Vx + Vy * Vy);
    double Ux = Vx / v;
    double Uy = Vy / v;

    uint16_t L[2], R[2];
    L[0] = x1 - Uy * w - Ux * v;
    L[1] = y1 + Ux * w - Uy * v;
    R[0] = x1 + Uy * w - Ux * v;
    R[1] = y1 - Ux * w - Uy * v;

    lcdDrawLine(dev, x1, y1, L[0], L[1], color);
    lcdDrawLine(dev, x1, y1, R[0], R[1], color);
    lcdDrawLine(dev, L[0], L[1], R[0], R[1], color);
}

// Draw arrow of filling
// x1:Start X
// y1:Start Y
// x2:End X
// y2:End Y
// w:Width of the bottom
// color:color
void lcdDrawFillArrow(TFT_t *dev, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t w, uint16_t color) {
    double Vx = x1 - x0;
    double Vy = y1 - y0;
    double v = sqrt(Vx * Vx + Vy * Vy);
    double Ux = Vx / v;
    double Uy = Vy / v;

    uint16_t L[2], R[2];
    L[0] = x1 - Uy * w - Ux * v;
    L[1] = y1 + Ux * w - Uy * v;
    R[0] = x1 + Uy * w - Ux * v;
    R[1] = y1 - Ux * w - Uy * v;

    lcdDrawLine(dev, x0, y0, x1, y1, color);
    lcdDrawLine(dev, x1, y1, L[0], L[1], color);
    lcdDrawLine(dev, x1, y1, R[0], R[1], color);
    lcdDrawLine(dev, L[0], L[1], R[0], R[1], color);

    for (int ww = w - 1; ww > 0; ww--) {
        L[0] = x1 - Uy * ww - Ux * v;
        L[1] = y1 + Ux * ww - Uy * v;
        R[0] = x1 + Uy * ww - Ux * v;
        R[1] = y1 - Ux * ww - Uy * v;
        lcdDrawLine(dev, x1, y1, L[0], L[1], color);
        lcdDrawLine(dev, x1, y1, R[0], R[1], color);
    }
}

// Implementation of rgb565_conv_with_color_tweaks
uint16_t rgb565_conv_with_color_tweaks(
    uint16_t r, uint16_t g, uint16_t b
) {
    float luminance = 0.2126f * r + 0.7152f * g + 0.0722f * b;
    const float shadows_threshold = 85.0f;
    const float highlights_threshold = 170.0f;

    int adjust_cyan_red = 0;
    int adjust_magenta_green = 0;
    int adjust_yellow_blue = 0;

    if (luminance <= shadows_threshold) {
        adjust_cyan_red = color_tweaks.shadows_cyan_red;
        adjust_magenta_green = color_tweaks.shadows_magenta_green;
        adjust_yellow_blue = color_tweaks.shadows_yellow_blue;
    } else if (luminance >= highlights_threshold) {
        adjust_cyan_red = color_tweaks.highlights_cyan_red;
        adjust_magenta_green = color_tweaks.highlights_magenta_green;
        adjust_yellow_blue = color_tweaks.highlights_yellow_blue;
    } else {
        adjust_cyan_red = color_tweaks.midtones_cyan_red;
        adjust_magenta_green = color_tweaks.midtones_magenta_green;
        adjust_yellow_blue = color_tweaks.midtones_yellow_blue;
    }

    float rf = r * (1.0f + adjust_cyan_red / 100.0f);
    float gf = g * (1.0f + adjust_magenta_green / 100.0f);
    float bf = b * (1.0f + adjust_yellow_blue / 100.0f);

    rf = fminf(fmaxf(rf, 0.0f), 255.0f);
    gf = fminf(fmaxf(gf, 0.0f), 255.0f);
    bf = fminf(fmaxf(bf, 0.0f), 255.0f);

    float brightness_factor = brightness_percent / 100.0f * 255.0f;
    rf = rf + brightness_factor;
    gf = gf + brightness_factor;
    bf = bf + brightness_factor;

    rf = fminf(fmaxf(rf, 0.0f), 255.0f);
    gf = fminf(fmaxf(gf, 0.0f), 255.0f);
    bf = fminf(fmaxf(bf, 0.0f), 255.0f);

    float contrast_factor = (100.0f + contrast_percent) / 100.0f;
    float midpoint = 128.0f;

    rf = (rf - midpoint) * contrast_factor + midpoint;
    gf = (gf - midpoint) * contrast_factor + midpoint;
    bf = (bf - midpoint) * contrast_factor + midpoint;

    rf = fminf(fmaxf(rf, 0.0f), 255.0f);
    gf = fminf(fmaxf(gf, 0.0f), 255.0f);
    bf = fminf(fmaxf(bf, 0.0f), 255.0f);

    rf = powf(rf / 255.0f, gamma_red) * 255.0f;
    gf = powf(gf / 255.0f, gamma_green) * 255.0f;
    bf = powf(bf / 255.0f, gamma_blue) * 255.0f;

    uint16_t ri = (uint16_t)(rf + 0.5f);
    uint16_t gi = (uint16_t)(gf + 0.5f);
    uint16_t bi = (uint16_t)(bf + 0.5f);

    return ((ri & 0xF8) << 8) | ((gi & 0xFC) << 3) | (bi >> 3);
}

// Draw ASCII character
// x:X coordinate
// y:Y coordinate
// ascii: ascii code
// color:color
int lcdDrawChar(TFT_t * dev, FontxFile *fxs, uint16_t x, uint16_t y, uint8_t ascii, uint16_t color) {
    uint16_t xx, yy, bit, ofs;
    unsigned char fonts[128]; // font pattern
    unsigned char pw, ph;
    int h, w;
    uint16_t mask;
    bool rc;

    if(_DEBUG_)printf("_font_direction=%d\n", dev->_font_direction);
    rc = GetFontx(fxs, ascii, fonts, &pw, &ph);
    if(_DEBUG_)printf("GetFontx rc=%d pw=%d ph=%d\n", rc, pw, ph);
    if (!rc) return 0;

    int16_t xd1 = 0;
    int16_t yd1 = 0;
    int16_t xd2 = 0;
    uint16_t xss = 0;
    uint16_t yss = 0;
    int16_t xsd = 0;
    int16_t ysd = 0;
    int16_t next = 0;
    uint16_t x0  = 0;
    uint16_t y0  = 0;
    uint16_t x1  = 0;
    uint16_t y1_ = 0;
    if (dev->_font_direction == DIRECTION0) {
        xd1 = +1;
        yd1 = +1; 
        xd2 =  0;
        yss =  y - (ph - 1);
        xsd =  1;
        ysd =  0;
        next = x + pw;

        x0  = x;
        y0  = y - (ph-1);
        x1  = x + (pw-1);
        y1_ = y;
    } else if (dev->_font_direction == DIRECTION180) {
        xd1 = -1;
        yd1 = -1; 
        xd2 =  0;
        yss =  y + ph + 1;
        xsd =  1;
        ysd =  0;
        next = x - pw;

        x0  = x - (pw-1);
        y0  = y;
        x1  = x;
        y1_ = y + (ph-1);
    } else if (dev->_font_direction == DIRECTION90) {
        xd1 =  0;
        yd1 =  0;
        xd2 = -1;
        ysd =  1; 
        xss =  x + ph;
        yss =  y;
        xsd =  0;
        next = y + pw;

        x0  = x;
        y0  = y;
        x1  = x + (ph-1);
        y1_ = y + (pw-1);
    } else if (dev->_font_direction == DIRECTION270) {
        xd1 =  0;
        yd1 =  0;
        xd2 = +1;
        ysd =  -1; 
        xss =  x - (ph - 1);
        yss =  y;
        xsd =  0;
        next = y - pw;

        x0  = x - (ph-1);
        y0  = y - (pw-1);
        x1  = x;
        y1_ = y;
    }

    if (dev->_font_fill) {
        lcdDrawFillRect(dev, x0, y0, x1, y1_, dev->_font_fill_color);
        ESP_LOGD(TAG, "Filling font background: x0=%d y0=%d x1=%d y1=%d color=0x%04X", x0, y0, x1, y1_, dev->_font_fill_color);
    }

    int bits;
    if(_DEBUG_)printf("xss=%d yss=%d\n",xss,yss);
    ofs = 0;
    yy = yss;
    xx = xss;
    for(h = 0; h < ph; h++) {
        if(xsd) xx = xss;
        if(ysd) yy = yss;
        bits = pw;
        for(w = 0; w < ((pw + 4) / 8); w++) {
            mask = 0x80;
            for(bit = 0; bit < 8; bit++) {
                bits--;
                if (bits < 0) continue;
                if (fonts[ofs] & mask) {
                    lcdDrawPixel(dev, xx, yy, color);
                }
                if (h == (ph-2) && dev->_font_underline)
                    lcdDrawPixel(dev, xx, yy, dev->_font_underline_color);
                if (h == (ph-1) && dev->_font_underline)
                    lcdDrawPixel(dev, xx, yy, dev->_font_underline_color);
                xx = xx + xd1;
                yy = yy + ysd;
                mask = mask >> 1;
            }
            ofs++;
        }
        yy = yy + yd1;
        xx = xx + xd2;
    }

    if (next < 0) next = 0;
    return next;
}

int lcdDrawString(TFT_t * dev, FontxFile *fx, uint16_t x, uint16_t y, uint8_t * ascii, uint16_t color) {
    int length = strlen((char *)ascii);
    if(_DEBUG_)printf("lcdDrawString length=%d\n",length);
    ESP_LOGD(TAG, "Drawing string: \"%s\" at x=%d y=%d color=0x%04X", ascii, x, y, color);
    for(int i=0;i<length;i++) {
        if(_DEBUG_)printf("ascii[%d]=%x x=%d y=%d\n",i,ascii[i],x,y);
        if (dev->_font_direction == 0)
            x = lcdDrawChar(dev, fx, x, y, ascii[i], color);
        if (dev->_font_direction == 1)
            y = lcdDrawChar(dev, fx, x, y, ascii[i], color);
        if (dev->_font_direction == 2)
            x = lcdDrawChar(dev, fx, x, y, ascii[i], color);
        if (dev->_font_direction == 3)
            y = lcdDrawChar(dev, fx, x, y, ascii[i], color);
    }
    if (dev->_font_direction == 0) return x;
    if (dev->_font_direction == 2) return x;
    if (dev->_font_direction == 1) return y;
    if (dev->_font_direction == 3) return y;
    return 0;
}

// Draw Non-Alphanumeric character
// x:X coordinate
// y:Y coordinate
// code:character code
// color:color
int lcdDrawCode(TFT_t * dev, FontxFile *fx, uint16_t x,uint16_t y,uint8_t code,uint16_t color) {
    if(_DEBUG_)printf("code=%x x=%d y=%d\n",code,x,y);
    if (dev->_font_direction == 0)
        x = lcdDrawChar(dev, fx, x, y, code, color);
    if (dev->_font_direction == 1)
        y = lcdDrawChar(dev, fx, x, y, code, color);
    if (dev->_font_direction == 2)
        x = lcdDrawChar(dev, fx, x, y, code, color);
    if (dev->_font_direction == 3)
        y = lcdDrawChar(dev, fx, x, y, code, color);
    if (dev->_font_direction == 0) return x;
    if (dev->_font_direction == 2) return x;
    if (dev->_font_direction == 1) return y;
    if (dev->_font_direction == 3) return y;
    return 0;
}

// Set font direction
// dir:Direction
void lcdSetFontDirection(TFT_t *dev, uint16_t dir) {
    dev->_font_direction = dir;
}

// Set font filling
// color:fill color
void lcdSetFontFill(TFT_t *dev, uint16_t color) {
    dev->_font_fill = true;
    dev->_font_fill_color = color;
}

// UnSet font filling
void lcdUnsetFontFill(TFT_t *dev) {
    dev->_font_fill = false;
}

// Set font underline
// color:frame color
void lcdSetFontUnderLine(TFT_t *dev, uint16_t color) {
    dev->_font_underline = true;
    dev->_font_underline_color = color;
}

// UnSet font underline
void lcdUnsetFontUnderLine(TFT_t *dev) {
    dev->_font_underline = false;
}

// Backlight ON
void lcdBacklightOn(TFT_t *dev) {
    if (dev->_bl >= 0) {
        gpio_set_level(dev->_bl, 1);  
        ESP_LOGI(TAG, "Backlight turned on via lcdBacklightOn");
    }
}

// Backlight OFF
void lcdBacklightOff(TFT_t *dev) {
    if (dev->_bl >= 0) {
        gpio_set_level(dev->_bl, 0);  
        ESP_LOGI(TAG, "Backlight turned off via lcdBacklightOff");
    }
}

// Display Inversion Off
void lcdInversionOff(TFT_t *dev) {
    ESP_LOGI(TAG, "Disabling Display Inversion");
    parallel_master_write_command(dev, 0x20);  // INVOFF
}

// Display Inversion On
void lcdInversionOn(TFT_t *dev) {
    ESP_LOGI(TAG, "Enabling Display Inversion");
    parallel_master_write_command(dev, 0x21);  // INVON
}
