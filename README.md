# STM32 HAL nRF24L01+ Library

This folder contains a CubeIDE-friendly nRF24L01+ driver written for STM32 HAL.  It uses the project SPI handle, a timer for microsecond delays, and normal HAL GPIO calls for the CE/CSN control pins.

## Hardware connections

The default bindings are in `NRF24_lib_config.h`:

| nRF24L01+ pin | STM32 default | Notes |
| --- | --- | --- |
| VCC | 3.3 V only | Do not connect to 5 V. Add local decoupling close to the module. |
| GND | GND | Common ground is required. |
| CE | PA1 | Software controlled radio enable. |
| CSN | PA2 | Software controlled SPI chip select, active low. |
| SCK | PA5 / SPI1 SCK | SPI mode 0. |
| MOSI | PA7 / SPI1 MOSI | SPI mode 0. |
| MISO | PA6 / SPI1 MISO | SPI mode 0. |
| IRQ | Disabled by default (`NRF24_USE_IRQ = 0`) | Optional interrupt input; polling also works. |

> **Important:** The default project polls the radio and does not initialize IRQ. If you enable `NRF24_USE_IRQ`, choose a GPIO that does not conflict with SPI pins. CE and CSN must be normal GPIO outputs. SPI NSS should be configured as software NSS.

## CubeIDE setup

Make these settings in the project's `.ioc` file through STM32CubeIDE instead of adding manual GPIO or SPI setup code to `main.c`:

1. Enable an SPI peripheral as master, 8-bit, MSB first, clock polarity low, first edge phase. Specify the SPI handler in config file.
2. Keep the SPI baud rate at or below 10Mbps. Change prescaler to reach the desired baud rate.
3. Enable a timer in **one pulse mode** and set the prescaler to 0 and by changing counter period (auto reload register) create 1us pulse. For example if you are using tim2 and timer clock (generally APB1 timer clock) is to 8MHz you should set counter period to 7 to create a 1us pulse. (DO NOT set prescaller other than 0 as it will be changed during the program multiple times.)
4. Configure the CE pin named in `NRF24_CE_Pin` as **GPIO Output**, push-pull, no pull, high speed, and set its initial output level to **Low**.
5. Configure the CSN pin named in `NRF24_CSN_Pin` as **GPIO Output**, push-pull, no pull, high speed, and set its initial output level to **High**.
6. If you enable `NRF24_USE_IRQ`, configure the IRQ pin named in `NRF24_IRQ_Pin` as a **GPIO input** with pull-up, or as an EXTI input with **pull-up** if your application handles radio interrupts.
7. Add library to the compiler include paths.

After changing the `.ioc`, regenerate the CubeIDE project so the generated functions contain these settings.

## Minimal receiver example

```c
#include "../NRF24_Library/NRF24_lib.h"

static NRF24_HandleTypeDef radio;
static uint8_t address[5] = { 'N', 'O', 'D', 'E', '1' };
static uint8_t rx_payload[32];

void app_radio_init(void)
{
    NRF24_ConfigTypeDef cfg;
    NRF24_GetDefaultConfig(&cfg);
    cfg.channel = 76;
    cfg.payload_size = 32;

    if (NRF24_Init(&radio, &cfg) == NRF24_OK) {
        NRF24_OpenReadingPipe(&radio, 1, address, sizeof(address));
        NRF24_StartListening(&radio);
    }
}

void app_radio_poll(void)
{
    uint8_t pipe;
    if (NRF24_Available(&radio, &pipe)) {
        NRF24_Read(&radio, rx_payload, radio.config.payload_size);
    }
}
```

## Minimal transmitter example

```c
static NRF24_HandleTypeDef radio;
static uint8_t address[5] = { 'N', 'O', 'D', 'E', '1' };

void app_radio_init(void)
{
    NRF24_ConfigTypeDef cfg;
    NRF24_GetDefaultConfig(&cfg);
    cfg.channel = 76;
    cfg.payload_size = 32;

    if (NRF24_Init(&radio, &cfg) == NRF24_OK) {
        NRF24_OpenWritingPipe(&radio, address, sizeof(address));
        NRF24_StopListening(&radio);
    }
}

void app_radio_send(void)
{
    uint8_t payload[32] = "hello from STM32";
    NRF24_StatusTypeDef st = NRF24_Write(&radio, payload, sizeof(payload), 100);
    if (st != NRF24_OK) {
        /* Use NRF24_StatusToString(st), NRF24_ReadObserveTx(), and breakpoints to debug. */
    }
}
```

## Core API

- `NRF24_GetDefaultConfig()` fills safe defaults: channel 76, 32-byte payloads, 5-byte addresses, 1 Mbps, 0 dBm, 2-byte CRC, and auto acknowledgements.
- `NRF24_Init()` binds the HAL handles from `NRF24_lib_config.h`, verifies SPI communication, writes the radio setup registers, flushes FIFOs, and clears IRQ flags.
- `NRF24_OpenWritingPipe()` writes `TX_ADDR` and pipe 0 so auto acknowledgements can return correctly.
- `NRF24_OpenReadingPipe()` configures RX pipe addresses and enables the selected pipe.
- `NRF24_StartListening()` enters PRX receive mode.
- `NRF24_StopListening()` returns to standby/PTX mode.
- `NRF24_Write()` sends a payload and waits for `TX_DS`, `MAX_RT`, or timeout.
- `NRF24_Available()` and `NRF24_Read()` provide polling receive support.
- `NRF24_ReadRegister()` / `NRF24_WriteRegister()` expose the register map for debugging and advanced tuning.

## Debugging checklist

- Confirm the module is powered from 3.3 V and has a nearby capacitor. Many PA/LNA modules need more current than a weak regulator can provide.
- Confirm CSN idles high and CE idles low before initialization.
- Confirm SPI mode 0 and software NSS.
- Call `NRF24_IsPresent()` or check that `NRF24_Init()` returns `NRF24_OK`; `NRF24_ERROR_NOT_PRESENT` usually means wiring, SPI, or power trouble.
- Read `NRF24_ReadObserveTx()` after transmit errors. The high nibble shows lost packets and the low nibble shows retry count.
- Use the same channel, address width, address bytes, CRC, and payload mode on both radios.

## Notes on dynamic payloads and ACK payloads

Set `cfg.dynamic_payloads = true` before `NRF24_Init()` to enable dynamic payload width. Use `NRF24_GetDynamicPayloadSize()` before `NRF24_Read()` when dynamic payloads are enabled.

Set `cfg.ack_payloads = true` before `NRF24_Init()` to enable ACK payload support, then load pipe-specific ACK data with `NRF24_WriteAckPayload()` while in receive mode.
