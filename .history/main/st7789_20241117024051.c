#include <string.h>
#include <inttypes.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_log.h"

#include "st7789.h"

#define TAG "ST7789"
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

    ESP_LOGI(TAG, "GPIO_CS=%d", GPIO_CS);
    if (GPIO_CS >= 0) {
        gpio_reset_pin(GPIO_CS);
        gpio_set_direction(GPIO_CS, GPIO_MODE_OUTPUT);
        gpio_set_level(GPIO_CS, 0);
    }

    ESP_LOGI(TAG, "GPIO_DC=%d", GPIO_DC);
    gpio_reset_pin(GPIO_DC);
    gpio_set_direction(GPIO_DC, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_DC, 0);

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

    ESP_LOGI(TAG, "GPIO_BL=%d", GPIO_BL);
    if (GPIO_BL >= 0) {
        gpio_reset_pin(GPIO_BL);
        gpio_set_direction(GPIO_BL, GPIO_MODE_OUTPUT);
        gpio_set_level(GPIO_BL, 1);  // Set GPIO_BL high to turn on the backlight
        ESP_LOGI(TAG, "Backlight enabled on GPIO_BL=%d", GPIO_BL);
    }

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
    devcfg.mode = 2;
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

    dev->_dc = GPIO_DC;
    dev->_bl = GPIO_BL;
    dev->_SPIHandle = handle;
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
    static uint8_t Byte = 0;
    Byte = cmd;
    gpio_set_level(dev->_dc, SPI_Command_Mode);
    return spi_master_write_byte(dev->_SPIHandle, &Byte, 1);
}

bool spi_master_write_data_byte(TFT_t *dev, uint8_t data) {
    static uint8_t Byte = 0;
    Byte = data;
    gpio_set_level(dev->_dc, SPI_Data_Mode);
    return spi_master_write_byte(dev->_SPIHandle, &Byte, 1);
}

bool spi_master_write_data_word(TFT_t *dev, uint16_t data) {
    static uint8_t Byte[2];
    Byte[0] = (data >> 8) & 0xFF;
    Byte[1] = data & 0xFF;
    gpio_set_level(dev->_dc, SPI_Data_Mode);
    return spi_master_write_byte(dev->_SPIHandle, Byte, 2);
}

bool spi_master_write_addr(TFT_t *dev, uint16_t addr1, uint16_t addr2) {
    static uint8_t Byte[4];
    Byte[0] = (addr1 >> 8) & 0xFF;
    Byte[1] = addr1 & 0xFF;
    Byte[2] = (addr2 >> 8) & 0xFF;
    Byte[3] = addr2 & 0xFF;
    gpio_set_level(dev->_dc, SPI_Data_Mode);
    return spi_master_write_byte(dev->_SPIHandle, Byte, 4);
}

bool spi_master_write_color(TFT_t *dev, uint16_t color, uint16_t size) {
    static uint8_t Byte[1024];
    int index = 0;
    uint32_t len = size * 2;
    uint32_t max_chunk = sizeof(Byte);

    while (len > 0) {
        uint32_t chunk_size = (len > max_chunk) ? max_chunk : len;
        for (int i = 0; i < chunk_size / 2; i++) {
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
    static uint8_t Byte[1024];
    int index = 0;
    uint32_t len = size * 2;
    uint32_t max_chunk = sizeof(Byte);

    while (len > 0) {
        uint32_t chunk_size = (len > max_chunk) ? max_chunk : len;
        int color_count = chunk_size / 2;
        for (int i = 0; i < color_count; i++) {
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

void lcdInit(TFT_t *dev, int width, int height, int offsetx, int offsety) {
    dev->_width = width;
    dev->_height = height;
    dev->_offsetx = offsetx;
    dev->_offsety = offsety;
    dev->_font_direction = DIRECTION0;
    dev->_font_fill = false;
    dev->_font_underline = false;

    ESP_LOGI(TAG, "Initializing LCD");

    ESP_LOGI(TAG, "Sending Software Reset");
    spi_master_write_command(dev, 0x01);  // Software Reset
    delayMS(150);

    ESP_LOGI(TAG, "Exiting Sleep Mode");
    spi_master_write_command(dev, 0x11);  // Sleep Out
    delayMS(255);

    ESP_LOGI(TAG, "Setting Interface Pixel Format");
    spi_master_write_command(dev, 0x3A);  // Interface Pixel Format
    spi_master_write_data_byte(dev, 0x55);
    delayMS(10);

    ESP_LOGI(TAG, "Setting Memory Data Access Control");
    spi_master_write_command(dev, 0x36);  // Memory Data Access Control
    spi_master_write_data_byte(dev, 0x00);

    ESP_LOGI(TAG, "Setting Column Address Set");
    spi_master_write_command(dev, 0x2A);  // Column Address Set
    spi_master_write_data_byte(dev, 0x00);
    spi_master_write_data_byte(dev, 0x00);
    spi_master_write_data_byte(dev, (dev->_width - 1) >> 8);
    spi_master_write_data_byte(dev, (dev->_width - 1) & 0xFF);

    ESP_LOGI(TAG, "Setting Row Address Set");
    spi_master_write_command(dev, 0x2B);  // Row Address Set
    spi_master_write_data_byte(dev, 0x00);
    spi_master_write_data_byte(dev, 0x00);
    spi_master_write_data_byte(dev, (dev->_height - 1) >> 8);
    spi_master_write_data_byte(dev, (dev->_height - 1) & 0xFF);

    ESP_LOGI(TAG, "Enabling Display Inversion");
    spi_master_write_command(dev, 0x21);  // Display Inversion On
    delayMS(10);

    ESP_LOGI(TAG, "Setting Normal Display Mode");
    spi_master_write_command(dev, 0x13);  // Normal Display Mode On
    delayMS(10);

    ESP_LOGI(TAG, "Turning Display On");
    spi_master_write_command(dev, 0x29);  // Display ON
    delayMS(255);

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
    if (x >= dev->_width)
        return;
    if (y >= dev->_height)
        return;

    uint16_t _x = x + dev->_offsetx;
    uint16_t _y = y + dev->_offsety;

    spi_master_write_command(dev, 0x2A);  // set column(x) address
    spi_master_write_addr(dev, _x, _x);
    spi_master_write_command(dev, 0x2B);  // set Page(y) address
    spi_master_write_addr(dev, _y, _y);
    spi_master_write_command(dev, 0x2C);  // Memory Write
    spi_master_write_data_word(dev, color);
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

    spi_master_write_command(dev, 0x2A);  // set column(x) address
    spi_master_write_addr(dev, _x1, _x2);
    spi_master_write_command(dev, 0x2B);  // set Page(y) address
    spi_master_write_addr(dev, _y1, _y2);
    spi_master_write_command(dev, 0x2C);  // Memory Write

    uint32_t size = (_x2 - _x1 + 1) * (_y2 - _y1 + 1);
    spi_master_write_color(dev, color, size);
}

// Fill screen
// color:color
void lcdFillScreen(TFT_t *dev, uint16_t color) {
    lcdDrawFillRect(dev, 0, 0, dev->_width - 1, dev->_height - 1, color);
}

// Display OFF
void lcdDisplayOff(TFT_t *dev) {
    spi_master_write_command(dev, 0x28);  // Display off
}

// Display ON
void lcdDisplayOn(TFT_t *dev) {
    spi_master_write_command(dev, 0x29);  // Display on
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

// Backlight OFF
void lcdBacklightOff(TFT_t *dev) {
    if (dev->_bl >= 0) {
        gpio_set_level(dev->_bl, 0);
    }
}

// Backlight ON
void lcdBacklightOn(TFT_t *dev) {
    if (dev->_bl >= 0) {
        gpio_set_level(dev->_bl, 1);
    }
}

// Display Inversion Off
void lcdInversionOff(TFT_t *dev) {
    spi_master_write_command(dev, 0x20);  // Display Inversion Off
}

// Display Inversion On
void lcdInversionOn(TFT_t *dev) {
    spi_master_write_command(dev, 0x21);  // Display Inversion On
}
