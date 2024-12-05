// File: tusb_config.h

#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

// Common Configuration
#define CFG_TUSB_MCU                OPT_MCU_ESP32S3

#define BOARD_DEVICE_RHPORT_NUM     0
#define BOARD_DEVICE_RHPORT_SPEED   OPT_MODE_HIGH_SPEED

// defined by the compiler flags for flexibility
#ifndef CFG_TUSB_RHPORT0_MODE
#define CFG_TUSB_RHPORT0_MODE       (OPT_MODE_DEVICE | OPT_MODE_HIGH_SPEED)
#endif

// Use FreeRTOS in TinyUSB
#define CFG_TUSB_OS                 OPT_OS_FREERTOS

// Debug Level (set to 0 to disable debug)
#define CFG_TUSB_DEBUG              0

// Device Configuration
#define CFG_TUD_ENDPOINT0_SIZE      64

// Enable Device Classes
#define CFG_TUD_CDC                 0
#define CFG_TUD_MSC                 1
#define CFG_TUD_HID                 0
#define CFG_TUD_MIDI                0
#define CFG_TUD_VENDOR              0

// MSC Buffer Size
#define CFG_TUD_MSC_EP_BUFSIZE      512

#endif /* _TUSB_CONFIG_H_ */
