// main/st7796s.h

#ifndef MAIN_ST7796S_H_
#define MAIN_ST7796S_H_

#include <stdbool.h> // Added this include to ensure bool is defined
#include "fontx.h"    // We still need fontx.h for font functions

// Removed #include "driver/spi_master.h" since we're no longer using SPI.

// Color definitions in RGB565 format
#define RED             0xF800
#define GREEN           0x07E0
#define BLUE            0x001F
#define BLACK           0x0000
#define WHITE           0xFFFF
#define GRAY            0x8410
#define YELLOW          0xFFE0
#define CYAN            0x07FF
#define PURPLE          0xF81F

// Display orientations
#define DIRECTION0      0
#define DIRECTION90     1
#define DIRECTION180    2
#define DIRECTION270    3

// Correctly define orientation constants for the display
#define ORIENTATION_LANDSCAPE           0x48 // Landscape orientation
#define ORIENTATION_PORTRAIT            0x28 // Portrait orientation
#define ORIENTATION_INVERTED_LANDSCAPE  0x88 // Inverted landscape orientation
#define ORIENTATION_INVERTED_PORTRAIT   0xE8 // Inverted portrait orientation

typedef struct {
    uint16_t _width;
    uint16_t _height;
    uint16_t _offsetx;
    uint16_t _offsety;
    uint16_t _font_direction;
    bool     _font_fill;
    uint16_t _font_fill_color;
    bool     _font_underline;
    uint16_t _font_underline_color;
    int16_t  _dc;
    int16_t  _bl;
    int16_t  _reset;
    // Removed spi_device_handle_t _SPIHandle; since SPI is no longer used.
    // For parallel interface, we add write/read control pins.
    int16_t  _wr;
    int16_t  _rd;
} TFT_t;

typedef struct {
    // For Shadows (dark areas)
    int shadows_cyan_red;       // Range from -100 to 100 (%)
    int shadows_magenta_green;  // Range from -100 to 100 (%)
    int shadows_yellow_blue;    // Range from -100 to 100 (%)

    // For Midtones (medium brightness areas)
    int midtones_cyan_red;
    int midtones_magenta_green;
    int midtones_yellow_blue;

    // For Highlights (bright areas)
    int highlights_cyan_red;
    int highlights_magenta_green;
    int highlights_yellow_blue;
} ColorTweaks;

// Previously we had SPI initialization and SPI communication functions.
// Now we introduce parallel initialization and communication functions instead.

// Parallel initialization
void parallel_master_init(TFT_t *dev, int16_t GPIO_MOSI, int16_t GPIO_SCLK, int16_t GPIO_CS,
                          int16_t GPIO_DC, int16_t GPIO_RESET, int16_t GPIO_BL,
                          int16_t GPIO_WR, int16_t GPIO_RD);

// Parallel communication functions (replacing SPI functions)
bool parallel_master_write_byte(TFT_t *dev, const uint8_t *Data, size_t DataLength);
bool parallel_master_write_command(TFT_t *dev, uint8_t cmd);
bool parallel_master_write_data_byte(TFT_t *dev, uint8_t data);
bool parallel_master_write_data_word(TFT_t *dev, uint16_t data);
bool parallel_master_write_addr(TFT_t *dev, uint16_t addr1, uint16_t addr2);
bool parallel_master_write_color(TFT_t *dev, uint16_t color, uint32_t size);
bool parallel_master_write_colors(TFT_t *dev, uint16_t *colors, uint16_t size);

// Delay function
void delayMS(int ms);

// LCD initialization
void lcdInit(TFT_t *dev, int width, int height, int offsetx, int offsety, uint8_t orientation);

// Gamma and brightness/contrast initialization
void init_gamma_values(void);
void init_brightness_contrast_values(void);

// Drawing functions
void lcdDrawPixel(TFT_t *dev, uint16_t x, uint16_t y, uint16_t color);
void lcdDrawMultiPixels(TFT_t *dev, uint16_t x, uint16_t y, uint16_t size, uint16_t *colors);
void lcdDrawBitmap(TFT_t *dev, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *data);
// We keep these prototypes as is, to adhere strictly to the request:
void lcdStartWrite(TFT_t *dev);
void lcdSetWindow(TFT_t *dev, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void lcdDrawBitmapRect(TFT_t *dev, uint16_t x, uint16_t y, uint16_t w,
                       uint16_t h, uint16_t *data);
void lcdDrawFillRect(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcdFillScreen(TFT_t *dev, uint16_t color);
void lcdDrawLine(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcdDrawRect(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcdDrawRectAngle(TFT_t *dev, uint16_t xc, uint16_t yc, uint16_t w, uint16_t h,
                      uint16_t angle, uint16_t color);
void lcdDrawTriangle(TFT_t *dev, uint16_t xc, uint16_t yc, uint16_t w, uint16_t h,
                     uint16_t angle, uint16_t color);
void lcdDrawCircle(TFT_t *dev, uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void lcdDrawFillCircle(TFT_t *dev, uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void lcdDrawRoundRect(TFT_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                      uint16_t r, uint16_t color);
void lcdDrawArrow(TFT_t *dev, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                  uint16_t w, uint16_t color);
void lcdDrawFillArrow(TFT_t *dev, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                      uint16_t w, uint16_t color);

// Color conversion
uint16_t rgb565_conv_with_color_tweaks(
    uint16_t r, uint16_t g, uint16_t b
);

// Font drawing functions
int lcdDrawChar(TFT_t *dev, FontxFile *fx, uint16_t x, uint16_t y,
                uint8_t ascii, uint16_t color);
int lcdDrawString(TFT_t *dev, FontxFile *fx, uint16_t x, uint16_t y,
                  uint8_t *ascii, uint16_t color);
int lcdDrawCode(TFT_t *dev, FontxFile *fx, uint16_t x, uint16_t y,
                uint8_t code, uint16_t color);
// int lcdDrawUTF8Char(TFT_t *dev, FontxFile *fx, uint16_t x, uint16_t y,
//                     uint8_t *utf8, uint16_t color);
// int lcdDrawUTF8String(TFT_t *dev, FontxFile *fx, uint16_t x, uint16_t y,
//                       unsigned char *utfs, uint16_t color);

// Font settings
void lcdSetFontDirection(TFT_t *dev, uint16_t dir);
void lcdSetFontFill(TFT_t *dev, uint16_t color);
void lcdUnsetFontFill(TFT_t *dev);
void lcdSetFontUnderLine(TFT_t *dev, uint16_t color);
void lcdUnsetFontUnderLine(TFT_t *dev);

// Display control functions
void lcdDisplayOff(TFT_t *dev);
void lcdDisplayOn(TFT_t *dev);
void lcdInversionOff(TFT_t *dev);
void lcdInversionOn(TFT_t *dev);
void lcdBacklightOff(TFT_t *dev);
void lcdBacklightOn(TFT_t *dev);

#endif /* MAIN_ST7796S_H_ */
