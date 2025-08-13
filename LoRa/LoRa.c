#include "LoRa.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ---- SPI helpers ----
static inline void cs_select(sx1276_t* d){ gpio_set_level(d->pin_cs, 0); }
static inline void cs_deselect(sx1276_t* d){ gpio_set_level(d->pin_cs, 1); }

static uint8_t read_reg(sx1276_t* d, uint8_t reg){
    uint8_t tx[2] = { (uint8_t)(reg & 0x7F), 0x00 };
    uint8_t rx[2] = {0};
    spi_transaction_t t = {.length=16, .tx_buffer=tx, .rx_buffer=rx};
    cs_select(d);
    esp_err_t err = spi_device_transmit(d->spi, &t);
    cs_deselect(d);
    if (err != ESP_OK) return 0xFF;
    return rx[1];
}

static void write_reg(sx1276_t* d, uint8_t reg, uint8_t val){
    uint8_t tx[2] = { (uint8_t)(reg | 0x80), val };
    spi_transaction_t t = {.length=16, .tx_buffer=tx};
    cs_select(d);
    spi_device_transmit(d->spi, &t);
    cs_deselect(d);
}

static void burst_read(sx1276_t* d, uint8_t reg, uint8_t* buf, size_t len){
    uint8_t hdr = (uint8_t)(reg & 0x7F);
    spi_transaction_t t1 = {.length=8, .tx_buffer=&hdr};
    spi_transaction_t t2 = {.length=(int)len*8, .rx_buffer=buf};
    cs_select(d);
    spi_device_transmit(d->spi, &t1);
    spi_device_transmit(d->spi, &t2);
    cs_deselect(d);
}

static void reset_chip(sx1276_t* d){
    gpio_set_level(d->pin_rst, 0);
    vTaskDelay(pdMS_TO_TICKS(2));
    gpio_set_level(d->pin_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

// ---- Public helpers ----
uint8_t bw_to_reg(uint32_t bw_khz){
    switch(bw_khz){
        case 8: return 0; case 10: return 1; case 16: return 2; case 20: return 3;
        case 31: return 4; case 42: return 5; case 62: return 6;
        case 125: return 7; case 250: return 8; case 500: return 9;
        default: return 7; // default 125 kHz
    }
}

// ---- Public API ----
esp_err_t sx1276_init(sx1276_t* d){
    // CS & RST outputs
    gpio_config_t io = {
        .pin_bit_mask = (1ULL<<d->pin_cs) | (1ULL<<d->pin_rst),
        .mode = GPIO_MODE_OUTPUT, .pull_up_en = 0, .pull_down_en = 0, .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
    gpio_set_level(d->pin_cs, 1);

    reset_chip(d);

    // Enter LoRa sleep then standby
    write_reg(d, REG_OP_MODE, LONG_RANGE_MODE | MODE_SLEEP);
    vTaskDelay(pdMS_TO_TICKS(10));
    write_reg(d, REG_OP_MODE, LONG_RANGE_MODE | MODE_STDBY);

    // FIFO base addresses
    write_reg(d, REG_FIFO_TX_BASE_ADDR, 0x80);
    write_reg(d, REG_FIFO_RX_BASE_ADDR, 0x00);

    // LNA boost
    write_reg(d, REG_LNA, 0x23);

    // Map DIO0 -> RxDone (00)
    uint8_t dio = read_reg(d, REG_DIO_MAPPING1);
    dio &= ~(0xC0);
    write_reg(d, REG_DIO_MAPPING1, dio);

    return ESP_OK;
}

esp_err_t sx1276_set_frequency(sx1276_t* d, float freq_hz){
    // frf = Freq / (32e6 / 2^19) = Freq / 61.03515625
    uint32_t frf = (uint32_t)(freq_hz / 61.03515625f);
    write_reg(d, REG_FRF_MSB, (frf >> 16) & 0xFF);
    write_reg(d, REG_FRF_MID, (frf >> 8)  & 0xFF);
    write_reg(d, REG_FRF_LSB,  frf        & 0xFF);
    d->freq_hz = freq_hz;
    return ESP_OK;
}

esp_err_t sx1276_config_lora(sx1276_t* d, uint8_t bw_reg, uint8_t sf, uint8_t cr, uint8_t crc_on){
    // MODEM_CONFIG1: [7:4]=BW, [3:1]=CR, [0]=ImplicitHeader(0)
    uint8_t mc1 = (bw_reg << 4) | ((cr & 0x07) << 1);
    // MODEM_CONFIG2: [7:4]=SF, [2]=RxCrcOn
    uint8_t mc2 = ((sf & 0x0F) << 4) | (crc_on ? (1<<2) : 0);
    write_reg(d, REG_MODEM_CONFIG1, mc1);
    write_reg(d, REG_MODEM_CONFIG2, mc2);
    // MODEM_CONFIG3: [2]=AGC, [3]=LowDataRateOpt auto
    write_reg(d, REG_MODEM_CONFIG3, 0x0C);

    d->bw = bw_reg; d->sf = sf; d->cr = cr; d->crc_on = crc_on;
    return ESP_OK;
}

esp_err_t sx1276_start_rx_continuous(sx1276_t* d){
    // Clear IRQs, set FIFO ptr to RX base, continuous RX
    write_reg(d, REG_IRQ_FLAGS, IRQ_ALL);
    uint8_t base = read_reg(d, REG_FIFO_RX_BASE_ADDR);
    write_reg(d, REG_FIFO_ADDR_PTR, base);
    write_reg(d, REG_OP_MODE, LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
    return ESP_OK;
}

esp_err_t sx1276_read_packet(sx1276_t* d, lora_packet_t* out, uint8_t* irq_flags){
    uint8_t flags = read_reg(d, REG_IRQ_FLAGS);
    if (irq_flags) *irq_flags = flags;

    if (flags & IRQ_RXDONE){
        if (flags & IRQ_PAYLOAD_CRC_ERROR){
            write_reg(d, REG_IRQ_FLAGS, (IRQ_PAYLOAD_CRC_ERROR | IRQ_RXDONE));
            return ESP_FAIL;
        }
        uint8_t len = read_reg(d, REG_RX_NB_BYTES);
        uint8_t cur = read_reg(d, REG_FIFO_RX_CURRENT_ADDR);
        write_reg(d, REG_FIFO_ADDR_PTR, cur);
        if ((size_t)len > sizeof(out->data)) len = (uint8_t)sizeof(out->data); // warning-safe
        burst_read(d, REG_FIFO, out->data, len);
        out->len = len;

        // Convert SNR/RSSI (HF port >=779 MHz)
        int8_t snr_raw = (int8_t)read_reg(d, REG_PKT_SNR_VALUE);
        out->snr_db = ((float)snr_raw)/4.0f;
        int16_t rssi_raw = read_reg(d, REG_PKT_RSSI_VALUE);
        float rssi_dbm = -157.0f + (float)rssi_raw;
        if (out->snr_db < 0) rssi_dbm += out->snr_db;
        out->rssi_dbm = (int16_t)rssi_dbm;

        write_reg(d, REG_IRQ_FLAGS, IRQ_RXDONE);
        return ESP_OK;
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t sx1276_clear_irq(sx1276_t* d, uint8_t flags){
    write_reg(d, REG_IRQ_FLAGS, flags);
    return ESP_OK;
}