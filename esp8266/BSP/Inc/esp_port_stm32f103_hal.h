#ifndef ESP_PORT_STM32F103_HAL_H
#define ESP_PORT_STM32F103_HAL_H

#include <stdint.h>
#include <stdbool.h>

#include "esp_cmsis_compat.h"
#include "stm32f1xx_hal.h"
#include "esp_port.h"


#ifdef __cplusplus // اگر سی ++ باشد
extern "C" { // شروع سی ++
#endif// پایان سی ++

typedef struct {
    UART_HandleTypeDef *uart_handle;
    GPIO_TypeDef *reset_gpio_port;
    uint16_t reset_gpio_pin;
    GPIO_TypeDef *enable_gpio_port;
    uint16_t enable_gpio_pin;
    uint8_t *rx_buffer; //حافظه‌ای که port برای ذخیره‌ی بایت‌های دریافتی UART استفاده می‌کند.
    uint16_t rx_buffer_size;
} esp_port_stm32f103_hal_config_t;

bool esp_port_stm32f103_hal_init(const esp_port_stm32f103_hal_config_t *config ,esp_port_t *out_port);

#ifdef __cplusplus // اگر سی ++ باشد 
} // پایان سی ++
#endif // پایان سی ++
#endif // ESP_PORT_STM32F103_HAL_H // پایان فایل