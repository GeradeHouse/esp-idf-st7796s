// main/main.c

// Set the local log level for this file to ERROR
#define LOG_LOCAL_LEVEL ESP_LOG_ERROR

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>

#include "tusb.h"
#include "tusb_config.h"

#include "tinyusb.h"
#include "tusb_msc_storage.h"

#include "class/msc/msc.h"           // Include MSC class definitions
#include "class/msc/msc_device.h"    // Include MSC device definitions

#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"

#include "driver/ledc.h"      // Include LEDC driver definitions
#include "driver/gpio.h"      // Include GPIO driver definitions
#include "driver/rtc_io.h"    // Include RTC GPIO driver definitions

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_log.h"

#include <miniz.h>  // Use the updated standalone miniz.h

#include "st7796s.h"  // Updated to include the correct header file
#include "fontx.h"
#include "bmpfile.h"
#include "decode_jpeg.h"
#include "decode_png.h"
#include "pngle.h"
#include "decode_gif.h"
#include "decode_rgb565ani.h" // Include the header for RGB565ANI parser

#include "sdkconfig.h" // Ensure sdkconfig.h is included to access CONFIG_* variables

#include "msc.h"     // Include MSC class definitions

#include "class/msc/msc.h"           // Include MSC class definitions
#include "class/msc/msc_device.h"    // Include MSC device definitions

#include "esp_sleep.h" // Include sleep-related functions and types

#ifndef SCSI_SENSE_NOT_READY
#define SCSI_SENSE_NOT_READY 0x02
#endif

#ifndef SCSI_SENSE_ILLEGAL_REQUEST
#define SCSI_SENSE_ILLEGAL_REQUEST 0x05
#endif

#ifndef SCSI_SENSE_HARDWARE_ERROR
#define SCSI_SENSE_HARDWARE_ERROR 0x04
#endif

#ifndef SCSI_ASC_MEDIUM_NOT_PRESENT
#define SCSI_ASC_MEDIUM_NOT_PRESENT 0x3A
#endif

#ifndef SCSI_ASC_INVALID_COMMAND_OPERATION_CODE
#define SCSI_ASC_INVALID_COMMAND_OPERATION_CODE 0x20
#endif

#ifndef SCSI_ASC_UNRECOVERED_READ_ERROR
#define SCSI_ASC_UNRECOVERED_READ_ERROR 0x11
#endif

#ifndef SCSI_ASC_WRITE_FAULT
#define SCSI_ASC_WRITE_FAULT 0x03
#endif

#ifndef SCSI_ASCQ
#define SCSI_ASCQ 0x00
#endif

#define LED_PIN GPIO_NUM_0  // GPIO 0 is connected to the onboard RGB LED

// Define GPIOs for SD card SPI interface
#define CONFIG_SD_MOSI_GPIO   11  // Adjust these GPIO numbers as per your hardware setup
#define CONFIG_SD_MISO_GPIO   13
#define CONFIG_SD_SCLK_GPIO   12

#define INTERVAL 400  // Interval between tests in milliseconds
#define INTERVAL_LONG 2000  // Interval between tests in milliseconds

#define WAIT vTaskDelay(INTERVAL)  // Macro to simplify delay calls
#define WAIT_LONG vTaskDelay(INTERVAL_LONG)  // Macro to simplify delay calls

static const char *TAG = "ST7796S";  // Logging tag for this module
sdmmc_card_t* sdcard = NULL; // Global variable to access the SD card information

// Define orientation constants for the display
#define ORIENTATION_LANDSCAPE           0x48 // Landscape orientation
#define ORIENTATION_PORTRAIT            0x28 // Portrait orientation
#define ORIENTATION_INVERTED_LANDSCAPE  0x88 // Inverted landscape orientation
#define ORIENTATION_INVERTED_PORTRAIT   0xE8 // Inverted portrait orientation

// Map the selected orientation from menuconfig to the orientation constant
#if CONFIG_ORIENTATION_LANDSCAPE
    #define CONFIG_ORIENTATION ORIENTATION_LANDSCAPE
#elif CONFIG_ORIENTATION_PORTRAIT
    #define CONFIG_ORIENTATION ORIENTATION_PORTRAIT
#elif CONFIG_ORIENTATION_INVERTED_LANDSCAPE
    #define CONFIG_ORIENTATION ORIENTATION_INVERTED_LANDSCAPE
#elif CONFIG_ORIENTATION_INVERTED_PORTRAIT
    #define CONFIG_ORIENTATION ORIENTATION_INVERTED_PORTRAIT
#else
    #define CONFIG_ORIENTATION ORIENTATION_LANDSCAPE // Default orientation
#endif

static bool enable_usb_connection = false;  // Global flag to control USB connectivity

// Define boolean flags for each media file type
#define PLAY_JPEG       true
#define PLAY_GIF        true
#define PLAY_RGB565ANI  true
#define PLAY_BMP        false
#define PLAY_PNG        false

// Function to list files in SPIFFS directory
static void SPIFFS_Directory(char * path) {
    DIR* dir = opendir(path);
    assert(dir != NULL);
    while (true) {
        struct dirent* pe = readdir(dir);
        if (!pe) break;
        ESP_LOGI(__FUNCTION__, "d_name=%s d_ino=%d d_type=%x", pe->d_name, pe->d_ino, pe->d_type);
    }
    closedir(dir);
}

SemaphoreHandle_t sdcard_mutex; // Mutex for SD card access

TickType_t FillTest(TFT_t * dev, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

    lcdFillScreen(dev, RED);
    vTaskDelay(50);
    lcdFillScreen(dev, GREEN);
    vTaskDelay(50);
    lcdFillScreen(dev, BLUE);
    vTaskDelay(50);

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%u", (unsigned int)(diffTick * portTICK_PERIOD_MS));
    return diffTick;
}

TickType_t ColorBarTest(TFT_t * dev, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

    if (width < height) {
        uint16_t y1, y2;
        y1 = height / 3;
        y2 = (height / 3) * 2;
        lcdDrawFillRect(dev, 0, 0, width - 1, y1 - 1, RED);
        vTaskDelay(1);
        lcdDrawFillRect(dev, 0, y1, width - 1, y2 - 1, GREEN);
        vTaskDelay(1);
        lcdDrawFillRect(dev, 0, y2, width - 1, height - 1, BLUE);
    } else {
        uint16_t x1, x2;
        x1 = width / 3;
        x2 = (width / 3) * 2;
        lcdDrawFillRect(dev, 0, 0, x1 - 1, height - 1, RED);
        vTaskDelay(1);
        lcdDrawFillRect(dev, x1, 0, x2 - 1, height - 1, GREEN);
        vTaskDelay(1);
        lcdDrawFillRect(dev, x2, 0, width - 1, height - 1, BLUE);
    }

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%u", (unsigned int)(diffTick * portTICK_PERIOD_MS));
    return diffTick;
}

TickType_t ArrowTest(TFT_t * dev, FontxFile *fx, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

	// get font width & height
    uint8_t buffer[FontxGlyphBufSize];
    uint8_t fontWidth;
    uint8_t fontHeight;
    GetFontx(fx, 0, buffer, &fontWidth, &fontHeight);
	//ESP_LOGI(__FUNCTION__,"fontWidth=%d fontHeight=%d",fontWidth,fontHeight);

    uint16_t xpos;
    uint16_t ypos;
    int stlen;
    uint8_t ascii[24];
    uint16_t color;

    lcdFillScreen(dev, BLACK);

    strcpy((char *)ascii, "ST7796S");  // Updated the display text
    if (width < height) {
        xpos = ((width - fontHeight) / 2) - 1;
        ypos = (height - (strlen((char *)ascii) * fontWidth)) / 2;
        lcdSetFontDirection(dev, DIRECTION90);
    } else {
        ypos = ((height - fontHeight) / 2) - 1;
        xpos = (width - (strlen((char *)ascii) * fontWidth)) / 2;
        lcdSetFontDirection(dev, DIRECTION0);
    }
    color = WHITE;
    lcdDrawString(dev, fx, xpos, ypos, ascii, color);

    lcdSetFontDirection(dev, 0);
    color = RED;
    lcdDrawFillArrow(dev, 10, 10, 0, 0, 5, color);
    strcpy((char *)ascii, "0,0");
    lcdDrawString(dev, fx, 0, 30, ascii, color);

    color = GREEN;
    lcdDrawFillArrow(dev, width - 11, 10, width - 1, 0, 5, color);
    sprintf((char *)ascii, "%u,0", (unsigned int)(width - 1));
    stlen = strlen((char *)ascii);
    xpos = (width - 1) - (fontWidth * stlen);
    lcdDrawString(dev, fx, xpos, 30, ascii, color);

    color = GRAY;
    lcdDrawFillArrow(dev, 10, height - 11, 0, height - 1, 5, color);
    sprintf((char *)ascii, "0,%u", (unsigned int)(height - 1));
    ypos = (height - 11) - (fontHeight) + 5;
    lcdDrawString(dev, fx, 0, ypos, ascii, color);

    color = CYAN;
    lcdDrawFillArrow(dev, width - 11, height - 11, width - 1, height - 1, 5, color);
    sprintf((char *)ascii, "%u,%u", (unsigned int)(width - 1), (unsigned int)(height - 1));
    stlen = strlen((char *)ascii);
    xpos = (width - 1) - (fontWidth * stlen);
    lcdDrawString(dev, fx, xpos, ypos, ascii, color);

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%u", (unsigned int)(diffTick * portTICK_PERIOD_MS));
    return diffTick;
}

TickType_t DirectionTest(TFT_t * dev, FontxFile *fx, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

	// get font width & height
    uint8_t buffer[FontxGlyphBufSize];
    uint8_t fontWidth;
    uint8_t fontHeight;
    GetFontx(fx, 0, buffer, &fontWidth, &fontHeight);
	//ESP_LOGI(__FUNCTION__,"fontWidth=%d fontHeight=%d",fontWidth,fontHeight);

    uint16_t color;
    lcdFillScreen(dev, BLACK);
    uint8_t ascii[20];

    color = RED;
    strcpy((char *)ascii, "Direction=0");
    lcdSetFontDirection(dev, 0);
    lcdDrawString(dev, fx, 0, fontHeight - 1, ascii, color);

    color = BLUE;
    strcpy((char *)ascii, "Direction=2");
    lcdSetFontDirection(dev, 2);
    lcdDrawString(dev, fx, (width - 1), (height - 1) - (fontHeight * 1), ascii, color);

    color = CYAN;
    strcpy((char *)ascii, "Direction=1");
    lcdSetFontDirection(dev, 1);
    lcdDrawString(dev, fx, (width - 1) - fontHeight, 0, ascii, color);

    color = GREEN;
    strcpy((char *)ascii, "Direction=3");
    lcdSetFontDirection(dev, 3);
    lcdDrawString(dev, fx, (fontHeight - 1), height - 1, ascii, color);

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%u", (unsigned int)(diffTick * portTICK_PERIOD_MS));
    return diffTick;
}

TickType_t HorizontalTest(TFT_t * dev, FontxFile *fx, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

	// get font width & height
    uint8_t buffer[FontxGlyphBufSize];
    uint8_t fontWidth;
    uint8_t fontHeight;
    GetFontx(fx, 0, buffer, &fontWidth, &fontHeight);
	//ESP_LOGI(__FUNCTION__,"fontWidth=%d fontHeight=%d",fontWidth,fontHeight);

    uint16_t color;
    lcdFillScreen(dev, BLACK);
    uint8_t ascii[20];

    color = RED;
    strcpy((char *)ascii, "Direction=0");
    lcdSetFontDirection(dev, 0);
    lcdDrawString(dev, fx, 0, fontHeight * 1 - 1, ascii, color);
    lcdSetFontUnderLine(dev, RED);
    lcdDrawString(dev, fx, 0, fontHeight * 2 - 1, ascii, color);
    lcdUnsetFontUnderLine(dev);

    lcdSetFontFill(dev, GREEN);
    lcdDrawString(dev, fx, 0, fontHeight * 3 - 1, ascii, color);
    lcdSetFontUnderLine(dev, RED);
    lcdDrawString(dev, fx, 0, fontHeight * 4 - 1, ascii, color);
    lcdUnsetFontFill(dev);
    lcdUnsetFontUnderLine(dev);

    color = BLUE;
    strcpy((char *)ascii, "Direction=2");
    lcdSetFontDirection(dev, 2);
    lcdDrawString(dev, fx, width, height - (fontHeight * 1) - 1, ascii, color);
    lcdSetFontUnderLine(dev, BLUE);
    lcdDrawString(dev, fx, width, height - (fontHeight * 2) - 1, ascii, color);
    lcdUnsetFontUnderLine(dev);

    lcdSetFontFill(dev, YELLOW);
    lcdDrawString(dev, fx, width, height - (fontHeight * 3) - 1, ascii, color);
    lcdSetFontUnderLine(dev, BLUE);
    lcdDrawString(dev, fx, width, height - (fontHeight * 4) - 1, ascii, color);
    lcdUnsetFontFill(dev);
    lcdUnsetFontUnderLine(dev);

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%u", (unsigned int)(diffTick * portTICK_PERIOD_MS));
    return diffTick;
}

TickType_t VerticalTest(TFT_t * dev, FontxFile *fx, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

	// get font width & height
    uint8_t buffer[FontxGlyphBufSize];
    uint8_t fontWidth;
    uint8_t fontHeight;
    GetFontx(fx, 0, buffer, &fontWidth, &fontHeight);
	//ESP_LOGI(__FUNCTION__,"fontWidth=%d fontHeight=%d",fontWidth,fontHeight);

    uint16_t color;
    lcdFillScreen(dev, BLACK);
    uint8_t ascii[20];

    color = RED;
    strcpy((char *)ascii, "Direction=1");
    lcdSetFontDirection(dev, 1);
    lcdDrawString(dev, fx, width - (fontHeight * 1), 0, ascii, color);
    lcdSetFontUnderLine(dev, RED);
    lcdDrawString(dev, fx, width - (fontHeight * 2), 0, ascii, color);
    lcdUnsetFontUnderLine(dev);

    lcdSetFontFill(dev, GREEN);
    lcdDrawString(dev, fx, width - (fontHeight * 3), 0, ascii, color);
    lcdSetFontUnderLine(dev, RED);
    lcdDrawString(dev, fx, width - (fontHeight * 4), 0, ascii, color);
    lcdUnsetFontFill(dev);
    lcdUnsetFontUnderLine(dev);

    color = BLUE;
    strcpy((char *)ascii, "Direction=3");
    lcdSetFontDirection(dev, 3);
    lcdDrawString(dev, fx, (fontHeight * 1) - 1, height, ascii, color);
    lcdSetFontUnderLine(dev, BLUE);
    lcdDrawString(dev, fx, (fontHeight * 2) - 1, height, ascii, color);
    lcdUnsetFontUnderLine(dev);

    lcdSetFontFill(dev, YELLOW);
    lcdDrawString(dev, fx, (fontHeight * 3) - 1, height, ascii, color);
    lcdSetFontUnderLine(dev, BLUE);
    lcdDrawString(dev, fx, (fontHeight * 4) - 1, height, ascii, color);
    lcdUnsetFontFill(dev);
    lcdUnsetFontUnderLine(dev);

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%u", (unsigned int)(diffTick * portTICK_PERIOD_MS));
    return diffTick;
}

TickType_t LineTest(TFT_t * dev, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

    uint16_t color;
	//lcdFillScreen(dev, WHITE);
    lcdFillScreen(dev, BLACK);
    color = RED;
    for (int ypos = 0; ypos < height; ypos = ypos + 10) {
        lcdDrawLine(dev, 0, ypos, width, ypos, color);
        vTaskDelay(1); // Yield after each line
    }
    for (int xpos = 0; xpos < width; xpos = xpos + 10) {
        lcdDrawLine(dev, xpos, 0, xpos, height, color);
        vTaskDelay(1); // Yield after each line
    }

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%u", (unsigned int)(diffTick * portTICK_PERIOD_MS));
    return diffTick;
}

TickType_t CircleTest(TFT_t * dev, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

    uint16_t color;
	//lcdFillScreen(dev, WHITE);
    lcdFillScreen(dev, BLACK);
    color = CYAN;
    uint16_t xpos = width / 2;
    uint16_t ypos = height / 2;
    for (int i = 5; i < height; i = i + 5) {
        lcdDrawCircle(dev, xpos, ypos, i, color);
        vTaskDelay(1);
    }

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%u", (unsigned int)(diffTick * portTICK_PERIOD_MS));
    return diffTick;
}

TickType_t RectAngleTest(TFT_t * dev, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

    uint16_t color;
	//lcdFillScreen(dev, WHITE);
    lcdFillScreen(dev, BLACK);
	color = CYAN;
	uint16_t xpos = width / 2;
	uint16_t ypos = height / 2;

	uint16_t w = width * 0.6;
	uint16_t h = w * 0.5;
	int angle;
	for (angle = 0; angle <= (360 * 3); angle = angle + 30) {
		lcdDrawRectAngle(dev, xpos, ypos, w, h, angle, color);
		usleep(10000);
		lcdDrawRectAngle(dev, xpos, ypos, w, h, angle, BLACK);
        vTaskDelay(1);
    }

	for (angle = 0; angle <= 180; angle = angle + 30) {
        lcdDrawRectAngle(dev, xpos, ypos, w, h, angle, color);
        vTaskDelay(1);
    }

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%u", (unsigned int)(diffTick * portTICK_PERIOD_MS));
    return diffTick;
}

TickType_t TriangleTest(TFT_t * dev, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

    uint16_t color;
	//lcdFillScreen(dev, WHITE);
    lcdFillScreen(dev, BLACK);
    color = CYAN;
    uint16_t xpos = width / 2;
    uint16_t ypos = height / 2;

    uint16_t w = width * 0.6;
    uint16_t h = w * 1.0;
    int angle;

	for (angle = 0; angle <= (360 * 3); angle = angle + 30) {
        lcdDrawTriangle(dev, xpos, ypos, w, h, angle, color);
        usleep(10000);
        lcdDrawTriangle(dev, xpos, ypos, w, h, angle, BLACK);
        vTaskDelay(1);
    }

	for (angle = 0; angle <= 360; angle = angle + 30) {
        lcdDrawTriangle(dev, xpos, ypos, w, h, angle, color);
        vTaskDelay(1);
    }

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%u", (unsigned int)(diffTick * portTICK_PERIOD_MS));
    return diffTick;
}

TickType_t RoundRectTest(TFT_t * dev, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

    uint16_t color;
    uint16_t limit = width;
    if (width > height) limit = height;
	//lcdFillScreen(dev, WHITE);
    lcdFillScreen(dev, BLACK);
    color = BLUE;
    for (int i = 5; i < limit; i = i + 5) {
        if (i > (limit - i - 1)) break;
		//ESP_LOGI(__FUNCTION__, "i=%d, width-i-1=%d",i, width-i-1);
        lcdDrawRoundRect(dev, i, i, (width - i - 1), (height - i - 1), 10, color);
        vTaskDelay(1);
    }

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%u", (unsigned int)(diffTick * portTICK_PERIOD_MS));
    return diffTick;
}

TickType_t FillRectTest(TFT_t * dev, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

    uint16_t color;
    lcdFillScreen(dev, CYAN);

    uint16_t red;
    uint16_t green;
    uint16_t blue;
    srand( (unsigned int)time( NULL ) );
    for(int i=1;i<100;i++) {
        red=rand()%255;
        green=rand()%255;
        blue=rand()%255;
        color=rgb565_conv_with_color_tweaks(red, green, blue); // Updated function call
        uint16_t xpos=rand()%width;
        uint16_t ypos=rand()%height;
        uint16_t size=rand()%(width/5);
        lcdDrawFillRect(dev, xpos, ypos, xpos+size, ypos+size, color);
        vTaskDelay(1);
    }

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%u", (unsigned int)(diffTick * portTICK_PERIOD_MS));
    return diffTick;
}

TickType_t ColorTest(TFT_t * dev, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

    uint16_t color;
    lcdFillScreen(dev, WHITE);
    color = RED;
    uint16_t delta = height/16;
    uint16_t ypos = 0;
    for(int i=0;i<16;i++) {
		//ESP_LOGI(__FUNCTION__, "color=0x%x",color);
        lcdDrawFillRect(dev, 0, ypos, width-1, ypos+delta, color);
        color = color >> 1;
        ypos = ypos + delta;
        vTaskDelay(1);
    }

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%u", (unsigned int)(diffTick * portTICK_PERIOD_MS));
    return diffTick;
}
TickType_t BMPTest(TFT_t * dev, char * file, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

    lcdSetFontDirection(dev, 0);
    lcdFillScreen(dev, BLACK);

    // Try to acquire the mutex with zero wait time
    if (xSemaphoreTake(sdcard_mutex, 0) == pdTRUE)
    {
        // Open the requested file
        FILE* fp = fopen(file, "rb");
        if (fp == NULL) {
            ESP_LOGW(__FUNCTION__, "File not found [%s]", file);
            xSemaphoreGive(sdcard_mutex);
            return 0;
        }

        // Read BMP header
        bmpfile_t *result = (bmpfile_t*)malloc(sizeof(bmpfile_t));
        size_t ret = fread(result->header.magic, 1, 2, fp);
        assert(ret == 2);
        if (result->header.magic[0] != 'B' || result->header.magic[1] != 'M') {
            ESP_LOGW(__FUNCTION__, "File is not BMP");
            free(result);
            fclose(fp);
            xSemaphoreGive(sdcard_mutex);
            return 0;
        }
        ret = fread(&result->header.filesz, 4, 1, fp);
        assert(ret == 1);
        ESP_LOGD(__FUNCTION__, "result->header.filesz=%u", (unsigned int)result->header.filesz);
        ret = fread(&result->header.creator1, 2, 1, fp);
        assert(ret == 1);
        ret = fread(&result->header.creator2, 2, 1, fp);
        assert(ret == 1);
        ret = fread(&result->header.offset, 4, 1, fp);
        assert(ret == 1);

        // Read DIB header
        ret = fread(&result->dib.header_sz, 4, 1, fp);
        assert(ret == 1);
        ret = fread(&result->dib.width, 4, 1, fp);
        assert(ret == 1);
        ret = fread(&result->dib.height, 4, 1, fp);
        assert(ret == 1);
        ret = fread(&result->dib.nplanes, 2, 1, fp);
        assert(ret == 1);
        ret = fread(&result->dib.depth, 2, 1, fp);
        assert(ret == 1);
        ret = fread(&result->dib.compress_type, 4, 1, fp);
        assert(ret == 1);
        ret = fread(&result->dib.bmp_bytesz, 4, 1, fp);
        assert(ret == 1);
        ret = fread(&result->dib.hres, 4, 1, fp);
        assert(ret == 1);
        ret = fread(&result->dib.vres, 4, 1, fp);
        assert(ret == 1);
        ret = fread(&result->dib.ncolors, 4, 1, fp);
        assert(ret == 1);
        ret = fread(&result->dib.nimpcolors, 4, 1, fp);
        assert(ret == 1);

        // Log BMP header details
        ESP_LOGI(__FUNCTION__, "BMP Header Details:");
        ESP_LOGI(__FUNCTION__, "Width: %u, Height: %u", (unsigned int)result->dib.width, (unsigned int)result->dib.height);
        ESP_LOGI(__FUNCTION__, "Depth: %u bits, Compression Type: %u", (unsigned int)result->dib.depth, (unsigned int)result->dib.compress_type);
        ESP_LOGI(__FUNCTION__, "BMP Bytes Size: %u", (unsigned int)result->dib.bmp_bytesz);

        // Release the mutex after reading headers
        xSemaphoreGive(sdcard_mutex);

        if ((result->dib.depth == 24) && (result->dib.compress_type == 0)) {
            ESP_LOGD(__FUNCTION__, "Processing 24-bit BMP");
            // BMP rows are padded (if needed) to 4-byte boundary
            uint32_t rowSize = (result->dib.width * 3 + 3) & ~3;
            int w = result->dib.width;
            int h = result->dib.height;
            ESP_LOGD(__FUNCTION__, "w=%d h=%d", w, h);
            int _x;
            int _w;
            int _cols;
            int _cole;
            if (width >= w) {
                _x = (width - w) / 2;
                _w = w;
                _cols = 0;
                _cole = w - 1;
            } else {
                _x = 0;
                _w = width;
                _cols = (w - width) / 2;
                _cole = _cols + width - 1;
            }
            ESP_LOGD(__FUNCTION__, "_x=%d _w=%d _cols=%d _cole=%d", _x, _w, _cols, _cole);

            int _y;
            int _rows;
            int _rowe;
            if (height >= h) {
                _y = (height - h) / 2;
                _rows = 0;
                _rowe = h - 1;
            } else {
                _y = 0;
                _rows = (h - height) / 2;
                _rowe = _rows + height - 1;
            }
            ESP_LOGD(__FUNCTION__, "_y=%d _rows=%d _rowe=%d", _y, _rows, _rowe);

            #define BUFFPIXEL 20
            uint8_t sdbuffer[3 * BUFFPIXEL]; // Pixel buffer (R+G+B per pixel)
            uint16_t *colors = (uint16_t*)malloc(sizeof(uint16_t) * w);

            for (int row = 0; row < h; row++) { // For each scanline...
                if (row < _rows || row > _rowe) continue;
                // Seek to start of scan line
                // Try to acquire the mutex with zero wait time
                if (xSemaphoreTake(sdcard_mutex, 0) == pdTRUE)
                {
                    int pos = result->header.offset + (h - 1 - row) * rowSize;
                    fseek(fp, pos, SEEK_SET);
                    int buffidx = sizeof(sdbuffer); // Force buffer reload

                    int index = 0;
                    for (int col = 0; col < w; col++) { // For each pixel...
                        if (buffidx >= sizeof(sdbuffer)) { // Buffer reload
                            fread(sdbuffer, sizeof(sdbuffer), 1, fp);
                            buffidx = 0; // Reset index to beginning
                        }
                        if (col < _cols || col > _cole) continue;
                        // Convert pixel from BMP to TFT format
                        uint8_t b = sdbuffer[buffidx++];
                        uint8_t g = sdbuffer[buffidx++];
                        uint8_t r = sdbuffer[buffidx++];
                        colors[index++] = rgb565_conv_with_color_tweaks(r, g, b); // Updated function call
                    } // end for col

                    // Release the mutex after reading the row
                    xSemaphoreGive(sdcard_mutex);

                    ESP_LOGD(__FUNCTION__, "lcdDrawMultiPixels _x=%d _y=%d row=%d", _x, _y, row);
                    lcdDrawMultiPixels(dev, _x, _y, _w, colors);
                    _y++;

                    // Allow RTOS to switch tasks to prevent watchdog timeout
                    vTaskDelay(1);
                }
                else
                {
                    ESP_LOGW(__FUNCTION__, "SD card busy, skipping remaining rows");
                    break;
                }
            } // end for row

            free(colors);
        } // end if depth==24 and compress_type==0

        free(result);
        fclose(fp);
    }
    else
    {
        ESP_LOGW(__FUNCTION__, "SD card busy, skipping image: %s", file);
        return 0;
    }

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(__FUNCTION__, "Elapsed time [ms]: %u", (unsigned int)(diffTick * portTICK_PERIOD_MS));
    return diffTick;
}

TickType_t JPEGTest(TFT_t * dev, char * file, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

    lcdSetFontDirection(dev, 0);
    // lcdFillScreen(dev, BLACK);

    pixel_jpeg **pixels;
    int imageWidth;
    int imageHeight;

    // Try to acquire the mutex with zero wait time
    if (xSemaphoreTake(sdcard_mutex, 0) == pdTRUE)
    {
        esp_err_t err = decode_jpeg(&pixels, file, width, height, &imageWidth, &imageHeight);
        xSemaphoreGive(sdcard_mutex);

        ESP_LOGD(__FUNCTION__, "decode_image err=%d imageWidth=%d imageHeight=%d", err, imageWidth, imageHeight);
        if (err == ESP_OK) {

            uint16_t _width = width;
            uint16_t _cols = 0;
            if (width > imageWidth) {
                _width = imageWidth;
                _cols = (width - imageWidth) / 2;
            }
            ESP_LOGD(__FUNCTION__, "_width=%d _cols=%d", _width, _cols);

            uint16_t _height = height;
            uint16_t _rows = 0;
            if (height > imageHeight) {
                _height = imageHeight;
                _rows = (height - imageHeight) / 2;
            }
            ESP_LOGD(__FUNCTION__, "_height=%d _rows=%d", _height, _rows);
            uint16_t *colors = (uint16_t*)malloc(sizeof(uint16_t) * _width);

            for(int y = 0; y < _height; y++){
                for(int x = 0;x < _width; x++){
                    colors[x] = pixels[y][x];
                }
                lcdDrawMultiPixels(dev, _cols, y+_rows, _width, colors);
                vTaskDelay(1);
            }

            free(colors);
            release_image(&pixels, width, height);
            ESP_LOGD(__FUNCTION__, "Finish");
        } else {
            ESP_LOGE(__FUNCTION__, "decode_jpeg fail=%d", err);
        }
    }
    else
    {
        ESP_LOGW(__FUNCTION__, "SD card busy, skipping image: %s", file);
        return 0;
    }

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%u", (unsigned int)(diffTick * portTICK_PERIOD_MS));
    return diffTick;
}


TickType_t PNGTest(TFT_t * dev, char * file, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

    lcdSetFontDirection(dev, 0);
    lcdFillScreen(dev, BLACK);

    // Try to acquire the mutex with zero wait time
    if (xSemaphoreTake(sdcard_mutex, 0) == pdTRUE)
    {
        // Open PNG file
        FILE* fp = fopen(file, "rb");
        if (fp == NULL) {
            ESP_LOGW(__FUNCTION__, "File not found [%s]", file);
            xSemaphoreGive(sdcard_mutex);
            return 0;
        }

        char buf[1024];
        size_t remain = 0;
        int len;

        pngle_t *pngle = pngle_new(width, height);

        pngle_set_init_callback(pngle, png_init);
        pngle_set_draw_callback(pngle, png_draw);
        pngle_set_done_callback(pngle, png_finish);

        double display_gamma = 2.2;
        pngle_set_display_gamma(pngle, display_gamma);

        while (!feof(fp)) {
            if (remain >= sizeof(buf)) {
                ESP_LOGE(__FUNCTION__, "Buffer exceeded");
                while(1) vTaskDelay(1);
            }

            len = fread(buf + remain, 1, sizeof(buf) - remain, fp);
            if (len <= 0) {
                // printf("EOF\n");
                break;
            }

            xSemaphoreGive(sdcard_mutex); // Release the mutex during processing

            int fed = pngle_feed(pngle, buf, remain + len);
            if (fed < 0) {
                ESP_LOGE(__FUNCTION__, "ERROR; %s", pngle_error(pngle));
                while(1) vTaskDelay(1);
            }

            remain = remain + len - fed;
            if (remain > 0) memmove(buf, buf + fed, remain);

            // Try to re-acquire the mutex before next read
            if (xSemaphoreTake(sdcard_mutex, portMAX_DELAY) != pdTRUE)
            {
                ESP_LOGW(__FUNCTION__, "Failed to re-acquire SD card mutex");
                fclose(fp);
                pngle_destroy(pngle, width, height);
                return 0;
            }
        }

        fclose(fp);
        xSemaphoreGive(sdcard_mutex); // Release the mutex after file operations

        uint16_t _width = width;
        uint16_t _cols = 0;
        if (width > pngle->imageWidth) {
            _width = pngle->imageWidth;
            _cols = (width - pngle->imageWidth) / 2;
        }
        ESP_LOGD(__FUNCTION__, "_width=%d _cols=%d", _width, _cols);

        uint16_t _height = height;
        uint16_t _rows = 0;
        if (height > pngle->imageHeight) {
                _height = pngle->imageHeight;
                _rows = (height - pngle->imageHeight) / 2;
        }
        ESP_LOGD(__FUNCTION__, "_height=%d _rows=%d", _height, _rows);
        uint16_t *colors = (uint16_t*)malloc(sizeof(uint16_t) * _width);

        for(int y = 0; y < _height; y++){
            for(int x = 0;x < _width; x++){
                colors[x] = pngle->pixels[y][x];
            }
            lcdDrawMultiPixels(dev, _cols, y+_rows, _width, colors);
            vTaskDelay(1);
        }
        free(colors);
        pngle_destroy(pngle, width, height);
    }
    else
    {
        ESP_LOGW(__FUNCTION__, "SD card busy, skipping image: %s", file);
        return 0;
    }

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%u", (unsigned int)(diffTick * portTICK_PERIOD_MS));
    return diffTick;
}


TickType_t CodeTest(TFT_t * dev, FontxFile *fx, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	// get font width & height
	uint8_t buffer[FontxGlyphBufSize];
	uint8_t fontWidth;
	uint8_t fontHeight;
	GetFontx(fx, 0, buffer, &fontWidth, &fontHeight);
	//ESP_LOGI(__FUNCTION__,"fontWidth=%d fontHeight=%d",fontWidth,fontHeight);
	uint8_t xmoji = width / fontWidth;
	uint8_t ymoji = height / fontHeight;
	//ESP_LOGI(__FUNCTION__,"xmoji=%d ymoji=%d",xmoji, ymoji);


	uint16_t color;
	lcdFillScreen(dev, BLACK);
	uint8_t code;

	color = CYAN;
	lcdSetFontDirection(dev, 0);
	code = 0xA0;
	for(int y=0;y<ymoji;y++) {
		uint16_t xpos = 0;
		uint16_t ypos =  fontHeight*(y+1)-1;
		for(int x=0;x<xmoji;x++) {
			xpos = lcdDrawCode(dev, fx, xpos, ypos, code, color);
			if (code == 0xFF) break;
			code++;
		}
	}

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%u", (unsigned int)(diffTick * portTICK_PERIOD_MS));
	return diffTick;
}

TickType_t GIFTest(TFT_t * dev, char * file, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

    lcdSetFontDirection(dev, 0);
    lcdFillScreen(dev, BLACK);

    if (xSemaphoreTake(sdcard_mutex, portMAX_DELAY) == pdTRUE)
    {
        esp_err_t err = decode_gif(dev, file, width, height);
        xSemaphoreGive(sdcard_mutex);

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to decode GIF: %s", file);
            return err;
        }
    }
    else
    {
        ESP_LOGW(TAG, "SD card busy, skipping image: %s", file);
        return ESP_FAIL;
    }

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(TAG, "GIFTest elapsed time[ms]:%u", (unsigned int)(diffTick * portTICK_PERIOD_MS));
    return ESP_OK;
}

TickType_t RGB565ANITest(TFT_t * dev, char * file, int width, int height) {
    TickType_t startTick, endTick, diffTick;
    startTick = xTaskGetTickCount();

    lcdSetFontDirection(dev, 0);
    lcdFillScreen(dev, BLACK);

    if (xSemaphoreTake(sdcard_mutex, portMAX_DELAY) == pdTRUE)
    {
        esp_err_t err = play_rgb565ani(dev, file, width, height);
        xSemaphoreGive(sdcard_mutex);

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to play RGB565ANI: %s", file);
            return err;
        }
    }
    else
    {
        ESP_LOGW(TAG, "SD card busy, skipping animation: %s", file);
        return ESP_FAIL;
    }

    endTick = xTaskGetTickCount();
    diffTick = endTick - startTick;
    ESP_LOGI(TAG, "RGB565ANITest elapsed time[ms]:%u", (unsigned int)(diffTick * portTICK_PERIOD_MS));
    return ESP_OK;
}

/**
 * Main task for handling display operations and rendering images/GIFs.
 *
 * \param[in] pvParameters  Pointer to task parameters.
 */
void st7796s_task(void *pvParameters) 
{
    // Set font files
    FontxFile fx16G[2];
    FontxFile fx24G[2];
    FontxFile fx32G[2];
    FontxFile fx32L[2];
    InitFontx(fx16G, "/spiffs/ILGH16XB.FNT", ""); // 8x16Dot Gothic
    InitFontx(fx24G, "/spiffs/ILGH24XB.FNT", ""); // 12x24Dot Gothic
    InitFontx(fx32G, "/spiffs/ILGH32XB.FNT", ""); // 16x32Dot Gothic
    InitFontx(fx32L, "/spiffs/LATIN32B.FNT", ""); // 16x32Dot Latin

    FontxFile fx16M[2];
    FontxFile fx24M[2];
    FontxFile fx32M[2];
    InitFontx(fx16M, "/spiffs/ILMH16XB.FNT", ""); // 8x16Dot Mincho
    InitFontx(fx24M, "/spiffs/ILMH24XB.FNT", ""); // 12x24Dot Mincho
    InitFontx(fx32M, "/spiffs/ILMH32XB.FNT", ""); // 16x32Dot Mincho

    // Initialize the TFT display structure
    TFT_t dev;
    spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO,
                    CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
    ESP_LOGI(TAG, "SPI Master initialized");

    // Initialize the display with the orientation specified in menuconfig
    lcdInit(&dev, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY, CONFIG_ORIENTATION);
    ESP_LOGI(TAG, "LCD Initialized with orientation 0x%02X", CONFIG_ORIENTATION);

    // After lcdInit():
    lcdFillScreen(&dev, CYAN); // Just a test
    vTaskDelay(2000 / portTICK_PERIOD_MS);


    while (1) {
		// FillTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		// WAIT;

		// ColorBarTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		// WAIT;

		// ArrowTest(&dev, fx16G, CONFIG_WIDTH, CONFIG_HEIGHT);
		// WAIT;

		// LineTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		// WAIT;

		// CircleTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		// WAIT;

		// RoundRectTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		// WAIT;

		// RectAngleTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		// WAIT;

		// TriangleTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		// WAIT;

        // Try to acquire the mutex with zero wait time to check if the SD card is available
        if (xSemaphoreTake(sdcard_mutex, 0) == pdTRUE)
        {
            DIR *dir = opendir("/sdcard/");
            if (dir == NULL) {
                ESP_LOGE(TAG, "Failed to open directory /sdcard/");
                xSemaphoreGive(sdcard_mutex);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                continue;
            }

            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_type == DT_REG) {
                    char *filename = entry->d_name;
                    char filepath[265];
                    snprintf(filepath, sizeof(filepath), "/sdcard/%s", filename);

                    char *ext = strrchr(filename, '.');
                    if (ext != NULL) {
                        ext++; // Skip the dot

                        // Convert extension to lowercase for case-insensitive comparison
                        for (char *p = ext; *p; ++p) *p = tolower(*p);

                        if (strcmp(ext, "gif") == 0 && PLAY_GIF) {
                            ESP_LOGI(TAG, "Playing GIF: %s", filepath);
                            
                            // Release the mutex before processing the GIF
                            xSemaphoreGive(sdcard_mutex);
                            
                            // Play the GIF
                            esp_err_t gif_err = GIFTest(&dev, filepath, CONFIG_WIDTH, CONFIG_HEIGHT);
                            if (gif_err != ESP_OK) {
                                ESP_LOGE(TAG, "Failed to play GIF: %s", filepath);
                            }

                            // WAIT_LONG;

                            // Re-acquire the mutex to continue processing remaining files
                            if (xSemaphoreTake(sdcard_mutex, portMAX_DELAY) != pdTRUE) {
                                ESP_LOGE(TAG, "Failed to re-acquire SD card mutex");
                                break;
                            }
                            continue;

                        } else if ((strcmp(ext, "jpeg") == 0 || strcmp(ext, "jpg") == 0) && PLAY_JPEG) {
                            ESP_LOGI(TAG, "Displaying JPEG: %s", filepath);
                            
                            // Release the mutex before processing the JPEG
                            xSemaphoreGive(sdcard_mutex);
                            
                            // Display the JPEG
                            JPEGTest(&dev, filepath, CONFIG_WIDTH, CONFIG_HEIGHT);

                            WAIT_LONG;

                            // Re-acquire the mutex
                            if (xSemaphoreTake(sdcard_mutex, portMAX_DELAY) != pdTRUE) {
                                ESP_LOGE(TAG, "Failed to re-acquire SD card mutex");
                                break;
                            }
                            continue;

                        } else if (strcmp(ext, "bmp") == 0 && PLAY_BMP) {
                            ESP_LOGI(TAG, "Displaying BMP: %s", filepath);
                            
                            // Release the mutex before processing the BMP
                            xSemaphoreGive(sdcard_mutex);
                            
                            // Display the BMP
                            BMPTest(&dev, filepath, CONFIG_WIDTH, CONFIG_HEIGHT);

                            WAIT_LONG;

                            // Re-acquire the mutex
                            if (xSemaphoreTake(sdcard_mutex, portMAX_DELAY) != pdTRUE) {
                                ESP_LOGE(TAG, "Failed to re-acquire SD card mutex");
                                break;
                            }
                            continue;

                        } else if (strcmp(ext, "png") == 0 && PLAY_PNG) {
                            ESP_LOGI(TAG, "Displaying PNG: %s", filepath);
                            
                            // Release the mutex before processing the PNG
                            xSemaphoreGive(sdcard_mutex);
                            
                            // Display the PNG
                            PNGTest(&dev, filepath, CONFIG_WIDTH, CONFIG_HEIGHT);

                            WAIT_LONG;

                            // Re-acquire the mutex
                            if (xSemaphoreTake(sdcard_mutex, portMAX_DELAY) != pdTRUE) {
                                ESP_LOGE(TAG, "Failed to re-acquire SD card mutex");
                                break;
                            }
                            continue;

                        } else if (strcmp(ext, "rgb565ani") == 0 && PLAY_RGB565ANI) {
                            ESP_LOGI(TAG, "Playing RGB565ANI: %s", filepath);
                            
                            // Release the mutex before processing the RGB565ANI
                            xSemaphoreGive(sdcard_mutex);
                            
                            // Play the RGB565ANI animation
                            esp_err_t ani_err = play_rgb565ani(&dev, filepath, CONFIG_WIDTH, CONFIG_HEIGHT);
                            if (ani_err != ESP_OK) {
                                ESP_LOGE(TAG, "Failed to play RGB565ANI: %s", filepath);
                            }

                            // WAIT_LONG;

                            // Re-acquire the mutex
                            if (xSemaphoreTake(sdcard_mutex, portMAX_DELAY) != pdTRUE) {
                                ESP_LOGE(TAG, "Failed to re-acquire SD card mutex");
                                break;
                            }
                            continue;
                        }
                    }
                }
            }

            // Close directory and release mutex after processing all files
            closedir(dir);
            xSemaphoreGive(sdcard_mutex);
        }
        else
        {
            ESP_LOGW(TAG, "SD card busy, skipping image display cycle");
        }

        // vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    // never reach
    while (1) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}


void init_sdcard(void) {
    ESP_LOGI(TAG, "Initializing SD card");

    if (xSemaphoreTake(sdcard_mutex, portMAX_DELAY)) {
        sdmmc_host_t host = SDSPI_HOST_DEFAULT();
        host.slot = SPI2_HOST;

        spi_bus_config_t bus_cfg = {
            .mosi_io_num = CONFIG_SD_MOSI_GPIO,
            .miso_io_num = CONFIG_SD_MISO_GPIO,
            .sclk_io_num = CONFIG_SD_SCLK_GPIO,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 4000,
        };

        esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH_AUTO);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize SPI bus for SD card: %s", esp_err_to_name(ret));
            xSemaphoreGive(sdcard_mutex);
            return;
        }

        sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
        slot_config.gpio_cs = GPIO_NUM_46;
        slot_config.host_id = host.slot;

        const char mount_point[] = "/sdcard";
        esp_vfs_fat_mount_config_t mount_config = {
            .format_if_mount_failed = false,
            .max_files = 5,
            .allocation_unit_size = 16 * 1024
        };

        ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &sdcard);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
            xSemaphoreGive(sdcard_mutex);
            return;
        }

        // Print SD card info to verify successful initialization
        sdmmc_card_print_info(stdout, sdcard);

        // Validate SD card attributes
        if (sdcard->csd.capacity == 0 || sdcard->csd.sector_size == 0) {
            ESP_LOGE(TAG, "SD card attributes are invalid (capacity: %u, sector size: %u)",
                     (unsigned int)sdcard->csd.capacity, (unsigned int)sdcard->csd.sector_size);
        } else {
            ESP_LOGI(TAG, "SD card initialized successfully: Capacity: %u bytes, Sector Size: %u bytes",
                     (unsigned int)sdcard->csd.capacity, (unsigned int)sdcard->csd.sector_size);
        }

        xSemaphoreGive(sdcard_mutex);
    } else {
        ESP_LOGE(TAG, "Failed to obtain SD card mutex in init_sdcard");
    }
}



// USB Device Descriptors

// Device descriptor
tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200, // USB 2.0
    .bDeviceClass       = 0x00,   // Use class code 0 (each interface specifies its own class)
    .bDeviceSubClass    = 0x00,   // Subclass code 0
    .bDeviceProtocol    = 0x00,   // Protocol code 0
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0x303A, // Espressif VID
    .idProduct          = 0x4001, // PID for MSC device
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,   // Manufacturer string index
    .iProduct           = 0x02,   // Product string index
    .iSerialNumber      = 0x03,   // Serial string index

    .bNumConfigurations = 0x01
};

// Configuration descriptor
enum
{
  ITF_NUM_MSC = 0,
  ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

#define EPNUM_MSC_OUT   0x01
#define EPNUM_MSC_IN    0x81

uint8_t const desc_configuration[] =
{
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_SELF_POWERED, 100),

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),
};

// String Descriptors

// Language ID string descriptor
uint16_t const string_desc_langid[] = { (TUSB_DESC_STRING << 8 ) | 4, 0x0409 };

char const *string_desc_arr[] =
{
    "Espressif",        // 1: Manufacturer
    "ESP32S3 SDCard",   // 2: Product
    "123456",           // 3: Serial Number
};

// MSC Callbacks

// Invoked when received SCSI_CMD_INQUIRY
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
    ESP_LOGI(TAG, "tud_msc_inquiry_cb called for LUN %u", lun);

    const char vid[] = "ESP32S3";
    const char pid[] = "SDCard";
    const char rev[] = "1.0";

    memset(vendor_id, ' ', 8);
    memset(product_id, ' ', 16);
    memset(product_rev, ' ', 4);

    memcpy(vendor_id, vid, strlen(vid) > 8 ? 8 : strlen(vid));
    memcpy(product_id, pid, strlen(pid) > 16 ? 16 : strlen(pid));
    memcpy(product_rev, rev, strlen(rev) > 4 ? 4 : strlen(rev));
}

// Invoked when received Test Unit Ready command.
bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
    // ESP_LOGI(TAG, "tud_msc_test_unit_ready_cb called for LUN %u", lun);

    if (sdcard == NULL)
    {
        ESP_LOGW(TAG, "SD card not initialized in tud_msc_test_unit_ready_cb");
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, SCSI_ASC_MEDIUM_NOT_PRESENT, SCSI_ASCQ);
        return false;
    }
    return true;
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY
void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size)
{
    ESP_LOGI(TAG, "tud_msc_capacity_cb called for LUN %u", lun);

    if (sdcard != NULL)
    {
        *block_size = sdcard->csd.sector_size; // Block size is sector size
        *block_count = sdcard->csd.capacity;    // Number of blocks on the SD card
        ESP_LOGI(TAG, "SD card capacity: %u blocks, block size: %u", (unsigned int)*block_count, (unsigned int)*block_size);
    }
    else
    {
        ESP_LOGW(TAG, "SD card not initialized in tud_msc_capacity_cb");
        *block_size = 512; // Default block size
        *block_count = 0;  // No blocks available
    }
}

// Invoked when received an SCSI command not in built-in list
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize)
{
    ESP_LOGI(TAG, "tud_msc_scsi_cb called with command: 0x%02X", scsi_cmd[0]);
    // Return zero to indicate unsupported command
    return -1;
}

// Invoked when received READ10 command
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
    static uint32_t accumulated_sectors = 0;  // Tracks accumulated sectors for logging
    static uint32_t last_logged_lba = 0;      // Tracks the last LBA for the log message

    if (sdcard == NULL)
    {
        ESP_LOGW(TAG, "SD card not initialized in tud_msc_read10_cb");
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, SCSI_ASC_MEDIUM_NOT_PRESENT, SCSI_ASCQ);
        return -1; // SD card not initialized
    }

    if (xSemaphoreTake(sdcard_mutex, portMAX_DELAY))
    {
        uint32_t sector_size = sdcard->csd.sector_size;
        uint32_t n_sectors = (bufsize + sector_size - 1) / sector_size;

        // Verify card attributes
        if (sdcard->csd.capacity == 0 || sector_size == 0) {
            ESP_LOGE(TAG, "SD card attributes are invalid");
            tud_msc_set_sense(lun, SCSI_SENSE_HARDWARE_ERROR, SCSI_ASC_UNRECOVERED_READ_ERROR, SCSI_ASCQ);
            xSemaphoreGive(sdcard_mutex);
            return -1;
        }

        esp_err_t err = sdmmc_read_sectors(sdcard, buffer, lba, n_sectors);
        xSemaphoreGive(sdcard_mutex);

        if (err != ESP_OK)
        {
            // Log error with specific LBA that failed
            ESP_LOGE(TAG, "sdmmc_read_sectors failed at LBA %u (Error: 0x%x)", (unsigned int)lba, err);
            tud_msc_set_sense(lun, SCSI_SENSE_HARDWARE_ERROR, SCSI_ASC_UNRECOVERED_READ_ERROR, SCSI_ASCQ);
            return -1;
        }

        // Accumulate the sectors read and log only when crossing 1000-sector threshold
        accumulated_sectors += n_sectors;

        // Log if accumulated sectors >= 1000 or if this is the first operation
        if (accumulated_sectors >= 1000 || last_logged_lba == 0) {
            uint32_t end_lba = lba + accumulated_sectors - 1;
            ESP_LOGI(TAG, "Reading from LBA %u to LBA %u, total sectors: %u", 
                     (unsigned int)last_logged_lba, (unsigned int)end_lba, (unsigned int)accumulated_sectors);

            // Reset counters after logging
            accumulated_sectors = 0;
            last_logged_lba = end_lba + 1;  // Update for the next operation
        } else {
            // Update the last LBA without logging
            last_logged_lba = lba + n_sectors;
        }

        return bufsize; // Return the number of bytes read
    }
    else
    {
        ESP_LOGE(TAG, "Failed to obtain SD card mutex in tud_msc_read10_cb");
        tud_msc_set_sense(lun, SCSI_SENSE_HARDWARE_ERROR, SCSI_ASC_UNRECOVERED_READ_ERROR, SCSI_ASCQ);
        return -1;
    }
}

#define TAG "ST7796S_WRITE" // Update the TAG for clarity

// Invoked when received WRITE10 command
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
    // Static variables to accumulate sectors and track logging
    static uint32_t accumulated_sectors = 0;  // Tracks accumulated sectors for logging
    static uint32_t last_logged_lba = 0;      // Tracks the last LBA for the log message

    ESP_LOGD(TAG, "tud_msc_write10_cb called: lun=%u, lba=%" PRIu32 ", offset=%" PRIu32 ", bufsize=%" PRIu32,
             lun, lba, offset, bufsize);

    if (sdcard == NULL)
    {
        ESP_LOGW(TAG, "SD card not initialized in tud_msc_write10_cb");
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, SCSI_ASC_MEDIUM_NOT_PRESENT, SCSI_ASCQ);
        return -1; // SD card not initialized
    }

    if (xSemaphoreTake(sdcard_mutex, portMAX_DELAY))
    {
        uint32_t sector_size = sdcard->csd.sector_size;
        uint32_t n_sectors = (bufsize + sector_size - 1) / sector_size;

        // Verify that card attributes are valid
        if (sdcard->csd.capacity == 0 || sector_size == 0)
        {
            ESP_LOGE(TAG, "SD card attributes are invalid");
            tud_msc_set_sense(lun, SCSI_SENSE_HARDWARE_ERROR, SCSI_ASC_WRITE_FAULT, SCSI_ASCQ);
            xSemaphoreGive(sdcard_mutex);
            return -1;
        }

        uint8_t *write_ptr = buffer;
        uint32_t sectors_written = 0;

        while (sectors_written < n_sectors)
        {
            uint32_t sectors_to_write = n_sectors - sectors_written;

            // Attempt to write multiple sectors if possible
            if (sectors_to_write > 16) // Adjust batch size as needed
            {
                sectors_to_write = 16;
            }

            int retry_count = 3; // Retry failed writes up to 3 times
            esp_err_t err;

            do
            {
                err = sdmmc_write_sectors(sdcard, write_ptr, lba + sectors_written, sectors_to_write);
                if (err == ESP_OK)
                {
                    break; // Write succeeded
                }

                ESP_LOGW(TAG, "Write retry for LBA %" PRIu32 ", sectors: %" PRIu32 " (Error: 0x%x)",
                         lba + sectors_written, sectors_to_write, err);
                retry_count--;
            } while (retry_count > 0);

            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to write sectors starting at LBA %" PRIu32 " (Error: 0x%x)",
                         lba + sectors_written, err);

                // Log the error code directly since sdmmc_get_status is incorrect
                ESP_LOGE(TAG, "SD card write error: 0x%x", err);

                tud_msc_set_sense(lun, SCSI_SENSE_HARDWARE_ERROR, SCSI_ASC_WRITE_FAULT, SCSI_ASCQ);
                xSemaphoreGive(sdcard_mutex);
                return -1;
            }

            write_ptr += sectors_to_write * sector_size;
            sectors_written += sectors_to_write;
        }

        // Accumulate the sectors written and log only when crossing 1000-sector threshold
        accumulated_sectors += n_sectors;

        // Log if accumulated sectors >= 1000 or if this is the first operation
        if (accumulated_sectors >= 1000 || last_logged_lba == 0)
        {
            uint32_t end_lba = lba + accumulated_sectors - 1;
            ESP_LOGI(TAG, "Writing from LBA %" PRIu32 " to LBA %" PRIu32 ", total sectors: %" PRIu32,
                     last_logged_lba, end_lba, accumulated_sectors);

            // Reset counters after logging
            accumulated_sectors = 0;
            last_logged_lba = end_lba + 1;  // Update for the next operation
        }
        else
        {
            // Update the last LBA without logging
            last_logged_lba = lba + n_sectors;
        }

        ESP_LOGI(TAG, "Write operation completed successfully: %" PRIu32 " sectors written", n_sectors);
        xSemaphoreGive(sdcard_mutex);

        return bufsize; // Return the number of bytes written
    }
    else
    {
        ESP_LOGE(TAG, "Failed to obtain SD card mutex in tud_msc_write10_cb");
        tud_msc_set_sense(lun, SCSI_SENSE_HARDWARE_ERROR, SCSI_ASC_WRITE_FAULT, SCSI_ASCQ);
        return -1;
    }

    // To satisfy the compiler, although all paths above should return. 
    // This line should never be reached.
    return -1;
}

// Indicate if device is writable
bool tud_msc_is_writable_cb(uint8_t lun)
{
    // ESP_LOGI(TAG, "tud_msc_is_writable_cb called for LUN %" PRIu32, (uint32_t)lun);
    return true;
}





// USB Task for TinyUSB stack
void usb_task(void *param)
{
    ESP_LOGI(TAG, "Starting USB task");

    while (1)
    {
        if (enable_usb_connection) {
            ESP_LOGD(TAG, "USB task: Running tud_task()");
            tud_task(); // TinyUSB device task
        } else {
            ESP_LOGD(TAG, "USB task: USB connection disabled, yielding");
            // When USB is disabled, just yield the task
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Define GPIO number and active level for the button
#define BUTTON_GPIO GPIO_NUM_4
#define BUTTON_ACTIVE_LEVEL 0  // 0 for Active Low (button press connects GPIO to GND)

#define BUTTON_TAG "BUTTON"  // Logging tag for button functionality

// Queue handle for button events
static QueueHandle_t button_queue = NULL;

// ISR handler for the button
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    BaseType_t higher_priority_task_woken = pdFALSE;

    // Send the GPIO number to the button_queue from ISR
    if (xQueueSendFromISR(button_queue, &gpio_num, &higher_priority_task_woken) != pdTRUE) {
        ESP_LOGW(BUTTON_TAG, "Button ISR: Failed to send to queue");
    } else {
        ESP_LOGD(BUTTON_TAG, "Button ISR: GPIO %" PRIu32 " event sent to queue", gpio_num);
    }

    // Yield to higher priority task if necessary
    if (higher_priority_task_woken) {
        portYIELD_FROM_ISR();
    }
}

// Task to handle button press events
static void button_task(void* arg) {
    uint32_t io_num;
    TickType_t last_press = 0;
    const TickType_t debounce_time = pdMS_TO_TICKS(200); // 200 ms debounce

    ESP_LOGI(BUTTON_TAG, "Button task started");

    while (1) {
        if (xQueueReceive(button_queue, &io_num, portMAX_DELAY)) {
            ESP_LOGD(BUTTON_TAG, "Button task: Received event for GPIO %" PRIu32, io_num);

            TickType_t current_time = xTaskGetTickCount();
            if ((current_time - last_press) > debounce_time) {
                ESP_LOGI(BUTTON_TAG, "Button pressed on GPIO %" PRIu32 ", preparing to enter deep sleep", io_num);
                last_press = current_time;

                // Disable USB if it's enabled
                if (enable_usb_connection) {
                    enable_usb_connection = false;
                    ESP_LOGI(BUTTON_TAG, "USB connection disabled");
                }

                // Perform any other necessary cleanup here
                // Example: Turn off LEDs, save state, etc.
                ESP_LOGI(BUTTON_TAG, "Performing cleanup before deep sleep");

                // Log before entering deep sleep
                ESP_LOGI(BUTTON_TAG, "Entering deep sleep now");

                // Initiate deep sleep
                esp_deep_sleep_start();

                // If deep_sleep_start returns, log an error
                ESP_LOGE(BUTTON_TAG, "Deep sleep failed to initiate");
            } else {
                ESP_LOGI(BUTTON_TAG, "Button press on GPIO %" PRIu32 " ignored due to debounce", io_num);
            }
        }
    }
}

void app_main(void)
{
    // Suppress DEBUG and INFO logs from spi_master
    // esp_log_level_set("spi_master", ESP_LOG_ERROR);

    ESP_LOGI(TAG, "Starting app_main");

    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 12,
        .format_if_mount_failed = true
    };

    // Initialize and mount SPIFFS filesystem
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %u, used: %u", (unsigned int)total, (unsigned int)used);
    }

    // List files in the SPIFFS directory
    SPIFFS_Directory("/spiffs/");

    // Create a mutex for SD card access
    sdcard_mutex = xSemaphoreCreateMutex();
    if (sdcard_mutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create SD card mutex");
        return;
    }

    // Initialize SD card using a separate function
    init_sdcard();

    // Check wakeup reason
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
        ESP_LOGI(BUTTON_TAG, "Woke up from deep sleep by external button press");
        // Optional: Perform any actions needed after wakeup
    } else {
        ESP_LOGI(BUTTON_TAG, "Device booted normally");
    }

    // Configure the button GPIO as input with internal pull-up/down
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = (BUTTON_ACTIVE_LEVEL == 0) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = (BUTTON_ACTIVE_LEVEL == 1) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE // Interrupt on falling edge for active low
    };
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "Configured GPIO %d as input with %s pull", BUTTON_GPIO, (BUTTON_ACTIVE_LEVEL == 0) ? "pull-up" : "pull-down");

    // Create a queue to handle button events
    button_queue = xQueueCreate(10, sizeof(uint32_t));
    if (button_queue == NULL) {
        ESP_LOGE(BUTTON_TAG, "Failed to create button_queue");
        return;
    }

    // Install GPIO ISR service
    esp_err_t gpio_isr_ret = gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    if (gpio_isr_ret != ESP_OK) {
        ESP_LOGE(BUTTON_TAG, "Failed to install GPIO ISR service: %s", esp_err_to_name(gpio_isr_ret));
        return;
    }

    // Add ISR handler for the button
    gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler, (void*) BUTTON_GPIO);
    ESP_LOGI(BUTTON_TAG, "GPIO ISR handler added for GPIO %d", BUTTON_GPIO);

    // Enable wakeup on button press using EXT1 wakeup
    uint64_t wakeup_mask = (1ULL << BUTTON_GPIO);
    esp_err_t sleep_ret = esp_sleep_enable_ext1_wakeup(
        wakeup_mask,
        (BUTTON_ACTIVE_LEVEL == 0) ? ESP_EXT1_WAKEUP_ANY_LOW : ESP_EXT1_WAKEUP_ANY_HIGH
    );
    if (sleep_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable EXT1 wakeup: %s", esp_err_to_name(sleep_ret));
    } else {
        ESP_LOGI(TAG, "EXT1 wakeup enabled for GPIO %d", BUTTON_GPIO);
    }

    // Create the button handling task
    BaseType_t button_task_ret = xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
    if (button_task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button_task");
        return;
    }
    ESP_LOGI(TAG, "button_task created successfully");

    // Initialize TinyUSB only if USB connection is enabled
    if (enable_usb_connection) {
        ESP_LOGI(TAG, "Initializing TinyUSB stack");
        tinyusb_config_t tusb_cfg = {};
        ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

        // Create the USB task with highest priority
        xTaskCreate(usb_task, "usb_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);

        // Delay the display task to allow USB enumeration
        vTaskDelay(pdMS_TO_TICKS(30000));
    }

    // Disable touch functionality on GPIO0
    rtc_gpio_deinit(GPIO_NUM_0); // Deinitialize RTC functions on GPIO0

    // Configure GPIO0 as a regular GPIO pin
    gpio_reset_pin(GPIO_NUM_0);
    gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT); // Set as input
    gpio_pullup_dis(GPIO_NUM_0);  // Disable pull-up resistor
    gpio_pulldown_dis(GPIO_NUM_0); // Disable pull-down resistor

    // Initialize LEDC to control the onboard RGB LED on GPIO38
    // Define the LEDC channel and timer configurations
    #define LEDC_TIMER              LEDC_TIMER_0
    #define LEDC_MODE               LEDC_LOW_SPEED_MODE
    #define LEDC_OUTPUT_IO          (38) // GPIO38
    #define LEDC_CHANNEL            LEDC_CHANNEL_0
    #define LEDC_DUTY_RES           LEDC_TIMER_8_BIT // Set duty resolution to 8 bits
    #define LEDC_DUTY               (0) // Set duty to 0%
    #define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz

    // Prepare and then apply the LEDC timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // Prepare and then apply the LEDC channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = LEDC_DUTY, // Set duty to 0%
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);

    // Configure LED_PIN as output to control the onboard LED if necessary
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0); // Initialize LED to OFF

    // Create the display task with lower priority
    xTaskCreate(st7796s_task, "ST7796S", 1024 * 6, NULL, 2, NULL);
}
