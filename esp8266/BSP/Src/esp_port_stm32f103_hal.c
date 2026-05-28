#include "esp_port_stm32f103_hal.h"
#include "esp_ring_buffer.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_uart.h"
#include <stdbool.h>

static esp_port_stm32f103_hal_config_t s_config;
static esp_ring_buffer_t s_rx_rb;
static uint8_t s_rx_byte;
static bool s_initialized = false;

static uint16_t stm32_hal_write (const void *data, uint16_t size, uint32_t timeout_ms) {
	if (!s_initialized || !data || size == 0) {
        return 0;
    }

	HAL_StatusTypeDef status = HAL_UART_Transmit(s_config.uart_handle,
	(uint8_t*)data,size,timeout_ms);
	
	if(status != HAL_OK){
		return 0;
	}

	return size;
}

static uint16_t stm32_hal_read(void *data, uint16_t size) {
    if (!s_initialized || !data || size == 0) {
        return 0;
    }

    return esp_ring_buffer_read(&s_rx_rb, (uint8_t *)data, size);
}

static uint16_t stm32_hal_available(void) {
    if (!s_initialized) {
        return 0;
    }

    return esp_ring_buffer_size(&s_rx_rb);
}

static uint32_t stm32_hal_millis(void){
	return HAL_GetTick();
}

static void stm32_hal_delay_ms(uint32_t ms){
	HAL_Delay(ms);
}

static void stm32_hal_set_reset(bool active){
	HAL_GPIO_WritePin(s_config.reset_gpio_port, s_config.reset_gpio_pin, (active == true) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void stm32_hal_set_enable(bool enable){
	HAL_GPIO_WritePin(s_config.enable_gpio_port, s_config.enable_gpio_pin, (enable == true) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool esp_port_stm32f103_hal_init(const esp_port_stm32f103_hal_config_t *config ,esp_port_t *out_port){
	s_initialized = false;
	
	if(!config || !out_port) {
		return false;
	}
	
	if(!config->uart_handle) {
		return false;
	}

	if(!config->rx_buffer || (config->rx_buffer_size < 64)) {
		return false;
	}

	if(!config->enable_gpio_port || !config->reset_gpio_port) {
		return false;
	}
	if((config->enable_gpio_pin == 0) || (config->reset_gpio_pin == 0)) {
		return false;
	}

	s_config = *config;

	esp_ring_buffer_status_t rb_status = esp_ring_buffer_init(&s_rx_rb, s_config.rx_buffer, s_config.rx_buffer_size);
	if(rb_status != ESP_RING_BUFFER_OK) {
		return false;
	}

	out_port->write 		= stm32_hal_write;
	out_port->read			= stm32_hal_read;
	out_port->available		= stm32_hal_available;
	out_port->millis		= stm32_hal_millis;
	out_port->delay_ms		= stm32_hal_delay_ms;
	out_port->set_enable	= stm32_hal_set_enable;
	out_port->set_reset		= stm32_hal_set_reset;

	out_port->set_enable(true);
	out_port->set_reset(false);

	HAL_StatusTypeDef status = HAL_UART_Receive_IT(s_config.uart_handle, &s_rx_byte, 1);
	if(status != HAL_OK) {
		return false;
	}

	s_initialized = true;
	return true;
}