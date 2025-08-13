#pragma once
#include <stdint.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"

// ---- Registers (subset) ----
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_LNA                  0x0C
#define REG_FIFO_ADDR_PTR        0x0D
#define REG_FIFO_TX_BASE_ADDR    0x0E
#define REG_FIFO_RX_BASE_ADDR    0x0F
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13
#define REG_PKT_SNR_VALUE        0x19
#define REG_PKT_RSSI_VALUE       0x1A
#define REG_MODEM_CONFIG1        0x1D
#define REG_MODEM_CONFIG2        0x1E
#define REG_MODEM_CONFIG3        0x26
#define REG_DIO_MAPPING1         0x40

// ---- Bits/modes ----
#define LONG_RANGE_MODE          0x80
#define MODE_SLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_RX_CONTINUOUS       0x05

#define IRQ_RXDONE               0x40
#define IRQ_PAYLOAD_CRC_ERROR    0x20
#define IRQ_ALL                  0xFF

typedef struct {
    spi_device_handle_t spi;
    gpio_num_t pin_cs;
    gpio_num_t pin_rst;
    gpio_num_t pin_dio0;
    float freq_hz;
    uint8_t bw;
    uint8_t sf;
    uint8_t cr;
    uint8_t crc_on;
} sx1276_t;

typedef struct {
    uint8_t len;
    uint8_t data[255]; // max LoRa payload
    int16_t rssi_dbm;
    float   snr_db;
} lora_packet_t;

// Public API
esp_err_t sx1276_init(sx1276_t* d);
esp_err_t sx1276_set_frequency(sx1276_t* d, float freq_hz);
esp_err_t sx1276_config_lora(sx1276_t* d, uint8_t bw_reg, uint8_t sf, uint8_t cr, uint8_t crc_on);
esp_err_t sx1276_start_rx_continuous(sx1276_t* d);
esp_err_t sx1276_read_packet(sx1276_t* d, lora_packet_t* out, uint8_t* irq_flags);
esp_err_t sx1276_clear_irq(sx1276_t* d, uint8_t flags);

// Helper
uint8_t bw_to_reg(uint32_t bw_khz);