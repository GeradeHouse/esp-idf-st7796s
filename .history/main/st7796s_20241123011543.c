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
 * Author: GeradeHouse Productions
 * Date: November 23, 2024
 */

#include <string.h>
#include <inttypes.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_log.h"

#include "st7796s.h"  // Updated to the correct header file

#define TAG "ST7796S"
#define _DEBUG_ 0

#if CONFIG_SPI2_HOST
#define HOST_ID SPI2_HOST
#elif CONFIG_SPI3_HOST
#define HOST_ID SPI3_HOST
#endif

static const int SPI_Command_Mode = 0;
static const int SPI_Data_Mode = 1;
static const int SPI_Frequency = SPI_MASTER_FREQ_40M;

void spi_master_init(TFT_t *dev, int16_t GPIO_MOSI, int16_t GPIO_SCLK, int16_t GPIO_CS,
                     int16_t GPIO_DC, int16_t GPIO_RESET, int16_t GPIO_BL) {
    esp_err_t ret;

    // Initialize CS pin
    ESP_LOGI(TAG, "GPIO_CS=%d", GPIO_CS);
    if (GPIO_CS >= 0) {
        gpio_reset_pin(GPIO_CS);  
        gpio_set_direction(GPIO_CS, GPIO_MODE_OUTPUT);
        gpio_set_level(GPIO_CS, 1);
    }

    // Initialize DC pin
    ESP_LOGI(TAG, "GPIO_DC=%d", GPIO_DC);
    gpio_reset_pin(GPIO_DC);
    gpio_set_direction(GPIO_DC, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_DC, 0);

    // Initialize RESET pin
    ESP_LOGI(TAG, "GPIO_RESET=%d", GPIO_RESET);
    if (GPIO_RESET >= 0) {
        gpio_reset_pin(GPIO_RESET);
        gpio_set_direction(GPIO_RESET, GPIO_MODE_OUTPUT);
        gpio_set_level(GPIO_RESET, 1);  // Ensure the reset pin starts high
        delayMS(100);
        gpio_set_level(GPIO_RESET, 0);  // Pulse reset pin
        delayMS(100);
        gpio_set_level(GPIO_RESET, 1);  // Set back to high
        delayMS(100);
    }

    // Initialize SPI bus
    ESP_LOGI(TAG, "GPIO_MOSI=%d", GPIO_MOSI);
    ESP_LOGI(TAG, "GPIO_SCLK=%d", GPIO_SCLK);
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 6 * 1024,
        .flags = SPICOMMON_BUSFLAG_MASTER,
    };

    ret = spi_bus_initialize(HOST_ID, &buscfg, SPI_DMA_CH_AUTO);
    ESP_LOGI(TAG, "spi_bus_initialize=%d", ret);
    assert(ret == ESP_OK);

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

    spi_device_handle_t handle;
    ret = spi_bus_add_device(HOST_ID, &devcfg, &handle);
    ESP_LOGI(TAG, "spi_bus_add_device=%d", ret);
    assert(ret == ESP_OK);

    // Store GPIO numbers and SPI handle in device structure
    dev->_dc = GPIO_DC;
    dev->_bl = GPIO_BL;
    dev->_SPIHandle = handle;

    // Initialize Backlight pin AFTER SPI initialization
    ESP_LOGI(TAG, "GPIO_BL=%d", GPIO_BL);
    if (GPIO_BL >= 1) {
        gpio_reset_pin(GPIO_BL);
        gpio_set_direction(GPIO_BL, GPIO_MODE_OUTPUT);
        gpio_set_level(GPIO_BL, 0);  // Set GPIO_BL high to turn on the backlight
        ESP_LOGI(TAG, "Backlight enabled on GPIO_BL=%d", GPIO_BL);
    }
}


bool spi_master_write_byte(spi_device_handle_t SPIHandle, const uint8_t *Data, size_t DataLength) {
    spi_transaction_t SPITransaction;
    esp_err_t ret;

    if (DataLength > 0) {
        memset(&SPITransaction, 0, sizeof(spi_transaction_t));
        SPITransaction.length = DataLength * 8;
        SPITransaction.tx_buffer = Data;
        ret = spi_device_polling_transmit(SPIHandle, &SPITransaction);
        assert(ret == ESP_OK);
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
        spi_master_write_byte(dev->_SPIHandle, Byte, chunk_size);
        len -= chunk_size;
    }
    return true;
}

// Add 202001
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
        spi_master_write_byte(dev->_SPIHandle, Byte, chunk_size);
        len -= chunk_size;
    }
    return true;
}

void delayMS(int ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

// Initialize the LCD display with specified dimensions and offsets
// Define orientation constants for the display
#define ORIENTATION_LANDSCAPE 0x48 // Landscape orientation
#define ORIENTATION_PORTRAIT  0x28 // Portrait orientation
// Add other orientation values as needed (e.g., inverted landscape, inverted portrait, etc.)

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

    // Set positive voltage gamma control for color adjustment (gamma curve)
    ESP_LOGI(TAG, "Setting Positive Voltage Gamma Control");
    spi_master_write_command(dev, 0xE0);    // Send the Positive Gamma Control command (0xE0)
    // Write gamma curve settings for positive voltages (adjust as needed for color calibration)
    uint8_t positive_gamma[] = {
        0xD0, // Gamma correction parameter 1
        0x08, // Gamma correction parameter 2
        0x11, // Gamma correction parameter 3
        0x08, // Gamma correction parameter 4
        0x0C, // Gamma correction parameter 5
        0x15, // Gamma correction parameter 6
        0x39, // Gamma correction parameter 7
        0x33, // Gamma correction parameter 8
        0x50, // Gamma correction parameter 9
        0x36, // Gamma correction parameter 10
        0x13, // Gamma correction parameter 11
        0x14, // Gamma correction parameter 12
        0x29, // Gamma correction parameter 13
        0x2D  // Gamma correction parameter 14
    };
    for (int i = 0; i < sizeof(positive_gamma); i++) {
        spi_master_write_data_byte(dev, positive_gamma[i]); // Write each gamma setting
    }
    delayMS(10);                            // Wait for 10 milliseconds after setting positive gamma

    // Set negative voltage gamma control for color adjustment (gamma curve)
    ESP_LOGI(TAG, "Setting Negative Voltage Gamma Control");
    spi_master_write_command(dev, 0xE1);    // Send the Negative Gamma Control command (0xE1)
    // Write gamma curve settings for negative voltages (adjust as needed for color calibration)
    uint8_t negative_gamma[] = {
        0xD0, // Gamma correction parameter 1
        0x08, // Gamma correction parameter 2
        0x10, // Gamma correction parameter 3
        0x08, // Gamma correction parameter 4
        0x06, // Gamma correction parameter 5
        0x06, // Gamma correction parameter 6
        0x39, // Gamma correction parameter 7
        0x44, // Gamma correction parameter 8
        0x51, // Gamma correction parameter 9
        0x0B, // Gamma correction parameter 10
        0x16, // Gamma correction parameter 11
        0x14, // Gamma correction parameter 12
        0x2F, // Gamma correction parameter 13
        0x31  // Gamma correction parameter 14
    };
    for (int i = 0; i < sizeof(negative_gamma); i++) {
        spi_master_write_data_byte(dev, negative_gamma[i]); // Write each gamma setting
    }
    delayMS(10);                            // Wait for 10 milliseconds after setting negative gamma

    // Enable display inversion for better color reproduction (inverts display colors)
    ESP_LOGI(TAG, "Enabling Display Inversion");
    spi_master_write_command(dev, 0x21);    // Send the Display Inversion On command (0x21)
    delayMS(10);                            // Wait for 10 milliseconds after enabling inversion

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

// RGB565 conversion
// RGB565 is R(5)+G(6)+B(5)=16bit color format.
// Bit image "RRRRRGGGGGGBBBBB"
uint16_t rgb565_conv(uint16_t r, uint16_t g, uint16_t b) {
    return (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// Draw ASCII character
// x:X coordinate
// y:Y coordinate
// ascii: ascii code
// color:color
int lcdDrawChar(TFT_t * dev, FontxFile *fxs, uint16_t x, uint16_t y, uint8_t ascii, uint16_t color) {
	uint16_t xx,yy,bit,ofs;
	unsigned char fonts[128]; // font pattern
	unsigned char pw, ph;
	int h,w;
	uint16_t mask;
	bool rc;

	if(_DEBUG_)printf("_font_direction=%d\n",dev->_font_direction);
	rc = GetFontx(fxs, ascii, fonts, &pw, &ph);
	if(_DEBUG_)printf("GetFontx rc=%d pw=%d ph=%d\n",rc,pw,ph);
	if (!rc) return 0;

	int16_t xd1 = 0;
	int16_t yd1 = 0;
	int16_t xd2 = 0;
	int16_t yd2 = 0;
	uint16_t xss = 0;
	uint16_t yss = 0;
	int16_t xsd = 0;
	int16_t ysd = 0;
	int16_t next = 0;
	uint16_t x0  = 0;
	uint16_t x1  = 0;
	uint16_t y0  = 0;
	uint16_t y1  = 0;
	if (dev->_font_direction == 0) {
		xd1 = +1;
		yd1 = +1; //-1;
		xd2 =  0;
		yd2 =  0;
		xss =  x;
		yss =  y - (ph - 1);
		xsd =  1;
		ysd =  0;
		next = x + pw;

		x0	= x;
		y0	= y - (ph-1);
		x1	= x + (pw-1);
		y1	= y;
	} else if (dev->_font_direction == 2) {
		xd1 = -1;
		yd1 = -1; //+1;
		xd2 =  0;
		yd2 =  0;
		xss =  x;
		yss =  y + ph + 1;
		xsd =  1;
		ysd =  0;
		next = x - pw;

		x0	= x - (pw-1);
		y0	= y;
		x1	= x;
		y1	= y + (ph-1);
	} else if (dev->_font_direction == 1) {
		xd1 =  0;
		yd1 =  0;
		xd2 = -1;
		yd2 = +1; //-1;
		xss =  x + ph;
		yss =  y;
		xsd =  0;
		ysd =  1;
		next = y + pw; //y - pw;

		x0	= x;
		y0	= y;
		x1	= x + (ph-1);
		y1	= y + (pw-1);
	} else if (dev->_font_direction == 3) {
		xd1 =  0;
		yd1 =  0;
		xd2 = +1;
		yd2 = -1; //+1;
		xss =  x - (ph - 1);
		yss =  y;
		xsd =  0;
		ysd =  1;
		next = y - pw; //y + pw;

		x0	= x - (ph-1);
		y0	= y - (pw-1);
		x1	= x;
		y1	= y;
	}

	if (dev->_font_fill) lcdDrawFillRect(dev, x0, y0, x1, y1, dev->_font_fill_color);
	
	ESP_LOGD(TAG, "Filling font background: x0=%d y0=%d x1=%d y1=%d color=0x%04X", x0, y0, x1, y1, dev->_font_fill_color);


	int bits;
	if(_DEBUG_)printf("xss=%d yss=%d\n",xss,yss);
	ofs = 0;
	yy = yss;
	xx = xss;
	for(h=0;h<ph;h++) {
		if(xsd) xx = xss;
		if(ysd) yy = yss;
		//for(w=0;w<(pw/8);w++) {
		bits = pw;
		for(w=0;w<((pw+4)/8);w++) {
			mask = 0x80;
			for(bit=0;bit<8;bit++) {
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
				yy = yy + yd2;
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
int lcdDrawUTF8Char(TFT_t * dev, FontxFile *fx, uint16_t x,uint16_t y,uint8_t *utf8,uint16_t color) {
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
