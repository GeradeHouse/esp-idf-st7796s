// gpio_led.c

#include "gpio_led.h"
#include "driver/rtc_io.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

// Define LED_PIN if not defined elsewhere
#ifndef LED_PIN
#define LED_PIN GPIO_NUM_0  // GPIO 0 is connected to the onboard RGB LED
#endif

// Define LEDC configuration constants
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (38)
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT
#define LEDC_DUTY               (0)
#define LEDC_FREQUENCY          (5000)

/**
 * @brief Configures GPIO_NUM_0 and initializes the LED using LEDC.
 */
void configure_gpio_and_led(void) {
    // Deinitialize and reset GPIO_NUM_0 (assuming it's used for some purpose)
    rtc_gpio_deinit(GPIO_NUM_0);
    gpio_reset_pin(GPIO_NUM_0);
    gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
    gpio_pullup_dis(GPIO_NUM_0);
    gpio_pulldown_dis(GPIO_NUM_0);

    // Configure LED using LEDC
    ledc_timer_config_t ledc_timer_config_struct = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer_config_struct);

    ledc_channel_config_t ledc_channel_config_struct = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = LEDC_DUTY,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel_config_struct);

    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0);
}
