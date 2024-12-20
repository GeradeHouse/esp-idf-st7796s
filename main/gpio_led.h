// gpio_led.h

#ifndef GPIO_LED_H
#define GPIO_LED_H

/**
 * @brief Configures GPIO_NUM_0 and initializes the LED using LEDC.
 *
 * This function deinitializes and resets GPIO_NUM_0, configures it as an input without pull-up/down,
 * and sets up the LED connected to GPIO_NUM_38 using the LEDC driver.
 */
void configure_gpio_and_led(void);

#endif // GPIO_LED_H
