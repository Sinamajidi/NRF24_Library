/*
 * NRF24_lib_config.h
 *
 * Hardware binding for the STM32 HAL nRF24L01+ driver.
 */

#ifndef NRF24_LIBRARY_NRF24_LIB_CONFIG_H_
#define NRF24_LIBRARY_NRF24_LIB_CONFIG_H_

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_spi.h"
#include "stm32f1xx_hal_tim.h"

#ifdef __cplusplus
extern "C" {
#endif

extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim2;

#define NRF24_SPI_HANDLE          (&hspi1)
#define NRF24_DELAY_TIMER_HANDLE  (&htim2)
#define NRF24_SPI_TIMEOUT_MS      (100U)

#define NRF24_CE_GPIO_Port        GPIOA
#define NRF24_CE_Pin              GPIO_PIN_4
#define NRF24_CSN_GPIO_Port       GPIOA
#define NRF24_CSN_Pin             GPIO_PIN_3
#define NRF24_USE_IRQ            0U
#define NRF24_IRQ_GPIO_Port       GPIOA
#define NRF24_IRQ_Pin             GPIO_PIN_2

#ifndef NRF24_DEFAULT_CHANNEL
#define NRF24_DEFAULT_CHANNEL     (76U)
#endif

#ifndef NRF24_DEFAULT_PAYLOAD_SIZE
#define NRF24_DEFAULT_PAYLOAD_SIZE (32U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* NRF24_LIBRARY_NRF24_LIB_CONFIG_H_ */
