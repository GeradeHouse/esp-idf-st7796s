// main/st7796s.c

/*
 * st7796s.c
 * 
 * ESP-IDF driver for the ST7796S 3.5 Inch 320x480 SPI TFT display.
 * 
 * This driver provides functions to initialize, configure, and control the ST7796S LCD display,
 * including:
 * - Display initialization and configuration
 * - SPI communication handling
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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_log.h"

#include "st7796s.h"  // Updated to the correct header file
#include "sdkconfig.h" // Include configuration header

#define TAG "ST7796S"
#define _DEBUG_ 0

#if CONFIG_SPI2_HOST
#define HOST_ID SPI2_HOST
#elif CONFIG_SPI3_HOST
#define HOST_ID SPI3_HOST
#else
#define HOST_ID SPI2_HOST // Default to SPI2_HOST if not defined
#endif

static const int SPI_Command_Mode = 0;
static const int SPI_Data_Mode = 1;
static const int SPI_Frequency = SPI_MASTER_FREQ_80M;

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

void spi_master_init(TFT_t *dev, int16_t GPIO_MOSI, int16_t GPIO_SCLK, int16_t GPIO_CS,
                     int16_t GPIO_DC, int16_t GPIO_RESET, int16_t GPIO_BL) {
    esp_err_t ret;

    // Initialize CS pin
    ESP_LOGI(TAG, "Initializing GPIO_CS=%d", GPIO_CS);
    if (GPIO_CS >= 0) {
        gpio_reset_pin(GPIO_CS);
        gpio_set_direction(GPIO_CS, GPIO_MODE_OUTPUT);
        gpio_set_level(GPIO_CS, 1);
        ESP_LOGI(TAG, "GPIO_CS=%d set to HIGH", GPIO_CS);
    } else {
        ESP_LOGW(TAG, "GPIO_CS not defined (value: %d)", GPIO_CS);
    }

    // Initialize DC pin
    ESP_LOGI(TAG, "Initializing GPIO_DC=%d", GPIO_DC);
    gpio_reset_pin(GPIO_DC);
    gpio_set_direction(GPIO_DC, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_DC, 0);
    ESP_LOGI(TAG, "GPIO_DC=%d set to LOW", GPIO_DC);

    // Initialize RESET pin
    ESP_LOGI(TAG, "Initializing GPIO_RESET=%d", GPIO_RESET);
    if (GPIO_RESET >= 0) {
        gpio_reset_pin(GPIO_RESET);
        gpio_set_direction(GPIO_RESET, GPIO_MODE_OUTPUT);
        gpio_set_level(GPIO_RESET, 1);  // Ensure the reset pin starts high
        ESP_LOGI(TAG, "GPIO_RESET=%d set to HIGH", GPIO_RESET);
        delayMS(100);
        gpio_set_level(GPIO_RESET, 0);  // Pulse reset pin
        ESP_LOGI(TAG, "GPIO_RESET=%d set to LOW (pulse)", GPIO_RESET);
        delayMS(100);
        gpio_set_level(GPIO_RESET, 1);  // Set back to high
        ESP_LOGI(TAG, "GPIO_RESET=%d set to HIGH (release)", GPIO_RESET);
        delayMS(100);
    } else {
        ESP_LOGW(TAG, "GPIO_RESET not defined (value: %d)", GPIO_RESET);
    }

    // Initialize SPI bus
    ESP_LOGI(TAG, "Initializing SPI bus with GPIO_MOSI=%d, GPIO_SCLK=%d", GPIO_MOSI, GPIO_SCLK);
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 307200, // Set to accommodate large transfers if needed
        .flags = SPICOMMON_BUSFLAG_MASTER,
    };

    ESP_LOGI(TAG, "SPI Bus Configuration: MOSI=%d, SCLK=%d, Max Transfer Size=%d bytes",
             buscfg.mosi_io_num, buscfg.sclk_io_num, buscfg.max_transfer_sz);

    // Use DMA channel
    ret = spi_bus_initialize(HOST_ID, &buscfg, SPI_DMA_CH_AUTO); // Use automatic DMA channel allocation
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "spi_bus_initialize successful");

    spi_device_interface_config_t devcfg;
    memset(&devcfg, 0, sizeof(devcfg));
    devcfg.clock_speed_hz = SPI_Frequency;
    devcfg.queue_size = 7;
    devcfg.mode = 0;  // SPI mode 0
    devcfg.flags = SPI_DEVICE_NO_DUMMY;

    if (GPIO_CS >= 0) {
        devcfg.spics_io_num = GPIO_CS;
    } else {
        devcfg.spics_io_num = -1;
    }

    ESP_LOGI(TAG, "Adding SPI device to bus");
    spi_device_handle_t handle;
    ret = spi_bus_add_device(HOST_ID, &devcfg, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "spi_bus_add_device successful, handle=%p", (void *)handle);

    // Store GPIO numbers and SPI handle in device structure
    dev->_dc = GPIO_DC;
    dev->_bl = GPIO_BL;
    dev->_reset = GPIO_RESET;
    dev->_SPIHandle = handle;

    // Initialize Backlight pin AFTER SPI initialization
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
}

bool spi_master_write_byte(spi_device_handle_t SPIHandle, const uint8_t *Data, size_t DataLength) {
    spi_transaction_t SPITransaction;
    esp_err_t ret;

    if (DataLength > 0) {
        memset(&SPITransaction, 0, sizeof(spi_transaction_t));
        SPITransaction.length = DataLength * 8;
        SPITransaction.tx_buffer = Data;

        ESP_LOGD(TAG, "Sending SPI byte data: Length=%zu bytes", DataLength);
        // Use asynchronous API
        ret = spi_device_queue_trans(SPIHandle, &SPITransaction, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "spi_device_queue_trans failed: %s", esp_err_to_name(ret));
            return false;
        }

        // Wait for transaction to complete
        spi_transaction_t *rtrans;
        ret = spi_device_get_trans_result(SPIHandle, &rtrans, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "spi_device_get_trans_result failed: %s", esp_err_to_name(ret));
            return false;
        }
        ESP_LOGD(TAG, "SPI byte data transmitted successfully");
    } else {
        ESP_LOGW(TAG, "spi_master_write_byte called with DataLength=0");
    }

    return true;
}

bool spi_master_write_command(TFT_t *dev, uint8_t cmd) {
    uint8_t Byte = cmd;
    gpio_set_level(dev->_dc, SPI_Command_Mode);
    return spi_master_write_byte(dev->_SPIHandle, &Byte, 1);
}

bool spi_master_write_data_byte(TFT_t *dev, uint8_t data) {
    uint8_t Byte = data;
    gpio_set_level(dev->_dc, SPI_Data_Mode);
    return spi_master_write_byte(dev->_SPIHandle, &Byte, 1);
}

bool spi_master_write_data_word(TFT_t *dev, uint16_t data) {
    uint8_t Byte[2];
    Byte[0] = (data >> 8) & 0xFF;
    Byte[1] = data & 0xFF;
    gpio_set_level(dev->_dc, SPI_Data_Mode);
    return spi_master_write_byte(dev->_SPIHandle, Byte, 2);
}

bool spi_master_write_addr(TFT_t *dev, uint16_t addr1, uint16_t addr2) {
    uint8_t Byte[4];
    Byte[0] = (addr1 >> 8) & 0xFF;
    Byte[1] = addr1 & 0xFF;
    Byte[2] = (addr2 >> 8) & 0xFF;
    Byte[3] = addr2 & 0xFF;
    gpio_set_level(dev->_dc, SPI_Data_Mode);
    return spi_master_write_byte(dev->_SPIHandle, Byte, 4);
}

bool spi_master_write_color(TFT_t *dev, uint16_t color, uint32_t size) {
    uint8_t Byte[1024];
    uint32_t len = size * 2;
    uint32_t max_chunk = sizeof(Byte);

    while (len > 0) {
        uint32_t chunk_size = (len > max_chunk) ? max_chunk : len;
        uint32_t color_count = chunk_size / 2;
        for (uint32_t i = 0; i < color_count; i++) {
            Byte[i * 2] = (color >> 8) & 0xFF;
            Byte[i * 2 + 1] = color & 0xFF;
        }
        gpio_set_level(dev->_dc, SPI_Data_Mode);

        spi_transaction_t SPITransaction;
        memset(&SPITransaction, 0, sizeof(spi_transaction_t));
        SPITransaction.length = chunk_size * 8;
        SPITransaction.tx_buffer = Byte;

        esp_err_t ret = spi_device_queue_trans(dev->_SPIHandle, &SPITransaction, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "spi_master_write_color: spi_device_queue_trans failed: %s", esp_err_to_name(ret));
            return false;
        }

        spi_transaction_t *rtrans;
        ret = spi_device_get_trans_result(dev->_SPIHandle, &rtrans, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "spi_master_write_color: spi_device_get_trans_result failed: %s", esp_err_to_name(ret));
            return false;
        }

        len -= chunk_size;
    }
    return true;
}

bool spi_master_write_colors(TFT_t *dev, uint16_t *colors, uint16_t size) {
    uint8_t Byte[1024];
    uint32_t index = 0;
    uint32_t len = size * 2;
    uint32_t max_chunk = sizeof(Byte);

    while (len > 0) {
        uint32_t chunk_size = (len > max_chunk) ? max_chunk : len;
        uint32_t color_count = chunk_size / 2;
        for (uint32_t i = 0; i < color_count; i++) {
            Byte[i * 2] = (colors[index] >> 8) & 0xFF;
            Byte[i * 2 + 1] = colors[index] & 0xFF;
            index++;
        }
        gpio_set_level(dev->_dc, SPI_Data_Mode);

        spi_transaction_t SPITransaction;
        memset(&SPITransaction, 0, sizeof(spi_transaction_t));
        SPITransaction.length = chunk_size * 8;
        SPITransaction.tx_buffer = Byte;

        esp_err_t ret = spi_device_queue_trans(dev->_SPIHandle, &SPITransaction, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "spi_master_write_colors: spi_device_queue_trans failed: %s", esp_err_to_name(ret));
            return false;
        }

        spi_transaction_t *rtrans;
        ret = spi_device_get_trans_result(dev->_SPIHandle, &rtrans, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "spi_master_write_colors: spi_device_get_trans_result failed: %s", esp_err_to_name(ret));
            return false;
        }

        len -= chunk_size;
    }
    return true;
}

void delayMS(int ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

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
    spi_master_write_command(dev, 0x01); // Send the Software Reset command (0x01)
    delayMS(150);                        // Wait for 150 milliseconds for the reset process to complete

    // Initialize gamma correction values
    init_gamma_values();

    // Initialize brightness and contrast values
    init_brightness_contrast_values();

    // Exit sleep mode to turn on the display
    ESP_LOGI(TAG, "Exiting Sleep Mode");
    spi_master_write_command(dev, 0x11); // Send the Sleep Out command (0x11)
    delayMS(120);                        // Wait for 120 milliseconds for the display to wake up

    // Set the memory data access control (defines the orientation of the display)
    ESP_LOGI(TAG, "Setting Memory Data Access Control");
    spi_master_write_command(dev, 0x36);    // Send the Memory Data Access Control command (0x36)
    spi_master_write_data_byte(dev, orientation); // Set the display orientation based on the provided parameter
    delayMS(10);                            // Wait for 10 milliseconds after setting orientation

    // Set the pixel format to 16 bits per pixel (RGB565 format)
    ESP_LOGI(TAG, "Setting Interface Pixel Format");
    spi_master_write_command(dev, 0x3A);    // Send the Interface Pixel Format command (0x3A)
    spi_master_write_data_byte(dev, 0x55);  // Set pixel format to 16 bits per pixel (0x55 corresponds to RGB565)
    delayMS(10);                            // Wait for 10 milliseconds after setting pixel format

    // Configure porch settings (timing for non-visible periods at the start/end of frames)
    ESP_LOGI(TAG, "Setting Porch Control");
    spi_master_write_command(dev, 0xB2);    // Send the Porch Setting command (0xB2)
    spi_master_write_data_byte(dev, 0x0C);  // Front porch setting (number of non-displayed lines at frame start)
    spi_master_write_data_byte(dev, 0x0C);  // Back porch setting (number of non-displayed lines at frame end)
    spi_master_write_data_byte(dev, 0x00);  // Shift line setting (set to 0x00 for default)
    spi_master_write_data_byte(dev, 0x33);  // Additional porch settings (fine-tuning)
    spi_master_write_data_byte(dev, 0x33);  // Additional porch settings (fine-tuning)
    delayMS(10);                            // Wait for 10 milliseconds after setting porch control

    // Set the VCOM voltage (controls display flickering and overall brightness)
    ESP_LOGI(TAG, "Setting VCOM");
    spi_master_write_command(dev, 0xBB);    // Send the VCOM Setting command (0xBB)
    spi_master_write_data_byte(dev, 0x35);  // Set VCOM voltage level (adjust as needed for your display)
    delayMS(10);                            // Wait for 10 milliseconds after setting VCOM

    // Configure the LCD control settings (various display parameters)
    ESP_LOGI(TAG, "Setting LCM Control");
    spi_master_write_command(dev, 0xC0);    // Send the LCM Control command (0xC0)
    spi_master_write_data_byte(dev, 0x2C);  // Set default LCM control setting (adjust as needed)
    delayMS(10);                            // Wait for 10 milliseconds after setting LCM control

    // Enable writing to VDV and VRH registers (voltage settings)
    ESP_LOGI(TAG, "Enabling VDV and VRH Commands");
    spi_master_write_command(dev, 0xC2);    // Send the VDV and VRH Command Enable (0xC2)
    spi_master_write_data_byte(dev, 0x01);  // Enable command writing to VDV and VRH
    delayMS(10);                            // Wait for 10 milliseconds after enabling commands

    // Set VRH voltage (adjusts the voltage of the panel)
    ESP_LOGI(TAG, "Setting VRH");
    spi_master_write_command(dev, 0xC3);    // Send the VRH Set command (0xC3)
    spi_master_write_data_byte(dev, 0x12);  // Set VRH voltage level (adjust as needed for your display)
    delayMS(10);                            // Wait for 10 milliseconds after setting VRH

    // Set VDV voltage (fine-tunes the voltage level)
    ESP_LOGI(TAG, "Setting VDV");
    spi_master_write_command(dev, 0xC4);    // Send the VDV Set command (0xC4)
    spi_master_write_data_byte(dev, 0x20);  // Set VDV voltage level (adjust as needed)
    delayMS(10);                            // Wait for 10 milliseconds after setting VDV

    // Set the frame rate to 60Hz (common refresh rate for displays)
    ESP_LOGI(TAG, "Setting Frame Rate Control");
    spi_master_write_command(dev, 0xC6);    // Send the Frame Rate Control command (0xC6)
    spi_master_write_data_byte(dev, 0x0F);  // Set frame rate to 60Hz (0x0F corresponds to 60Hz)
    delayMS(10);                            // Wait for 10 milliseconds after setting frame rate

    // Configure power settings (power control for internal circuits)
    ESP_LOGI(TAG, "Setting Power Control 1");
    spi_master_write_command(dev, 0xD0);    // Send the Power Control 1 command (0xD0)
    spi_master_write_data_byte(dev, 0xA4);  // Set default power control setting (first parameter)
    spi_master_write_data_byte(dev, 0xA1);  // Set default power control setting (second parameter)
    delayMS(10);                            // Wait for 10 milliseconds after setting power control

    // Setting Positive Voltage Gamma Control
    ESP_LOGI(TAG, "Setting Positive Voltage Gamma Control");
    spi_master_write_command(dev, 0xE0);    // Positive Gamma Control
    uint8_t positive_gamma[] = {
        0xCF, // Parameter 1 (decreased by 1)
        0x07, // Parameter 2 (decreased by 1)
        0x10, // Parameter 3 (decreased by 1)
        0x07, // Parameter 4 (decreased by 1)
        0x0B, // Parameter 5 (decreased by 1)
        0x14, // Parameter 6 (decreased by 1)
        0x38, // Parameter 7 (decreased by 1)
        0x32, // Parameter 8 (decreased by 1)
        0x4F, // Parameter 9 (decreased by 1)
        0x35, // Parameter 10 (decreased by 1)
        0x12, // Parameter 11 (decreased by 1)
        0x13, // Parameter 12 (decreased by 1)
        0x28, // Parameter 13 (decreased by 1)
        0x2C  // Parameter 14 (decreased by 1)
    };
    for (int i = 0; i < sizeof(positive_gamma); i++) {
        spi_master_write_data_byte(dev, positive_gamma[i]);
    }
    delayMS(10);

    // Setting Negative Voltage Gamma Control
    ESP_LOGI(TAG, "Setting Negative Voltage Gamma Control");
    spi_master_write_command(dev, 0xE1);    // Negative Gamma Control
    uint8_t negative_gamma[] = {
        0xCF, // Parameter 1 (decreased by 1)
        0x07, // Parameter 2 (decreased by 1)
        0x0F, // Parameter 3 (decreased by 1)
        0x07, // Parameter 4 (decreased by 1)
        0x05, // Parameter 5 (decreased by 1)
        0x05, // Parameter 6 (decreased by 1)
        0x38, // Parameter 7 (decreased by 1)
        0x43, // Parameter 8 (decreased by 1)
        0x50, // Parameter 9 (decreased by 1)
        0x0A, // Parameter 10 (decreased by 1)
        0x15, // Parameter 11 (decreased by 1)
        0x13, // Parameter 12 (decreased by 1)
        0x2E, // Parameter 13 (decreased by 1)
        0x30  // Parameter 14 (decreased by 1)
    };
    for (int i = 0; i < sizeof(negative_gamma); i++) {
        spi_master_write_data_byte(dev, negative_gamma[i]);
    }
    delayMS(10);

    // Conditionally Enable display inversion for better color reproduction (inverts display colors)
    #if CONFIG_INVERSION
        ESP_LOGI(TAG, "Enabling Display Inversion");
        spi_master_write_command(dev, 0x21);    // INVON: Display Inversion On
        delayMS(10);                            // Wait for 10 milliseconds after enabling inversion
    #endif

    // Set the display to normal mode (exit partial mode or idle mode if previously set)
    ESP_LOGI(TAG, "Setting Normal Display Mode");
    spi_master_write_command(dev, 0x13);    // Send the Normal Display Mode On command (0x13)
    delayMS(10);                            // Wait for 10 milliseconds after setting normal mode

    // Turn on the display (make the display output visible content)
    ESP_LOGI(TAG, "Turning Display On");
    spi_master_write_command(dev, 0x29);    // Send the Display On command (0x29)
    delayMS(120);                           // Wait for 120 milliseconds for the display to fully turn on

    // Turn on the backlight if the backlight control pin is defined
    if (dev->_bl >= 0) {
        gpio_set_level(dev->_bl, 1);        // Set backlight pin high to turn on the backlight
        ESP_LOGI(TAG, "Backlight turned on via lcdInit");
    }
}

// Draw pixel
// x:X coordinate
// y:Y coordinate
// color:color
// Draw a single pixel on the display at (x, y) with the specified color
void lcdDrawPixel(TFT_t *dev, uint16_t x, uint16_t y, uint16_t color) {
    // Check if coordinates are within the display area
    if (x >= dev->_width || y >= dev->_height)
        return;

    // Adjust for any offsets
    uint16_t _x = x + dev->_offsetx;
    uint16_t _y = y + dev->_offsety;

    // Set the column address (X coordinate)
    spi_master_write_command(dev, 0x2A); // Column Address Set command
    spi_master_write_addr(dev, _x, _x);

    // Set the row address (Y coordinate)
    spi_master_write_command(dev, 0x2B); // Row Address Set command
    spi_master_write_addr(dev, _y, _y);

    // Write the pixel color data to the display memory
    spi_master_write_command(dev, 0x2C); // Memory Write command
    spi_master_write_data_word(dev, color);
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

    spi_master_write_command(dev, 0x2A);  // CASET: Column Address Set
    spi_master_write_addr(dev, _x1, _x2);
    spi_master_write_command(dev, 0x2B);  // RASET: Row Address Set
    spi_master_write_addr(dev, _y1, _y2);
    spi_master_write_command(dev, 0x2C);  // RAMWR: Memory Write
    spi_master_write_colors(dev, colors, size);
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

    spi_master_write_command(dev, 0x2A);  // CASET: Column Address Set
    spi_master_write_addr(dev, _x1, _x2);
    spi_master_write_command(dev, 0x2B);  // RASET: Row Address Set
    spi_master_write_addr(dev, _y1, _y2);
    spi_master_write_command(dev, 0x2C);  // RAMWR: Memory Write

    uint32_t size = (_x2 - _x1 + 1) * (_y2 - _y1 + 1);
    spi_master_write_color(dev, color, size);
}

// Draw a bitmap to the display
// x: X coordinate (start position)
// y: Y coordinate (start position)
// w: Width of the bitmap
// h: Height of the bitmap
// data: Pointer to the bitmap data (RGB565 format)
// For lcdDrawBitmap, we also need to use asynchronous SPI calls.
// Here's where we implement the pipeline approach in lcdDrawBitmap:
void lcdDrawBitmap(TFT_t *dev, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *data) {
    // Check if coordinates are within the display area
    if ((x + w) > dev->_width || (y + h) > dev->_height)
        return;

    uint16_t x1 = x + dev->_offsetx;
    uint16_t y1 = y + dev->_offsety;
    uint16_t x2 = x1 + w - 1;
    uint16_t y2 = y1 + h - 1;

    // Set the column address (X coordinates)
    spi_master_write_command(dev, 0x2A); // Column Address Set
    spi_master_write_addr(dev, x1, x2);

    // Set the row address (Y coordinates)
    spi_master_write_command(dev, 0x2B); // Row Address Set
    spi_master_write_addr(dev, y1, y2);

    // Write the pixel data to the display memory
    spi_master_write_command(dev, 0x2C); // Memory Write

    uint32_t data_size = w * h * 2; // Size in bytes
    uint8_t *data_ptr = (uint8_t *)data;

    uint32_t max_chunk_size = 32 * 1024; // 32KB
    uint32_t remaining = data_size;

    // Variables for pipeline
    spi_transaction_t trans; // Declare transaction outside loop so it remains valid.
    spi_transaction_t *rtrans = NULL; // For receiving completed transactions
    spi_transaction_t *pending_trans = NULL; // Track a pending transaction

    // We now implement pipelining:
    // On each iteration, if we had a pending transaction from previous iteration,
    // we wait for it to finish before queueing a new one. This allows SPI and CPU to overlap:
    // CPU prepares next chunk while SPI is still busy from previous chunk.

    while (remaining > 0) {
        uint32_t chunk_size = (remaining > max_chunk_size) ? max_chunk_size : remaining;

        // Before we queue the next transaction, if there's a pending transaction,
        // wait for it to complete. This gives SPI time to finish previous chunk
        // while we were calculating chunk_size or doing other work.
        if (pending_trans != NULL) {
            esp_err_t ret = spi_device_get_trans_result(dev->_SPIHandle, &rtrans, portMAX_DELAY);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "lcdDrawBitmap: spi_device_get_trans_result failed: %s", esp_err_to_name(ret));
                return;
            }
            // Now the previous transaction is done, we can reuse 'trans' for this chunk.
            pending_trans = NULL;
        }

        // Prepare the transaction for this chunk
        memset(&trans, 0, sizeof(trans));
        trans.length = chunk_size * 8; // Length in bits
        trans.tx_buffer = data_ptr;
        gpio_set_level(dev->_dc, SPI_Data_Mode);

        // Queue the transaction without waiting for immediate completion
        esp_err_t ret = spi_device_queue_trans(dev->_SPIHandle, &trans, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "lcdDrawBitmap: spi_device_queue_trans failed: %s", esp_err_to_name(ret));
            return;
        }

        // Now we have a pending transaction in flight
        pending_trans = &trans;

        data_ptr += chunk_size;
        remaining -= chunk_size;
    }

    // After finishing the loop, we may have one last pending transaction.
    // Wait for it to complete.
    if (pending_trans != NULL) {
        esp_err_t ret = spi_device_get_trans_result(dev->_SPIHandle, &rtrans, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "lcdDrawBitmap: Final spi_device_get_trans_result failed: %s", esp_err_to_name(ret));
            return;
        }
        pending_trans = NULL;
    }

    // Now all transactions are done, and we effectively overlapped
    // the transaction time of previous chunks with the preparation time of next chunks.
    // This can lead to higher effective frame rates.
}

// New function to draw a small rectangle bitmap (optimized for small regions)
// x: X coordinate (start position)
// y: Y coordinate (start position)
// w: Width of the rectangle
// h: Height of the rectangle
// data: Pointer to the pixel data (RGB565 format)
void lcdDrawBitmapRect(TFT_t *dev, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *data) {
    // Check if coordinates are within the display area
    if ((x + w) > dev->_width || (y + h) > dev->_height)
        return;

    // Adjust for any offsets
    uint16_t x1 = x + dev->_offsetx;
    uint16_t y1 = y + dev->_offsety;
    uint16_t x2 = x1 + w - 1;
    uint16_t y2 = y1 + h - 1;

    // Set the column address (X coordinates)
    spi_master_write_command(dev, 0x2A); // Column Address Set
    spi_master_write_addr(dev, x1, x2);

    // Set the row address (Y coordinates)
    spi_master_write_command(dev, 0x2B); // Row Address Set
    spi_master_write_addr(dev, y1, y2);

    // Write the pixel data to the display memory
    spi_master_write_command(dev, 0x2C); // Memory Write

    uint32_t data_size = w * h * 2; // Size in bytes
    uint8_t *data_ptr = (uint8_t *)data;

    // For small rectangles, transmit data in a single transaction
    spi_transaction_t trans;
    memset(&trans, 0, sizeof(trans));
    trans.length = data_size * 8; // Length in bits
    trans.tx_buffer = data_ptr;
    gpio_set_level(dev->_dc, SPI_Data_Mode);

    esp_err_t ret = spi_device_polling_transmit(dev->_SPIHandle, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "lcdDrawBitmapRect: SPI transmit failed: %s", esp_err_to_name(ret));
        return;
    }
}

// Display OFF
void lcdDisplayOff(TFT_t *dev) {
    spi_master_write_command(dev, 0x28);  // DISPOFF: Display Off
}

// Display ON
void lcdDisplayOn(TFT_t *dev) {
    spi_master_write_command(dev, 0x29);  // DISPON: Display On
}

// Fill screen
// color:color
// Fill the entire screen with a single color
void lcdFillScreen(TFT_t *dev, uint16_t color) {
    // Draw a filled rectangle covering the whole screen
    lcdDrawFillRect(dev, 0, 0, dev->_width - 1, dev->_height - 1, color);
}


// Draw line
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End   X coordinate
// y2:End   Y coordinate
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
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End   X coordinate
// y2:End   Y coordinate
// color:color
void lcdDrawRect(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    lcdDrawLine(dev, x1, y1, x2, y1, color);
    lcdDrawLine(dev, x2, y1, x2, y2, color);
    lcdDrawLine(dev, x2, y2, x1, y2, color);
    lcdDrawLine(dev, x1, y2, x1, y1, color);
}

// Draw rectangle with angle
// xc:Center X coordinate
// yc:Center Y coordinate
// w:Width of rectangle
// h:Height of rectangle
// angle:Angle of rectangle
// color:color

// When the origin is (0, 0), the point (x1, y1) after rotating the point (x, y) by the angle is obtained by the following calculation.
// x1 = x * cos(angle) - y * sin(angle)
// y1 = x * sin(angle) + y * cos(angle)
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
// xc:Center X coordinate
// yc:Center Y coordinate
// w:Width of triangle
// h:Height of triangle
// angle:Angle of triangle
// color:color

// When the origin is (0, 0), the point (x1, y1) after rotating the point (x, y) by the angle is obtained by the following calculation.
// x1 = x * cos(angle) - y * sin(angle)
// y1 = x * sin(angle) + y * cos(angle)
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
// x0:Central X coordinate
// y0:Central Y coordinate
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

// Draw circle of filling
// x0:Central X coordinate
// y0:Central Y coordinate
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

// Draw rectangle with round corner
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End   X coordinate
// y2:End   Y coordinate
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
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End   X coordinate
// y2:End   Y coordinate
// w:Width of the bottom
// color:color
// Thanks http://k-hiura.cocolog-nifty.com/blog/2010/11/post-2a62.html
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
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End   X coordinate
// y2:End   Y coordinate
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
    // Compute luminance to determine tonal range
    float luminance = 0.2126f * r + 0.7152f * g + 0.0722f * b;

    // Determine tonal range thresholds (you can adjust these thresholds as needed)
    const float shadows_threshold = 85.0f;      // Approx 33% of 255
    const float highlights_threshold = 170.0f;  // Approx 66% of 255

    int adjust_cyan_red = 0;
    int adjust_magenta_green = 0;
    int adjust_yellow_blue = 0;

    // Select the appropriate adjustments based on the tonal range
    if (luminance <= shadows_threshold) {
        // Shadows
        adjust_cyan_red = color_tweaks.shadows_cyan_red;
        adjust_magenta_green = color_tweaks.shadows_magenta_green;
        adjust_yellow_blue = color_tweaks.shadows_yellow_blue;
    } else if (luminance >= highlights_threshold) {
        // Highlights
        adjust_cyan_red = color_tweaks.highlights_cyan_red;
        adjust_magenta_green = color_tweaks.highlights_magenta_green;
        adjust_yellow_blue = color_tweaks.highlights_yellow_blue;
    } else {
        // Midtones
        adjust_cyan_red = color_tweaks.midtones_cyan_red;
        adjust_magenta_green = color_tweaks.midtones_magenta_green;
        adjust_yellow_blue = color_tweaks.midtones_yellow_blue;
    }

    // Apply color adjustments as percentage
    float rf = r * (1.0f + adjust_cyan_red / 100.0f);
    float gf = g * (1.0f + adjust_magenta_green / 100.0f);
    float bf = b * (1.0f + adjust_yellow_blue / 100.0f);

    // Clamp the values between 0 and 255 to avoid overflow or underflow
    rf = fminf(fmaxf(rf, 0.0f), 255.0f);
    gf = fminf(fmaxf(gf, 0.0f), 255.0f);
    bf = fminf(fmaxf(bf, 0.0f), 255.0f);

    // Apply brightness adjustment
    float brightness_factor = brightness_percent / 100.0f * 255.0f;
    rf = rf + brightness_factor;
    gf = gf + brightness_factor;
    bf = bf + brightness_factor;

    // Clamp values again after brightness adjustment
    rf = fminf(fmaxf(rf, 0.0f), 255.0f);
    gf = fminf(fmaxf(gf, 0.0f), 255.0f);
    bf = fminf(fmaxf(bf, 0.0f), 255.0f);

    // Apply contrast adjustment
    float contrast_factor = (100.0f + contrast_percent) / 100.0f;
    float midpoint = 128.0f;

    rf = (rf - midpoint) * contrast_factor + midpoint;
    gf = (gf - midpoint) * contrast_factor + midpoint;
    bf = (bf - midpoint) * contrast_factor + midpoint;

    // Clamp values again after contrast adjustment
    rf = fminf(fmaxf(rf, 0.0f), 255.0f);
    gf = fminf(fmaxf(gf, 0.0f), 255.0f);
    bf = fminf(fmaxf(bf, 0.0f), 255.0f);

    // Apply gamma correction to each channel
    rf = powf(rf / 255.0f, gamma_red) * 255.0f;
    gf = powf(gf / 255.0f, gamma_green) * 255.0f;
    bf = powf(bf / 255.0f, gamma_blue) * 255.0f;

    // Convert to integer with rounding
    uint16_t ri = (uint16_t)(rf + 0.5f);
    uint16_t gi = (uint16_t)(gf + 0.5f);
    uint16_t bi = (uint16_t)(bf + 0.5f);

    // Convert to RGB565 format
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
    // Remove or comment out the unused variable 'yd2'
    // int16_t yd2 = 0;
    uint16_t xss = 0;
    uint16_t yss = 0;
    int16_t xsd = 0;
    int16_t ysd = 0;
    int16_t next = 0;
    uint16_t x0  = 0;
    uint16_t x1  = 0;
    uint16_t y0  = 0;
    uint16_t y1_ = 0;
    if (dev->_font_direction == DIRECTION0) {
        xd1 = +1;
        yd1 = +1; //-1;
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
        yd1 = -1; //+1;
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
        ysd =  1; //-1;
        xss =  x + ph;
        yss =  y;
        xsd =  0;
        next = y + pw; //y - pw;

        x0  = x;
        y0  = y;
        x1  = x + (ph-1);
        y1_ = y + (pw-1);
    } else if (dev->_font_direction == DIRECTION270) {
        xd1 =  0;
        yd1 =  0;
        xd2 = +1;
        ysd =  -1; //+1;
        xss =  x - (ph - 1);
        yss =  y;
        xsd =  0;
        next = y - pw; //y + pw;

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
        //for(w=0;w<(pw/8);w++) {
        bits = pw;
        for(w = 0; w < ((pw + 4) / 8); w++) {
            mask = 0x80;
            for(bit = 0; bit < 8; bit++) {
                bits--;
                if (bits < 0) continue;
                //if(_DEBUG_)printf("xx=%d yy=%d mask=%02x fonts[%d]=%02x\n",xx,yy,mask,ofs,fonts[ofs]);
                if (fonts[ofs] & mask) {
                    lcdDrawPixel(dev, xx, yy, color);
                } else {
                    //if (dev->_font_fill) lcdDrawPixel(dev, xx, yy, dev->_font_fill_color);
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

#if 0
// Draw UTF8 character
// x:X coordinate
// y:Y coordinate
// utf8:UTF8 code
// color:color
int lcdDrawUTF8Char(TFT_t *dev, FontxFile *fx, uint16_t x, uint16_t y, uint8_t *utf8, uint16_t color) {
    uint16_t sjis[1];

    sjis[0] = UTF2SJIS(utf8);
    if(_DEBUG_)printf("sjis=%04x\n",sjis[0]);
    return lcdDrawSJISChar(dev, fx, x, y, sjis[0], color);
}

// Draw UTF8 string
// x:X coordinate
// y:Y coordinate
// utfs:UTF8 string
// color:color
int lcdDrawUTF8String(TFT_t * dev, FontxFile *fx, uint16_t x, uint16_t y, unsigned char *utfs, uint16_t color) {

    int i;
    int spos;
    uint16_t sjis[64];
    spos = String2SJIS(utfs, strlen((char *)utfs), sjis, 64);
    if(_DEBUG_)printf("spos=%d\n",spos);
    ESP_LOGD(TAG, "Drawing UTF8 string with %d characters", spos);
    for(i=0;i<spos;i++) {
        if(_DEBUG_)printf("sjis[%d]=%x y=%d\n",i,sjis[i],y);
        if (dev->_font_direction == 0)
            x = lcdDrawSJISChar(dev, fx, x, y, sjis[i], color);
        if (dev->_font_direction == 1)
            y = lcdDrawSJISChar(dev, fx, x, y, sjis[i], color);
        if (dev->_font_direction == 2)
            x = lcdDrawSJISChar(dev, fx, x, y, sjis[i], color);
        if (dev->_font_direction == 3)
            y = lcdDrawSJISChar(dev, fx, x, y, sjis[i], color);
    }
    if (dev->_font_direction == 0) return x;
    if (dev->_font_direction == 2) return x;
    if (dev->_font_direction == 1) return y;
    if (dev->_font_direction == 3) return y;
    return 0;
}
#endif

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
        gpio_set_level(dev->_bl, 1);  // Set backlight pin high
        ESP_LOGI(TAG, "Backlight turned on via lcdBacklightOn");
    }
}

// Backlight OFF
void lcdBacklightOff(TFT_t *dev) {
    if (dev->_bl >= 0) {
        gpio_set_level(dev->_bl, 0);  // Set backlight pin low
        ESP_LOGI(TAG, "Backlight turned off via lcdBacklightOff");
    }
}


// Display Inversion Off
void lcdInversionOff(TFT_t *dev) {
    ESP_LOGI(TAG, "Disabling Display Inversion");
    spi_master_write_command(dev, 0x20);  // INVOFF: Display Inversion Off
}

// Display Inversion On
void lcdInversionOn(TFT_t *dev) {
    ESP_LOGI(TAG, "Enabling Display Inversion");
    spi_master_write_command(dev, 0x21);  // INVON: Display Inversion On
}