/*
 * NRF24_lib.c
 * STM32 HAL nRF24L01+ driver.
 */

#include "NRF24_lib.h"

#define NRF24_CMD_R_REGISTER         0x00U
#define NRF24_CMD_W_REGISTER         0x20U
#define NRF24_CMD_REGISTER_MASK      0x1FU
#define NRF24_CMD_R_RX_PAYLOAD       0x61U
#define NRF24_CMD_W_TX_PAYLOAD       0xA0U
#define NRF24_CMD_FLUSH_TX           0xE1U
#define NRF24_CMD_FLUSH_RX           0xE2U
#define NRF24_CMD_REUSE_TX_PL        0xE3U
#define NRF24_CMD_R_RX_PL_WID        0x60U
#define NRF24_CMD_W_ACK_PAYLOAD      0xA8U
#define NRF24_CMD_W_TX_PAYLOAD_NOACK 0xB0U
#define NRF24_CMD_NOP                0xFFU
#define NRF24_CMD_ACTIVATE           0x50U

#define NRF24_CONFIG_MASK_RX_DR      0x40U
#define NRF24_CONFIG_MASK_TX_DS      0x20U
#define NRF24_CONFIG_MASK_MAX_RT     0x10U
#define NRF24_CONFIG_EN_CRC          0x08U
#define NRF24_CONFIG_CRCO            0x04U
#define NRF24_CONFIG_PWR_UP          0x02U
#define NRF24_CONFIG_PRIM_RX         0x01U

#define NRF24_FEATURE_EN_DPL         0x04U
#define NRF24_FEATURE_EN_ACK_PAY     0x02U
#define NRF24_FEATURE_EN_DYN_ACK     0x01U

#define NRF24_TIMING_POWERUP_US      5000U
#define NRF24_TIMING_CE_PULSE_US     15U
#define NRF24_TIMING_RX_SETTLE_US    150U

static void csn_low(NRF24_HandleTypeDef *hnrf) { HAL_GPIO_WritePin(hnrf->csn_port, hnrf->csn_pin, GPIO_PIN_RESET); }
static void csn_high(NRF24_HandleTypeDef *hnrf) { HAL_GPIO_WritePin(hnrf->csn_port, hnrf->csn_pin, GPIO_PIN_SET); }
static void ce_low(NRF24_HandleTypeDef *hnrf) { HAL_GPIO_WritePin(hnrf->ce_port, hnrf->ce_pin, GPIO_PIN_RESET); }
static void ce_high(NRF24_HandleTypeDef *hnrf) { HAL_GPIO_WritePin(hnrf->ce_port, hnrf->ce_pin, GPIO_PIN_SET); }

static NRF24_StatusTypeDef hal_status(HAL_StatusTypeDef status)
{
    if (status == HAL_OK) return NRF24_OK;
    if (status == HAL_TIMEOUT) return NRF24_ERROR_TIMEOUT;
    return NRF24_ERROR_HAL;
}

static NRF24_StatusTypeDef transfer(NRF24_HandleTypeDef *hnrf, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    HAL_StatusTypeDef ret;
    if (tx != NULL && rx != NULL) ret = HAL_SPI_TransmitReceive(hnrf->hspi, (uint8_t *)tx, rx, len, hnrf->spi_timeout_ms);
    else if (tx != NULL) ret = HAL_SPI_Transmit(hnrf->hspi, (uint8_t *)tx, len, hnrf->spi_timeout_ms);
    else ret = HAL_SPI_Receive(hnrf->hspi, rx, len, hnrf->spi_timeout_ms);
    return hal_status(ret);
}

static NRF24_StatusTypeDef command(NRF24_HandleTypeDef *hnrf, uint8_t cmd, const uint8_t *tx, uint8_t *rx, uint8_t len)
{
    NRF24_StatusTypeDef ret;
    uint8_t status = 0U;

    csn_low(hnrf);
    ret = transfer(hnrf, &cmd, &status, 1U);
    if (ret == NRF24_OK && len > 0U) ret = transfer(hnrf, tx, rx, len);
    csn_high(hnrf);

    hnrf->status = status;
    return ret;
}

void NRF24_DelayUs(uint32_t us)
{


    __HAL_TIM_SET_PRESCALER(NRF24_DELAY_TIMER_HANDLE, us - 1);
    __HAL_TIM_SET_COUNTER(NRF24_DELAY_TIMER_HANDLE, 0U);
    __HAL_TIM_CLEAR_FLAG(NRF24_DELAY_TIMER_HANDLE, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start(NRF24_DELAY_TIMER_HANDLE);
    while (__HAL_TIM_GET_FLAG(NRF24_DELAY_TIMER_HANDLE, TIM_FLAG_UPDATE) == RESET) { }
    HAL_TIM_Base_Stop(NRF24_DELAY_TIMER_HANDLE);
}



void NRF24_GetDefaultConfig(NRF24_ConfigTypeDef *config)
{
    if (config == NULL) return;
    config->channel = NRF24_DEFAULT_CHANNEL;
    config->payload_size = NRF24_DEFAULT_PAYLOAD_SIZE;
    config->address_width = 5U;
    config->auto_ack_pipes = NRF24_PIPE_ALL;
    config->rx_pipes = NRF24_PIPE_0 | NRF24_PIPE_1;
    config->retransmit_delay_250us = 5U;
    config->retransmit_count = 15U;
    config->data_rate = NRF24_DATA_RATE_1MBPS;
    config->power = NRF24_PA_0DBM;
    config->crc = NRF24_CRC_2_BYTES;
    config->dynamic_payloads = false;
    config->ack_payloads = false;
}

NRF24_StatusTypeDef NRF24_ReadRegister(NRF24_HandleTypeDef *hnrf, uint8_t reg, uint8_t *value)
{
    if (hnrf == NULL || value == NULL || reg > NRF24_CMD_REGISTER_MASK) return NRF24_ERROR_INVALID_PARAM;
    return command(hnrf, NRF24_CMD_R_REGISTER | (reg & NRF24_CMD_REGISTER_MASK), NULL, value, 1U);
}

NRF24_StatusTypeDef NRF24_WriteRegister(NRF24_HandleTypeDef *hnrf, uint8_t reg, uint8_t value)
{
    if (hnrf == NULL || reg > NRF24_CMD_REGISTER_MASK) return NRF24_ERROR_INVALID_PARAM;
    return command(hnrf, NRF24_CMD_W_REGISTER | (reg & NRF24_CMD_REGISTER_MASK), &value, NULL, 1U);
}

NRF24_StatusTypeDef NRF24_ReadRegisterMulti(NRF24_HandleTypeDef *hnrf, uint8_t reg, uint8_t *data, uint8_t len)
{
    if (hnrf == NULL || data == NULL || len == 0U || reg > NRF24_CMD_REGISTER_MASK) return NRF24_ERROR_INVALID_PARAM;
    return command(hnrf, NRF24_CMD_R_REGISTER | (reg & NRF24_CMD_REGISTER_MASK), NULL, data, len);
}

NRF24_StatusTypeDef NRF24_WriteRegisterMulti(NRF24_HandleTypeDef *hnrf, uint8_t reg, const uint8_t *data, uint8_t len)
{
    if (hnrf == NULL || data == NULL || len == 0U || reg > NRF24_CMD_REGISTER_MASK) return NRF24_ERROR_INVALID_PARAM;
    return command(hnrf, NRF24_CMD_W_REGISTER | (reg & NRF24_CMD_REGISTER_MASK), data, NULL, len);
}

uint8_t NRF24_GetStatus(NRF24_HandleTypeDef *hnrf)
{
    uint8_t cmd = NRF24_CMD_NOP, status = 0U;
    if (hnrf == NULL) return 0U;
    csn_low(hnrf);
    (void)transfer(hnrf, &cmd, &status, 1U);
    csn_high(hnrf);
    hnrf->status = status;
    return status;
}

NRF24_StatusTypeDef NRF24_ClearIrqFlags(NRF24_HandleTypeDef *hnrf, uint8_t flags)
{
    return NRF24_WriteRegister(hnrf, NRF24_REG_STATUS, flags & NRF24_STATUS_IRQ_MASK);
}

NRF24_StatusTypeDef NRF24_SetChannel(NRF24_HandleTypeDef *hnrf, uint8_t channel)
{
    if (hnrf == NULL || channel > NRF24_MAX_CHANNEL) return NRF24_ERROR_INVALID_PARAM;
    hnrf->config.channel = channel;
    return NRF24_WriteRegister(hnrf, NRF24_REG_RF_CH, channel);
}

NRF24_StatusTypeDef NRF24_SetPayloadSize(NRF24_HandleTypeDef *hnrf, uint8_t pipe, uint8_t size)
{
    if (hnrf == NULL || pipe > 5U || size > NRF24_MAX_PAYLOAD_SIZE) return NRF24_ERROR_INVALID_PARAM;
    if (size == 0U) size = 1U;
    if (pipe == 0U) hnrf->config.payload_size = size;
    return NRF24_WriteRegister(hnrf, (uint8_t)(NRF24_REG_RX_PW_P0 + pipe), size);
}

NRF24_StatusTypeDef NRF24_SetAddressWidth(NRF24_HandleTypeDef *hnrf, uint8_t width)
{
    if (hnrf == NULL || width < 3U || width > 5U) return NRF24_ERROR_INVALID_PARAM;
    hnrf->config.address_width = width;
    return NRF24_WriteRegister(hnrf, NRF24_REG_SETUP_AW, (uint8_t)(width - 2U));
}

NRF24_StatusTypeDef NRF24_SetDataRate(NRF24_HandleTypeDef *hnrf, NRF24_DataRateTypeDef rate)
{
    uint8_t rf_setup;
    NRF24_StatusTypeDef ret;
    if (hnrf == NULL) return NRF24_ERROR_INVALID_PARAM;
    ret = NRF24_ReadRegister(hnrf, NRF24_REG_RF_SETUP, &rf_setup);
    if (ret != NRF24_OK) return ret;
    rf_setup &= (uint8_t)~0x28U;
    if (rate == NRF24_DATA_RATE_250KBPS) rf_setup |= 0x20U;
    else if (rate == NRF24_DATA_RATE_2MBPS) rf_setup |= 0x08U;
    hnrf->config.data_rate = rate;
    return NRF24_WriteRegister(hnrf, NRF24_REG_RF_SETUP, rf_setup);
}

NRF24_StatusTypeDef NRF24_SetPALevel(NRF24_HandleTypeDef *hnrf, NRF24_PowerTypeDef power)
{
    uint8_t rf_setup;
    NRF24_StatusTypeDef ret;
    if (hnrf == NULL || power > NRF24_PA_0DBM) return NRF24_ERROR_INVALID_PARAM;
    ret = NRF24_ReadRegister(hnrf, NRF24_REG_RF_SETUP, &rf_setup);
    if (ret != NRF24_OK) return ret;
    rf_setup = (uint8_t)((rf_setup & ~0x06U) | ((uint8_t)power << 1U));
    hnrf->config.power = power;
    return NRF24_WriteRegister(hnrf, NRF24_REG_RF_SETUP, rf_setup);
}

NRF24_StatusTypeDef NRF24_SetCrc(NRF24_HandleTypeDef *hnrf, NRF24_CrcTypeDef crc)
{
    uint8_t cfg;
    NRF24_StatusTypeDef ret;
    if (hnrf == NULL || crc > NRF24_CRC_2_BYTES) return NRF24_ERROR_INVALID_PARAM;
    ret = NRF24_ReadRegister(hnrf, NRF24_REG_CONFIG, &cfg);
    if (ret != NRF24_OK) return ret;
    cfg &= (uint8_t)~(NRF24_CONFIG_EN_CRC | NRF24_CONFIG_CRCO);
    if (crc == NRF24_CRC_1_BYTE) cfg |= NRF24_CONFIG_EN_CRC;
    if (crc == NRF24_CRC_2_BYTES) cfg |= NRF24_CONFIG_EN_CRC | NRF24_CONFIG_CRCO;
    hnrf->config.crc = crc;
    return NRF24_WriteRegister(hnrf, NRF24_REG_CONFIG, cfg);
}

NRF24_StatusTypeDef NRF24_SetAutoAck(NRF24_HandleTypeDef *hnrf, uint8_t pipe_mask)
{
    if (hnrf == NULL) return NRF24_ERROR_INVALID_PARAM;
    hnrf->config.auto_ack_pipes = pipe_mask & NRF24_PIPE_ALL;
    return NRF24_WriteRegister(hnrf, NRF24_REG_EN_AA, hnrf->config.auto_ack_pipes);
}

NRF24_StatusTypeDef NRF24_SetRxPipes(NRF24_HandleTypeDef *hnrf, uint8_t pipe_mask)
{
    if (hnrf == NULL) return NRF24_ERROR_INVALID_PARAM;
    hnrf->config.rx_pipes = pipe_mask & NRF24_PIPE_ALL;
    return NRF24_WriteRegister(hnrf, NRF24_REG_EN_RXADDR, hnrf->config.rx_pipes);
}

NRF24_StatusTypeDef NRF24_SetRetries(NRF24_HandleTypeDef *hnrf, uint8_t delay_250us, uint8_t count)
{
    if (hnrf == NULL || delay_250us > 15U || count > 15U) return NRF24_ERROR_INVALID_PARAM;
    hnrf->config.retransmit_delay_250us = delay_250us;
    hnrf->config.retransmit_count = count;
    return NRF24_WriteRegister(hnrf, NRF24_REG_SETUP_RETR, (uint8_t)((delay_250us << 4U) | count));
}

static NRF24_StatusTypeDef set_features(NRF24_HandleTypeDef *hnrf, bool dpl, bool ack_payloads)
{
    NRF24_StatusTypeDef ret;
    uint8_t activate[] = { NRF24_CMD_ACTIVATE, 0x73U };
    uint8_t feature = 0U;

    csn_low(hnrf);
    ret = transfer(hnrf, activate, NULL, sizeof(activate));
    csn_high(hnrf);
    if (ret != NRF24_OK) return ret;

    if (dpl) feature |= NRF24_FEATURE_EN_DPL;
    if (ack_payloads) feature |= NRF24_FEATURE_EN_ACK_PAY | NRF24_FEATURE_EN_DPL;
    ret = NRF24_WriteRegister(hnrf, NRF24_REG_FEATURE, feature);
    if (ret != NRF24_OK) return ret;
    return NRF24_WriteRegister(hnrf, NRF24_REG_DYNPD, (dpl || ack_payloads) ? hnrf->config.rx_pipes : 0U);
}

NRF24_StatusTypeDef NRF24_Init(NRF24_HandleTypeDef *hnrf, const NRF24_ConfigTypeDef *config)
{
    NRF24_StatusTypeDef ret;
    NRF24_ConfigTypeDef cfg;
    if (hnrf == NULL) return NRF24_ERROR_INVALID_PARAM;

    NRF24_GetDefaultConfig(&cfg);
    if (config != NULL) cfg = *config;
    if (cfg.payload_size == 0U || cfg.payload_size > NRF24_MAX_PAYLOAD_SIZE || cfg.address_width < 3U || cfg.address_width > 5U) return NRF24_ERROR_INVALID_PARAM;

    hnrf->hspi = NRF24_SPI_HANDLE;
    hnrf->htim = NRF24_DELAY_TIMER_HANDLE;
    hnrf->ce_port = NRF24_CE_GPIO_Port;
    hnrf->ce_pin = NRF24_CE_Pin;
    hnrf->csn_port = NRF24_CSN_GPIO_Port;
    hnrf->csn_pin = NRF24_CSN_Pin;
    hnrf->spi_timeout_ms = NRF24_SPI_TIMEOUT_MS;
    hnrf->config = cfg;
    hnrf->mode = NRF24_MODE_POWER_DOWN;
    hnrf->last_pipe = 0U;

    ce_low(hnrf);
    csn_high(hnrf);
    HAL_Delay(100U);

    ret = NRF24_IsPresent(hnrf);
    if (ret != NRF24_OK) return ret;
    ret = NRF24_WriteRegister(hnrf, NRF24_REG_CONFIG, NRF24_CONFIG_EN_CRC | NRF24_CONFIG_CRCO);
    if (ret != NRF24_OK) return ret;
    ret = NRF24_SetAddressWidth(hnrf, cfg.address_width);
    if (ret != NRF24_OK) return ret;
    ret = NRF24_SetChannel(hnrf, cfg.channel);
    if (ret != NRF24_OK) return ret;
    ret = NRF24_SetRetries(hnrf, cfg.retransmit_delay_250us, cfg.retransmit_count);
    if (ret != NRF24_OK) return ret;
    ret = NRF24_SetAutoAck(hnrf, cfg.auto_ack_pipes);
    if (ret != NRF24_OK) return ret;
    ret = NRF24_SetRxPipes(hnrf, cfg.rx_pipes);
    if (ret != NRF24_OK) return ret;
    ret = NRF24_WriteRegister(hnrf, NRF24_REG_RF_SETUP, 0x06U);
    if (ret != NRF24_OK) return ret;
    ret = NRF24_SetDataRate(hnrf, cfg.data_rate);
    if (ret != NRF24_OK) return ret;
    ret = NRF24_SetPALevel(hnrf, cfg.power);
    if (ret != NRF24_OK) return ret;
    for (uint8_t pipe = 0U; pipe <= 5U; pipe++) {
        ret = NRF24_SetPayloadSize(hnrf, pipe, cfg.payload_size);
        if (ret != NRF24_OK) return ret;
    }
    ret = set_features(hnrf, cfg.dynamic_payloads, cfg.ack_payloads);
    if (ret != NRF24_OK) return ret;
    (void)NRF24_FlushRx(hnrf);
    (void)NRF24_FlushTx(hnrf);
    return NRF24_ClearIrqFlags(hnrf, NRF24_STATUS_IRQ_MASK);
}

NRF24_StatusTypeDef NRF24_DeInit(NRF24_HandleTypeDef *hnrf)
{
    if (hnrf == NULL) return NRF24_ERROR_INVALID_PARAM;
    ce_low(hnrf);
    return NRF24_PowerDown(hnrf);
}

NRF24_StatusTypeDef NRF24_IsPresent(NRF24_HandleTypeDef *hnrf)
{
    uint8_t test[5] = { 0xA5U, 0x5AU, 0xC3U, 0x3CU, 0x69U };
    uint8_t readback[5] = { 0U };
    if (hnrf == NULL) return NRF24_ERROR_INVALID_PARAM;
    if (NRF24_WriteRegisterMulti(hnrf, NRF24_REG_TX_ADDR, test, 5U) != NRF24_OK) return NRF24_ERROR_HAL;
    if (NRF24_ReadRegisterMulti(hnrf, NRF24_REG_TX_ADDR, readback, 5U) != NRF24_OK) return NRF24_ERROR_HAL;
    for (uint8_t i = 0U; i < 5U; i++) if (readback[i] != test[i]) return NRF24_ERROR_NOT_PRESENT;
    return NRF24_OK;
}

NRF24_StatusTypeDef NRF24_OpenWritingPipe(NRF24_HandleTypeDef *hnrf, const uint8_t *address, uint8_t len)
{
    NRF24_StatusTypeDef ret;
    if (hnrf == NULL || address == NULL || len < 3U || len > 5U) return NRF24_ERROR_INVALID_PARAM;
    ret = NRF24_WriteRegisterMulti(hnrf, NRF24_REG_TX_ADDR, address, len);
    if (ret != NRF24_OK) return ret;
    ret = NRF24_WriteRegisterMulti(hnrf, NRF24_REG_RX_ADDR_P0, address, len);
    if (ret != NRF24_OK) return ret;
    return NRF24_SetPayloadSize(hnrf, 0U, hnrf->config.payload_size);
}

NRF24_StatusTypeDef NRF24_OpenReadingPipe(NRF24_HandleTypeDef *hnrf, uint8_t pipe, const uint8_t *address, uint8_t len)
{
    NRF24_StatusTypeDef ret;
    if (hnrf == NULL || address == NULL || pipe > 5U || len < 1U || len > 5U) return NRF24_ERROR_INVALID_PARAM;
    if (pipe <= 1U) ret = NRF24_WriteRegisterMulti(hnrf, (uint8_t)(NRF24_REG_RX_ADDR_P0 + pipe), address, len);
    else ret = NRF24_WriteRegister(hnrf, (uint8_t)(NRF24_REG_RX_ADDR_P0 + pipe), address[0]);
    if (ret != NRF24_OK) return ret;
    ret = NRF24_SetPayloadSize(hnrf, pipe, hnrf->config.payload_size);
    if (ret != NRF24_OK) return ret;
    return NRF24_SetRxPipes(hnrf, (uint8_t)(hnrf->config.rx_pipes | (1U << pipe)));
}

NRF24_StatusTypeDef NRF24_PowerUp(NRF24_HandleTypeDef *hnrf)
{
    uint8_t cfg;
    NRF24_StatusTypeDef ret;
    if (hnrf == NULL) return NRF24_ERROR_INVALID_PARAM;
    ret = NRF24_ReadRegister(hnrf, NRF24_REG_CONFIG, &cfg);
    if (ret != NRF24_OK) return ret;
    if ((cfg & NRF24_CONFIG_PWR_UP) == 0U) {
        ret = NRF24_WriteRegister(hnrf, NRF24_REG_CONFIG, cfg | NRF24_CONFIG_PWR_UP);
        if (ret != NRF24_OK) return ret;
        NRF24_DelayUs(NRF24_TIMING_POWERUP_US);
    }
    hnrf->mode = NRF24_MODE_STANDBY;
    return NRF24_OK;
}

NRF24_StatusTypeDef NRF24_PowerDown(NRF24_HandleTypeDef *hnrf)
{
    uint8_t cfg;
    NRF24_StatusTypeDef ret;
    if (hnrf == NULL) return NRF24_ERROR_INVALID_PARAM;
    ret = NRF24_ReadRegister(hnrf, NRF24_REG_CONFIG, &cfg);
    if (ret != NRF24_OK) return ret;
    ce_low(hnrf);
    hnrf->mode = NRF24_MODE_POWER_DOWN;
    return NRF24_WriteRegister(hnrf, NRF24_REG_CONFIG, cfg & (uint8_t)~NRF24_CONFIG_PWR_UP);
}

NRF24_StatusTypeDef NRF24_StartListening(NRF24_HandleTypeDef *hnrf)
{
    uint8_t cfg;
    NRF24_StatusTypeDef ret;
    if (hnrf == NULL) return NRF24_ERROR_INVALID_PARAM;
    ret = NRF24_ReadRegister(hnrf, NRF24_REG_CONFIG, &cfg);
    if (ret != NRF24_OK) return ret;
    ret = NRF24_WriteRegister(hnrf, NRF24_REG_CONFIG, cfg | NRF24_CONFIG_PWR_UP | NRF24_CONFIG_PRIM_RX);
    if (ret != NRF24_OK) return ret;
    (void)NRF24_ClearIrqFlags(hnrf, NRF24_STATUS_IRQ_MASK);
    ce_high(hnrf);
    NRF24_DelayUs(NRF24_TIMING_RX_SETTLE_US);
    hnrf->mode = NRF24_MODE_RX;
    return NRF24_OK;
}

NRF24_StatusTypeDef NRF24_StopListening(NRF24_HandleTypeDef *hnrf)
{
    uint8_t cfg;
    NRF24_StatusTypeDef ret;
    if (hnrf == NULL) return NRF24_ERROR_INVALID_PARAM;
    ce_low(hnrf);
    ret = NRF24_ReadRegister(hnrf, NRF24_REG_CONFIG, &cfg);
    if (ret != NRF24_OK) return ret;
    ret = NRF24_WriteRegister(hnrf, NRF24_REG_CONFIG, (cfg | NRF24_CONFIG_PWR_UP) & (uint8_t)~NRF24_CONFIG_PRIM_RX);
    if (ret == NRF24_OK) hnrf->mode = NRF24_MODE_STANDBY;
    return ret;
}

NRF24_StatusTypeDef NRF24_FlushTx(NRF24_HandleTypeDef *hnrf)
{
    if (hnrf == NULL) return NRF24_ERROR_INVALID_PARAM;
    return command(hnrf, NRF24_CMD_FLUSH_TX, NULL, NULL, 0U);
}

NRF24_StatusTypeDef NRF24_FlushRx(NRF24_HandleTypeDef *hnrf)
{
    if (hnrf == NULL) return NRF24_ERROR_INVALID_PARAM;
    return command(hnrf, NRF24_CMD_FLUSH_RX, NULL, NULL, 0U);
}

bool NRF24_Available(NRF24_HandleTypeDef *hnrf, uint8_t *pipe)
{
    uint8_t status = NRF24_GetStatus(hnrf);
    uint8_t pipe_num = (status >> 1U) & 0x07U;
    if ((status & NRF24_STATUS_RX_DR) != 0U && pipe_num <= 5U) {
        if (pipe != NULL) *pipe = pipe_num;
        hnrf->last_pipe = pipe_num;
        return true;
    }
    if ((NRF24_ReadFifoStatus(hnrf) & NRF24_FIFO_RX_EMPTY) == 0U) {
        if (pipe != NULL) *pipe = (pipe_num <= 5U) ? pipe_num : 0U;
        hnrf->last_pipe = (pipe_num <= 5U) ? pipe_num : 0U;
        return true;
    }
    return false;
}

NRF24_StatusTypeDef NRF24_Read(NRF24_HandleTypeDef *hnrf, void *buffer, uint8_t len)
{
    NRF24_StatusTypeDef ret;
    if (hnrf == NULL || buffer == NULL || len == 0U || len > NRF24_MAX_PAYLOAD_SIZE) return NRF24_ERROR_INVALID_PARAM;
    if ((NRF24_ReadFifoStatus(hnrf) & NRF24_FIFO_RX_EMPTY) != 0U) return NRF24_ERROR_NO_DATA;
    ret = command(hnrf, NRF24_CMD_R_RX_PAYLOAD, NULL, (uint8_t *)buffer, len);
    (void)NRF24_ClearIrqFlags(hnrf, NRF24_STATUS_RX_DR);
    return ret;
}

static NRF24_StatusTypeDef write_payload(NRF24_HandleTypeDef *hnrf, uint8_t cmd, const void *buffer, uint8_t len, uint32_t timeout_ms)
{
    uint32_t start;
    uint8_t status;
    NRF24_StatusTypeDef ret;
    if (hnrf == NULL || buffer == NULL || len == 0U || len > NRF24_MAX_PAYLOAD_SIZE) return NRF24_ERROR_INVALID_PARAM;
    ret = NRF24_StopListening(hnrf);
    if (ret != NRF24_OK) return ret;
    (void)NRF24_ClearIrqFlags(hnrf, NRF24_STATUS_IRQ_MASK);
    (void)NRF24_FlushTx(hnrf);
    ret = command(hnrf, cmd, (const uint8_t *)buffer, NULL, len);
    if (ret != NRF24_OK) return ret;
    ce_high(hnrf);
    NRF24_DelayUs(NRF24_TIMING_CE_PULSE_US);
    ce_low(hnrf);

    start = HAL_GetTick();
    do {
        status = NRF24_GetStatus(hnrf);
        if ((status & NRF24_STATUS_TX_DS) != 0U) {
            (void)NRF24_ClearIrqFlags(hnrf, NRF24_STATUS_TX_DS);
            return NRF24_OK;
        }
        if ((status & NRF24_STATUS_MAX_RT) != 0U) {
            (void)NRF24_ClearIrqFlags(hnrf, NRF24_STATUS_MAX_RT);
            (void)NRF24_FlushTx(hnrf);
            return NRF24_ERROR_MAX_RT;
        }
    } while ((HAL_GetTick() - start) < timeout_ms);
    return NRF24_ERROR_TIMEOUT;
}

NRF24_StatusTypeDef NRF24_Write(NRF24_HandleTypeDef *hnrf, const void *buffer, uint8_t len, uint32_t timeout_ms)
{
    return write_payload(hnrf, NRF24_CMD_W_TX_PAYLOAD, buffer, len, timeout_ms);
}

NRF24_StatusTypeDef NRF24_WriteNoAck(NRF24_HandleTypeDef *hnrf, const void *buffer, uint8_t len, uint32_t timeout_ms)
{
    return write_payload(hnrf, NRF24_CMD_W_TX_PAYLOAD_NOACK, buffer, len, timeout_ms);
}

NRF24_StatusTypeDef NRF24_GetDynamicPayloadSize(NRF24_HandleTypeDef *hnrf, uint8_t *size)
{
    NRF24_StatusTypeDef ret;
    if (hnrf == NULL || size == NULL) return NRF24_ERROR_INVALID_PARAM;
    ret = command(hnrf, NRF24_CMD_R_RX_PL_WID, NULL, size, 1U);
    if (ret != NRF24_OK) return ret;
    if (*size > NRF24_MAX_PAYLOAD_SIZE) {
        (void)NRF24_FlushRx(hnrf);
        return NRF24_ERROR;
    }
    return NRF24_OK;
}

NRF24_StatusTypeDef NRF24_WriteAckPayload(NRF24_HandleTypeDef *hnrf, uint8_t pipe, const void *buffer, uint8_t len)
{
    if (hnrf == NULL || buffer == NULL || pipe > 5U || len == 0U || len > NRF24_MAX_PAYLOAD_SIZE) return NRF24_ERROR_INVALID_PARAM;
    return command(hnrf, (uint8_t)(NRF24_CMD_W_ACK_PAYLOAD | (pipe & 0x07U)), (const uint8_t *)buffer, NULL, len);
}

uint8_t NRF24_ReadFifoStatus(NRF24_HandleTypeDef *hnrf)
{
    uint8_t value = 0U;
    (void)NRF24_ReadRegister(hnrf, NRF24_REG_FIFO_STATUS, &value);
    return value;
}

uint8_t NRF24_ReadObserveTx(NRF24_HandleTypeDef *hnrf)
{
    uint8_t value = 0U;
    (void)NRF24_ReadRegister(hnrf, NRF24_REG_OBSERVE_TX, &value);
    return value;
}

uint8_t NRF24_ReadRpd(NRF24_HandleTypeDef *hnrf)
{
    uint8_t value = 0U;
    (void)NRF24_ReadRegister(hnrf, NRF24_REG_RPD, &value);
    return value & 0x01U;
}

const char *NRF24_StatusToString(NRF24_StatusTypeDef status)
{
    switch (status) {
    case NRF24_OK: return "OK";
    case NRF24_ERROR: return "Error";
    case NRF24_ERROR_HAL: return "HAL error";
    case NRF24_ERROR_INVALID_PARAM: return "Invalid parameter";
    case NRF24_ERROR_TIMEOUT: return "Timeout";
    case NRF24_ERROR_MAX_RT: return "Maximum retransmits";
    case NRF24_ERROR_NO_DATA: return "No data";
    case NRF24_ERROR_NOT_PRESENT: return "nRF24L01+ not present";
    default: return "Unknown";
    }
}

int nrf_init(void)
{
    static NRF24_HandleTypeDef default_radio;
    return (NRF24_Init(&default_radio, NULL) == NRF24_OK) ? 0 : -1;
}
