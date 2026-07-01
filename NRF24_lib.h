/*
 * NRF24_lib.h
 * STM32 HAL nRF24L01+ driver.
 */

#ifndef NRF24_LIBRARY_NRF24_LIB_H_
#define NRF24_LIBRARY_NRF24_LIB_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "stm32h7xx_hal.h"
#include "NRF24_lib_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NRF24_MAX_PAYLOAD_SIZE 32U
#define NRF24_MAX_CHANNEL      125U
#define NRF24_MAX_ADDRESS_LEN  5U

#define NRF24_REG_CONFIG      0x00U
#define NRF24_REG_EN_AA       0x01U
#define NRF24_REG_EN_RXADDR   0x02U
#define NRF24_REG_SETUP_AW    0x03U
#define NRF24_REG_SETUP_RETR  0x04U
#define NRF24_REG_RF_CH       0x05U
#define NRF24_REG_RF_SETUP    0x06U
#define NRF24_REG_STATUS      0x07U
#define NRF24_REG_OBSERVE_TX  0x08U
#define NRF24_REG_RPD         0x09U
#define NRF24_REG_RX_ADDR_P0  0x0AU
#define NRF24_REG_RX_ADDR_P1  0x0BU
#define NRF24_REG_RX_ADDR_P2  0x0CU
#define NRF24_REG_RX_ADDR_P3  0x0DU
#define NRF24_REG_RX_ADDR_P4  0x0EU
#define NRF24_REG_RX_ADDR_P5  0x0FU
#define NRF24_REG_TX_ADDR     0x10U
#define NRF24_REG_RX_PW_P0    0x11U
#define NRF24_REG_RX_PW_P1    0x12U
#define NRF24_REG_RX_PW_P2    0x13U
#define NRF24_REG_RX_PW_P3    0x14U
#define NRF24_REG_RX_PW_P4    0x15U
#define NRF24_REG_RX_PW_P5    0x16U
#define NRF24_REG_FIFO_STATUS 0x17U
#define NRF24_REG_DYNPD       0x1CU
#define NRF24_REG_FEATURE     0x1DU

#define NRF24_STATUS_RX_DR    0x40U
#define NRF24_STATUS_TX_DS    0x20U
#define NRF24_STATUS_MAX_RT   0x10U
#define NRF24_STATUS_IRQ_MASK (NRF24_STATUS_RX_DR | NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT)

#define NRF24_FIFO_RX_EMPTY   0x01U
#define NRF24_FIFO_RX_FULL    0x02U
#define NRF24_FIFO_TX_EMPTY   0x10U
#define NRF24_FIFO_TX_FULL    0x20U

#define NRF24_PIPE_ALL        0x3FU
#define NRF24_PIPE_0          0x01U
#define NRF24_PIPE_1          0x02U
#define NRF24_PIPE_2          0x04U
#define NRF24_PIPE_3          0x08U
#define NRF24_PIPE_4          0x10U
#define NRF24_PIPE_5          0x20U

typedef enum {
    NRF24_OK = 0,
    NRF24_ERROR,
    NRF24_ERROR_HAL,
    NRF24_ERROR_INVALID_PARAM,
    NRF24_ERROR_TIMEOUT,
    NRF24_ERROR_MAX_RT,
    NRF24_ERROR_NO_DATA,
    NRF24_ERROR_NOT_PRESENT
} NRF24_StatusTypeDef;

typedef enum {
    NRF24_MODE_POWER_DOWN = 0,
    NRF24_MODE_STANDBY,
    NRF24_MODE_RX,
    NRF24_MODE_TX
} NRF24_ModeTypeDef;

typedef enum {
    NRF24_DATA_RATE_250KBPS = 0,
    NRF24_DATA_RATE_1MBPS,
    NRF24_DATA_RATE_2MBPS
} NRF24_DataRateTypeDef;

typedef enum {
    NRF24_PA_NEG18DBM = 0,
    NRF24_PA_NEG12DBM,
    NRF24_PA_NEG6DBM,
    NRF24_PA_0DBM
} NRF24_PowerTypeDef;

typedef enum {
    NRF24_CRC_DISABLED = 0,
    NRF24_CRC_1_BYTE,
    NRF24_CRC_2_BYTES
} NRF24_CrcTypeDef;

typedef struct {
    uint8_t channel;
    uint8_t payload_size;
    uint8_t address_width;
    uint8_t auto_ack_pipes;
    uint8_t rx_pipes;
    uint8_t retransmit_delay_250us;
    uint8_t retransmit_count;
    NRF24_DataRateTypeDef data_rate;
    NRF24_PowerTypeDef power;
    NRF24_CrcTypeDef crc;
    bool dynamic_payloads;
    bool ack_payloads;
} NRF24_ConfigTypeDef;

typedef struct {
    SPI_HandleTypeDef *hspi;
    TIM_HandleTypeDef *htim;
    GPIO_TypeDef *ce_port;
    uint16_t ce_pin;
    GPIO_TypeDef *csn_port;
    uint16_t csn_pin;
    uint32_t spi_timeout_ms;
    NRF24_ConfigTypeDef config;
    uint8_t status;
    uint8_t last_pipe;
    NRF24_ModeTypeDef mode;
} NRF24_HandleTypeDef;

void NRF24_DelayUs(uint32_t us);


void NRF24_GetDefaultConfig(NRF24_ConfigTypeDef *config);
NRF24_StatusTypeDef NRF24_Init(NRF24_HandleTypeDef *hnrf, const NRF24_ConfigTypeDef *config);
NRF24_StatusTypeDef NRF24_DeInit(NRF24_HandleTypeDef *hnrf);
NRF24_StatusTypeDef NRF24_IsPresent(NRF24_HandleTypeDef *hnrf);

NRF24_StatusTypeDef NRF24_ReadRegister(NRF24_HandleTypeDef *hnrf, uint8_t reg, uint8_t *value);
NRF24_StatusTypeDef NRF24_WriteRegister(NRF24_HandleTypeDef *hnrf, uint8_t reg, uint8_t value);
NRF24_StatusTypeDef NRF24_ReadRegisterMulti(NRF24_HandleTypeDef *hnrf, uint8_t reg, uint8_t *data, uint8_t len);
NRF24_StatusTypeDef NRF24_WriteRegisterMulti(NRF24_HandleTypeDef *hnrf, uint8_t reg, const uint8_t *data, uint8_t len);
uint8_t NRF24_GetStatus(NRF24_HandleTypeDef *hnrf);
NRF24_StatusTypeDef NRF24_ClearIrqFlags(NRF24_HandleTypeDef *hnrf, uint8_t flags);

NRF24_StatusTypeDef NRF24_SetChannel(NRF24_HandleTypeDef *hnrf, uint8_t channel);
NRF24_StatusTypeDef NRF24_SetPayloadSize(NRF24_HandleTypeDef *hnrf, uint8_t pipe, uint8_t size);
NRF24_StatusTypeDef NRF24_SetAddressWidth(NRF24_HandleTypeDef *hnrf, uint8_t width);
NRF24_StatusTypeDef NRF24_SetDataRate(NRF24_HandleTypeDef *hnrf, NRF24_DataRateTypeDef rate);
NRF24_StatusTypeDef NRF24_SetPALevel(NRF24_HandleTypeDef *hnrf, NRF24_PowerTypeDef power);
NRF24_StatusTypeDef NRF24_SetCrc(NRF24_HandleTypeDef *hnrf, NRF24_CrcTypeDef crc);
NRF24_StatusTypeDef NRF24_SetAutoAck(NRF24_HandleTypeDef *hnrf, uint8_t pipe_mask);
NRF24_StatusTypeDef NRF24_SetRxPipes(NRF24_HandleTypeDef *hnrf, uint8_t pipe_mask);
NRF24_StatusTypeDef NRF24_SetRetries(NRF24_HandleTypeDef *hnrf, uint8_t delay_250us, uint8_t count);

NRF24_StatusTypeDef NRF24_OpenWritingPipe(NRF24_HandleTypeDef *hnrf, const uint8_t *address, uint8_t len);
NRF24_StatusTypeDef NRF24_OpenReadingPipe(NRF24_HandleTypeDef *hnrf, uint8_t pipe, const uint8_t *address, uint8_t len);
NRF24_StatusTypeDef NRF24_StartListening(NRF24_HandleTypeDef *hnrf);
NRF24_StatusTypeDef NRF24_StopListening(NRF24_HandleTypeDef *hnrf);
NRF24_StatusTypeDef NRF24_PowerUp(NRF24_HandleTypeDef *hnrf);
NRF24_StatusTypeDef NRF24_PowerDown(NRF24_HandleTypeDef *hnrf);

NRF24_StatusTypeDef NRF24_FlushTx(NRF24_HandleTypeDef *hnrf);
NRF24_StatusTypeDef NRF24_FlushRx(NRF24_HandleTypeDef *hnrf);
bool NRF24_Available(NRF24_HandleTypeDef *hnrf, uint8_t *pipe);
NRF24_StatusTypeDef NRF24_Read(NRF24_HandleTypeDef *hnrf, void *buffer, uint8_t len);
NRF24_StatusTypeDef NRF24_Write(NRF24_HandleTypeDef *hnrf, const void *buffer, uint8_t len, uint32_t timeout_ms);
NRF24_StatusTypeDef NRF24_WriteNoAck(NRF24_HandleTypeDef *hnrf, const void *buffer, uint8_t len, uint32_t timeout_ms);
NRF24_StatusTypeDef NRF24_GetDynamicPayloadSize(NRF24_HandleTypeDef *hnrf, uint8_t *size);
NRF24_StatusTypeDef NRF24_WriteAckPayload(NRF24_HandleTypeDef *hnrf, uint8_t pipe, const void *buffer, uint8_t len);

uint8_t NRF24_ReadFifoStatus(NRF24_HandleTypeDef *hnrf);
uint8_t NRF24_ReadObserveTx(NRF24_HandleTypeDef *hnrf);
uint8_t NRF24_ReadRpd(NRF24_HandleTypeDef *hnrf);
const char *NRF24_StatusToString(NRF24_StatusTypeDef status);

/* Backward-compatible lower-case entry point. */
int nrf_init(void);

#ifdef __cplusplus
}
#endif

#endif /* NRF24_LIBRARY_NRF24_LIB_H_ */
