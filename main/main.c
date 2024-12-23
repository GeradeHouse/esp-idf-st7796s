// main/main.c

// Set the local log level for this file to DEBUG to see INFO and DEBUG logs
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

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
#include "esp_random.h"         // Include for esp_random()
#include "esp_timer.h"          // Required for esp_timer_get_time()
#include "esp_sleep.h"          // Include sleep-related functions and types

#include "driver/ledc.h"        // Include GPIO driver definitions
#include "driver/rtc_io.h"      // Include RTC GPIO driver definitions

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/spi_master.h"  // For SD card SPI

#include <miniz.h>  // Use the updated standalone miniz.h

#include "fontx.h" // We keep references to fontx only if absolutely needed
#include "bmpfile.h"
#include "decode_jpeg.h"
#include "decode_png.h"
#include "pngle.h"
#include "decode_gif.h"

// Replaced old st7796s driver with esp_lcd-based references

// ADD this new include to ensure esp_lcd_new_panel_st7796() is declared:
#include "esp_lcd_st7796.h"    // Provides esp_lcd_new_panel_st7796() declaration

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_types.h"

#include "decode_rgb565ani.h"  // Our updated decode code
#include "gpio_led.h"          // Include the GPIO and LED configuration header

#include "sdkconfig.h" // Ensure sdkconfig.h is included to access CONFIG_* variables

#include "msc.h"     // Include MSC class definitions

#include "class/msc/msc.h"           // Include MSC class definitions
#include "class/msc/msc_device.h"    // Include MSC device definitions

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

static const char *TAG = "I80_ST7796";  // Logging tag for this module
sdmmc_card_t* sdcard = NULL;           // Global variable to access the SD card

static bool enable_usb_connection = false;  // Global flag to control USB connectivity

// Use the Kconfig-defined resolution
#define LCD_WIDTH        CONFIG_WIDTH    // e.g. 480
#define LCD_HEIGHT       CONFIG_HEIGHT   // e.g. 320

// Pixel clock from Kconfig
#define LCD_PIXEL_CLK_HZ CONFIG_PIXEL_CLK_HZ

// Parallel bus pins, from Kconfig
#define LCD_DC_PIN   CONFIG_DC_GPIO
#define LCD_WR_PIN   CONFIG_WR_GPIO
#define LCD_CS_PIN   CONFIG_CD_GPIO
#define LCD_RST_PIN  CONFIG_RST_GPIO
#define LCD_DATA0_PIN CONFIG_D0_GPIO
#define LCD_DATA1_PIN CONFIG_D1_GPIO
#define LCD_DATA2_PIN CONFIG_D2_GPIO
#define LCD_DATA3_PIN CONFIG_D3_GPIO
#define LCD_DATA4_PIN CONFIG_D4_GPIO
#define LCD_DATA5_PIN CONFIG_D5_GPIO
#define LCD_DATA6_PIN CONFIG_D6_GPIO
#define LCD_DATA7_PIN CONFIG_D7_GPIO

// If you want backlight pin control, use:
#define LCD_BL_PIN   CONFIG_BL_GPIO

// Freed from older tests
#define PLAY_RGB565ANI  true

static QueueHandle_t button_queue = NULL;

// Button configurations
#define BUTTON_GPIO GPIO_NUM_4
#define BUTTON_ACTIVE_LEVEL 0  // Active low button
#define BUTTON_TAG "BUTTON"

// RTC memory to store the timestamp before sleep
RTC_DATA_ATTR uint64_t sleep_start_time = 0;

SemaphoreHandle_t sdcard_mutex; // Mutex for SD card access

// Official I80 bus handle / ST7796 panel
static esp_lcd_i80_bus_handle_t i80_bus = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;

/* NEW: Turn backlight on if LCD_BL_PIN >= 0 */
static void turn_on_backlight(void)
{
    if (LCD_BL_PIN >= 0) {
        gpio_config_t bk_gpio = {
            .pin_bit_mask = (1ULL << LCD_BL_PIN),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = false,
            .pull_down_en = false,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&bk_gpio);
        gpio_set_level(LCD_BL_PIN, 1);
        ESP_LOGI(TAG, "Backlight pin %d is now ON", LCD_BL_PIN);
    } else {
        ESP_LOGW(TAG, "No backlight pin configured, skipping backlight ON");
    }
}

/* NEW: Turn backlight off if LCD_BL_PIN >= 0 */
// static void turn_off_backlight(void)
// {
//     if (LCD_BL_PIN >= 0) {
//         gpio_set_level(LCD_BL_PIN, 0);
//         ESP_LOGI(TAG, "Backlight pin %d is now OFF", LCD_BL_PIN);
//     } else {
//         ESP_LOGW(TAG, "No backlight pin configured, skipping backlight OFF");
//     }
// }

// Helper: Initialize the I80 bus + ST7796 panel
static void init_i80_st7796(void)
{
    // 1. i80 bus config
    esp_lcd_i80_bus_config_t bus_config = {
        .dc_gpio_num = LCD_DC_PIN,
        .wr_gpio_num = LCD_WR_PIN,
        .data_gpio_nums = {
            LCD_DATA0_PIN,
            LCD_DATA1_PIN,
            LCD_DATA2_PIN,
            LCD_DATA3_PIN,
            LCD_DATA4_PIN,
            LCD_DATA5_PIN,
            LCD_DATA6_PIN,
            LCD_DATA7_PIN
        },
        .bus_width = 8,
        .clk_src = LCD_CLK_SRC_DEFAULT,
        // Enough for partial-chunk approach, e.g. 160 lines
        .max_transfer_bytes = LCD_WIDTH * 162 * 2,
    };
    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));

    // 2. i80 panel io config
    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = LCD_CS_PIN,    // or -1 if permanently tied low
        .pclk_hz = LCD_PIXEL_CLK_HZ,  // pixel clock from Kconfig
        .trans_queue_depth = 10,
        .dc_levels = {
            .dc_idle_level = 0,  // DC=0 when idle
            .dc_cmd_level = 0,   // DC=0 for commands
            .dc_dummy_level = 0,
            .dc_data_level = 1,  // DC=1 for data
        },
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    esp_lcd_panel_io_handle_t io_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle));

    // 3. ST7796 panel config
    // CHANGED: Switch from RGB to BGR to fix color swap
    esp_lcd_panel_dev_config_t panel_dev_config = {
        .reset_gpio_num = LCD_RST_PIN,  // or -1 if no reset line
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        .color_space = ESP_LCD_COLOR_SPACE_BGR,  // <-- was ESP_LCD_COLOR_SPACE_RGB
#else
        .rgb_endian = LCD_RGB_ENDIAN_BGR,         // <-- was LCD_RGB_ENDIAN_RGB
#endif
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(io_handle, &panel_dev_config, &panel_handle));


    // Reset the panel
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    // Initialize it
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    // Turn on display
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // ------------------- ORIENTATION FIX -------------------
    // For a 480(W) x 320(H) panel that is currently rotated or truncated,
    // we can swap X/Y axes (rotate 90) and optionally mirror to match your hardware.
    // Below is an example rotation that might fix your issue:
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));    // rotate 90 degrees
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, true)); 
    // ^ If your display is upside down or mirrored incorrectly, toggle these booleans.
    // For example, mirror(panel_handle, true, false), or remove the mirror call if unnecessary.
    // -------------------------------------------------------

    ESP_LOGI(TAG, "ST7796 panel via I80 bus initialized successfully");

    // Optionally, reduce extremely frequent debug logs from underlying SPI driver used by I80:
    // esp_log_level_set("spi_master", ESP_LOG_ERROR);
}

// Task that scans .rgb565ani in /sdcard/ and plays them
void display_task(void *pvParameters)
{
    // Initialize the I80 + ST7796
    init_i80_st7796();

    // (Optional) Turn on backlight here if you haven't already
    turn_on_backlight();

    ESP_LOGI(TAG, "Starting to scan /sdcard for .rgb565ani...");

    while (1) {
        // Try to acquire the SD card
        if (xSemaphoreTake(sdcard_mutex, 0) == pdTRUE) {
            DIR *dir = opendir("/sdcard/");
            if (dir == NULL) {
                ESP_LOGE(TAG, "Failed to open directory /sdcard/");
                xSemaphoreGive(sdcard_mutex);
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_type == DT_REG) {
                    char *filename = entry->d_name;
                    char filepath[265];
                    snprintf(filepath, sizeof(filepath), "/sdcard/%s", filename);

                    char *ext = strrchr(filename, '.');
                    if (ext) {
                        ext++; // skip the dot
                        // convert extension to lowercase for case-insensitive
                        for (char *p = ext; *p; ++p) {
                            *p = tolower(*p);
                        }

                        // Only .rgb565ani
                        if ((strcmp(ext, "rgb565ani") == 0) && PLAY_RGB565ANI) {
                            ESP_LOGI(TAG, "Playing RGB565ANI: %s", filepath);
                            xSemaphoreGive(sdcard_mutex);

                            // We pass the panel_handle to the decode function
                            esp_err_t err = play_rgb565ani(panel_handle, filepath, LCD_WIDTH, LCD_HEIGHT);
                            if (err != ESP_OK) {
                                ESP_LOGE(TAG, "Failed to play RGB565ANI: %s", filepath);
                            }

                            if (xSemaphoreTake(sdcard_mutex, portMAX_DELAY) != pdTRUE) {
                                ESP_LOGE(TAG, "Failed to re-acquire SD card mutex");
                                break;
                            }
                            continue;
                        }
                    }
                }
            }

            closedir(dir);
            xSemaphoreGive(sdcard_mutex);
        } else {
            ESP_LOGW(TAG, "SD card busy, skipping .rgb565ani cycle");
        }

        // Let other tasks run
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    // Not reached
    vTaskDelete(NULL);
}

// SD card init code
static void init_sdcard(void)
{
    ESP_LOGI(TAG, "Initializing SD card");
    if (xSemaphoreTake(sdcard_mutex, portMAX_DELAY)) {
        sdmmc_host_t host = SDSPI_HOST_DEFAULT();
        host.slot = SPI2_HOST;

        // Increased max_transfer_sz from 4000 to a larger value, e.g. 64KB
        spi_bus_config_t bus_cfg = {
            .mosi_io_num = CONFIG_SD_MOSI_GPIO,
            .miso_io_num = CONFIG_SD_MISO_GPIO,
            .sclk_io_num = CONFIG_SD_SCLK_GPIO,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 64 * 1024  // was 4000
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

        sdmmc_card_print_info(stdout, sdcard);
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

tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0x303A,
    .idProduct          = 0x4001,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

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
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_SELF_POWERED, 100),
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),
};

uint16_t const string_desc_langid[] = { (TUSB_DESC_STRING << 8 ) | 4, 0x0409 };

char const *string_desc_arr[] =
{
    "Espressif",
    "ESP32S3 SDCard",
    "123456",
};

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

bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
    if (sdcard == NULL)
    {
        ESP_LOGW(TAG, "SD card not initialized in tud_msc_test_unit_ready_cb");
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, SCSI_ASC_MEDIUM_NOT_PRESENT, SCSI_ASCQ);
        return false;
    }
    return true;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size)
{
    ESP_LOGI(TAG, "tud_msc_capacity_cb called for LUN %u", lun);

    if (sdcard != NULL)
    {
        *block_size = sdcard->csd.sector_size;
        *block_count = sdcard->csd.capacity;
        ESP_LOGI(TAG, "SD card capacity: %u blocks, block size: %u", (unsigned int)*block_count, (unsigned int)*block_size);
    }
    else
    {
        ESP_LOGW(TAG, "SD card not initialized in tud_msc_capacity_cb");
        *block_size = 512;
        *block_count = 0;
    }
}

int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize)
{
    ESP_LOGI(TAG, "tud_msc_scsi_cb called with command: 0x%02X", scsi_cmd[0]);
    return -1;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
    static uint32_t accumulated_sectors = 0;
    static uint32_t last_logged_lba = 0;

    if (sdcard == NULL)
    {
        ESP_LOGW(TAG, "SD card not initialized in tud_msc_read10_cb");
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, SCSI_ASC_MEDIUM_NOT_PRESENT, SCSI_ASCQ);
        return -1;
    }

    if (xSemaphoreTake(sdcard_mutex, portMAX_DELAY))
    {
        uint32_t sector_size = sdcard->csd.sector_size;
        uint32_t n_sectors = (bufsize + sector_size - 1) / sector_size;

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
            ESP_LOGE(TAG, "sdmmc_read_sectors failed at LBA %u (Error: 0x%x)", (unsigned int)lba, err);
            tud_msc_set_sense(lun, SCSI_SENSE_HARDWARE_ERROR, SCSI_ASC_UNRECOVERED_READ_ERROR, SCSI_ASCQ);
            return -1;
        }

        accumulated_sectors += n_sectors;

        if (accumulated_sectors >= 1000 || last_logged_lba == 0) {
            uint32_t end_lba = lba + accumulated_sectors - 1;
            ESP_LOGI(TAG, "Reading from LBA %u to LBA %u, total sectors: %u",
                     (unsigned int)last_logged_lba, (unsigned int)end_lba, (unsigned int)accumulated_sectors);

            accumulated_sectors = 0;
            last_logged_lba = end_lba + 1;
        } else {
            last_logged_lba = lba + n_sectors;
        }

        return bufsize;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to obtain SD card mutex in tud_msc_read10_cb");
        tud_msc_set_sense(lun, SCSI_SENSE_HARDWARE_ERROR, SCSI_ASC_UNRECOVERED_READ_ERROR, SCSI_ASCQ);
        return -1;
    }
}

#define TAG "ST7796S_WRITE"

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
    static uint32_t accumulated_sectors = 0;
    static uint32_t last_logged_lba = 0;

    ESP_LOGD(TAG, "tud_msc_write10_cb called: lun=%u, lba=%" PRIu32 ", offset=%" PRIu32 ", bufsize=%" PRIu32,
             lun, lba, offset, bufsize);

    if (sdcard == NULL)
    {
        ESP_LOGW(TAG, "SD card not initialized in tud_msc_write10_cb");
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, SCSI_ASC_MEDIUM_NOT_PRESENT, SCSI_ASCQ);
        return -1;
    }

    if (xSemaphoreTake(sdcard_mutex, portMAX_DELAY))
    {
        uint32_t sector_size = sdcard->csd.sector_size;
        uint32_t n_sectors = (bufsize + sector_size - 1) / sector_size;

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
            if (sectors_to_write > 16)
            {
                sectors_to_write = 16;
            }

            int retry_count = 3;
            esp_err_t err;

            do
            {
                err = sdmmc_write_sectors(sdcard, write_ptr, lba + sectors_written, sectors_to_write);
                if (err == ESP_OK)
                {
                    break;
                }

                ESP_LOGW(TAG, "Write retry for LBA %" PRIu32 ", sectors: %" PRIu32 " (Error: 0x%x)",
                         lba + sectors_written, sectors_to_write, err);
                retry_count--;
            } while (retry_count > 0);

            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to write sectors starting at LBA %" PRIu32 " (Error: 0x%x)",
                         lba + sectors_written, err);
                ESP_LOGE(TAG, "SD card write error: 0x%x", err);

                tud_msc_set_sense(lun, SCSI_SENSE_HARDWARE_ERROR, SCSI_ASC_WRITE_FAULT, SCSI_ASCQ);
                xSemaphoreGive(sdcard_mutex);
                return -1;
            }

            write_ptr += sectors_to_write * sector_size;
            sectors_written += sectors_to_write;
        }

        accumulated_sectors += n_sectors;

        if (accumulated_sectors >= 1000 || last_logged_lba == 0)
        {
            uint32_t end_lba = lba + accumulated_sectors - 1;
            ESP_LOGI(TAG, "Writing from LBA %" PRIu32 " to LBA %" PRIu32 ", total sectors: %" PRIu32,
                     last_logged_lba, end_lba, accumulated_sectors);

            accumulated_sectors = 0;
            last_logged_lba = end_lba + 1;
        }
        else
        {
            last_logged_lba = lba + n_sectors;
        }

        ESP_LOGI(TAG, "Write operation completed successfully: %" PRIu32 " sectors written", n_sectors);
        xSemaphoreGive(sdcard_mutex);

        return bufsize;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to obtain SD card mutex in tud_msc_write10_cb");
        tud_msc_set_sense(lun, SCSI_SENSE_HARDWARE_ERROR, SCSI_ASC_WRITE_FAULT, SCSI_ASCQ);
        return -1;
    }

    return -1;
}

bool tud_msc_is_writable_cb(uint8_t lun)
{
    return true;
}

void usb_task(void *param)
{
    ESP_LOGI(TAG, "Starting USB task");

    while (1)
    {
        if (enable_usb_connection) {
            ESP_LOGD(TAG, "USB task: Running tud_task()");
            tud_task();
        } else {
            ESP_LOGD(TAG, "USB task: USB connection disabled, yielding");
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

const char* get_wakeup_cause_str(esp_sleep_wakeup_cause_t cause) {
    switch (cause) {
        case ESP_SLEEP_WAKEUP_EXT0:
            return "EXT0 (External signal using RTC_IO)";
        case ESP_SLEEP_WAKEUP_EXT1:
            return "EXT1 (External signals using RTC_CNTL)";
        case ESP_SLEEP_WAKEUP_TIMER:
            return "TIMER (RTC Timer)";
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            return "TOUCHPAD (Touchpad)";
        case ESP_SLEEP_WAKEUP_ULP:
            return "ULP (ULP coprocessor)";
        case ESP_SLEEP_WAKEUP_GPIO:
            return "GPIO (Light-sleep only)";
        case ESP_SLEEP_WAKEUP_UART:
            return "UART (Light-sleep only)";
        case ESP_SLEEP_WAKEUP_WIFI:
            return "WIFI (Light-sleep only)";
        case ESP_SLEEP_WAKEUP_COCPU:
            return "COCPU (Light-sleep only)";
        case ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG:
            return "COCPU_TRAP_TRIG (Light-sleep only)";
        case ESP_SLEEP_WAKEUP_BT:
            return "BT (Light-sleep only)";
        case ESP_SLEEP_WAKEUP_ALL:
            return "ALL (All wakeup sources disabled)";
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            return "UNDEFINED (No wakeup source)";
    }
}

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    BaseType_t higher_priority_task_woken = pdFALSE;

    if (xQueueSendFromISR(button_queue, &gpio_num, &higher_priority_task_woken) != pdTRUE) {
        // queue is full or error
    }

    if (higher_priority_task_woken) {
        portYIELD_FROM_ISR();
    }
}

static void button_task(void* arg) {
    uint32_t io_num;
    TickType_t last_press = 0;
    const TickType_t debounce_time = pdMS_TO_TICKS(200);

    ESP_LOGI(BUTTON_TAG, "Button task started");

    while (1) {
        ESP_LOGD(BUTTON_TAG, "Button task: Waiting for queue events");
        if (xQueueReceive(button_queue, &io_num, portMAX_DELAY)) {
            ESP_LOGD(BUTTON_TAG, "Button task: Received event for GPIO %" PRIu32, io_num);

            TickType_t current_time = xTaskGetTickCount();
            if ((current_time - last_press) > debounce_time) {
                last_press = current_time;

                vTaskDelay(pdMS_TO_TICKS(50));

                if (gpio_get_level(BUTTON_GPIO) == 1) {
                    ESP_LOGI(BUTTON_TAG, "Button release stable, preparing for deep sleep");

                    struct timeval tv_now;
                    gettimeofday(&tv_now, NULL);
                    sleep_start_time = ((uint64_t)tv_now.tv_sec * 1000000L) + (uint64_t)tv_now.tv_usec;

                    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

                    esp_err_t pd_ret = esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO);
                    if (pd_ret != ESP_OK) {
                        ESP_LOGE(BUTTON_TAG, "Failed to configure RTC_PERIPH power domain: %s", esp_err_to_name(pd_ret));
                        continue;
                    }

                    uint64_t io_mask = (1ULL << BUTTON_GPIO);

                    esp_sleep_ext1_wakeup_mode_t wakeup_mode = ESP_EXT1_WAKEUP_ANY_LOW;

                    esp_err_t err = esp_sleep_enable_ext1_wakeup(io_mask, wakeup_mode);
                    if (err != ESP_OK) {
                        ESP_LOGE(BUTTON_TAG, "Failed to enable EXT1 wakeup: %s", esp_err_to_name(err));
                        continue;
                    } else {
                        ESP_LOGI(BUTTON_TAG, "EXT1 wakeup enabled for GPIO %d, wake on ANY_LOW level", BUTTON_GPIO);
                    }

                    ESP_LOGI(BUTTON_TAG, "Entering deep sleep now");
                    esp_deep_sleep_start();

                } else {
                    ESP_LOGW(BUTTON_TAG, "Button not high after release, not sleeping");
                }

            } else {
                ESP_LOGI(BUTTON_TAG, "Button press ignored due to debounce");
            }
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting app_main");

    // SPIFFS is not strictly required for .rgb565ani, but we keep it for minimal changes
    {
        esp_vfs_spiffs_conf_t conf = {
            .base_path = "/spiffs",
            .partition_label = NULL,
            .max_files = 12,
            .format_if_mount_failed = true
        };
        esp_err_t ret = esp_vfs_spiffs_register(&conf);
        if (ret != ESP_OK) {
            if (ret == ESP_FAIL) {
                ESP_LOGE(TAG, "Failed to mount or format filesystem");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                ESP_LOGE(TAG, "Failed to find SPIFFS partition");
            } else {
                ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
            }
        } else {
            size_t total = 0, used = 0;
            ret = esp_spiffs_info(NULL, &total, &used);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
            } else {
                ESP_LOGI(TAG, "SPIFFS Partition size: total: %u, used: %u",
                         (unsigned int)total, (unsigned int)used);
            }
        }
    }

    sdcard_mutex = xSemaphoreCreateMutex();
    if (sdcard_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create SD card mutex");
        return;
    }
    init_sdcard();

    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
        ESP_LOGI(BUTTON_TAG, "Woke up from deep sleep by button release (EXT1)");

        struct timeval tv_now;
        gettimeofday(&tv_now, NULL);
        uint64_t current_time_us = ((uint64_t)tv_now.tv_sec * 1000000L) + (uint64_t)tv_now.tv_usec;
        uint64_t sleep_duration_us = current_time_us - sleep_start_time;
        double sleep_duration_sec = (double)sleep_duration_us / 1000000.0;

        ESP_LOGI(BUTTON_TAG, "Sleep duration: %.2f seconds", sleep_duration_sec);

        if (sleep_duration_sec >= 5.0) {
            ESP_LOGI(BUTTON_TAG, "Valid sleep duration, handling wake-up event");
        } else {
            ESP_LOGW(BUTTON_TAG, "Sleep duration too short, ignoring wake-up event");
            esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT1);
            esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
        }

        rtc_gpio_deinit(BUTTON_GPIO);
    } else {
        ESP_LOGI(TAG, "Device booted normally");
    }

    gpio_config_t io_conf_sleep_initial = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = (BUTTON_ACTIVE_LEVEL == 0) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = (BUTTON_ACTIVE_LEVEL == 1) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };
    gpio_config(&io_conf_sleep_initial);
    ESP_LOGI(TAG, "Configured button GPIO %d as RTC GPIO with %s pull",
             BUTTON_GPIO, (BUTTON_ACTIVE_LEVEL == 0) ? "pull-up" : "pull-down");

    if (!rtc_gpio_is_valid_gpio(BUTTON_GPIO)) {
        ESP_LOGE(BUTTON_TAG, "GPIO %d does not support RTC IO", BUTTON_GPIO);
        return;
    }

    button_queue = xQueueCreate(10, sizeof(uint32_t));
    if (button_queue == NULL) {
        ESP_LOGE(BUTTON_TAG, "Failed to create button_queue");
        return;
    }

    esp_err_t gpio_isr_ret = gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    if (gpio_isr_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install GPIO ISR service: %s", esp_err_to_name(gpio_isr_ret));
        return;
    }

    esp_err_t gpio_add_isr_ret = gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler, (void*)BUTTON_GPIO);
    if (gpio_add_isr_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add ISR handler: %s", esp_err_to_name(gpio_add_isr_ret));
        return;
    }
    ESP_LOGI(BUTTON_TAG, "GPIO ISR handler added for GPIO %d", BUTTON_GPIO);

    BaseType_t button_task_ret = xTaskCreate(button_task, "button_task", 4096, NULL, 5, NULL);
    if (button_task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button_task");
        return;
    }
    ESP_LOGI(TAG, "button_task created successfully");

    extern bool enable_usb_connection;
    if (enable_usb_connection) {
        ESP_LOGI(TAG, "Initializing TinyUSB stack");
        tinyusb_config_t tusb_cfg = {};
        ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

        xTaskCreate(usb_task, "usb_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);

        vTaskDelay(pdMS_TO_TICKS(30000));
    }

    configure_gpio_and_led();

    // Optionally turn the backlight on here (before display_task):
    turn_on_backlight();

    // Now create our display task
    xTaskCreate(display_task, "display_task", 6 * 1024, NULL, 2, NULL);
}
