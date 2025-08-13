#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "LoRa.h"

static const char* TAG = "APP";

// ---------- ESP32‑C6 Pin map (EDIT IF NEEDED) ----------
#define PIN_SCK    12
#define PIN_MISO   13
#define PIN_MOSI   11
#define PIN_CS     10   // LoRa NSS
#define PIN_RST     9   // LoRa RESET
#define PIN_DIO0    8   // LoRa DIO0 (RxDone)

// ---------- LoRa config ----------
#define LORA_FREQ_HZ   915000000UL   // US915
#define LORA_BW_KHZ    125           // 7.8,10.4,15.6,20.8,31.25,41.7,62.5,125,250,500
#define LORA_SF        7             // 7..12
#define LORA_CR        1             // 1..4 => 4/5..4/8
#define LORA_CRC_ON    1

// ---------- Temporary storage: ring buffer in RAM ----------
typedef struct {
    uint64_t ts_us;
    int16_t  rssi_dbm;
    float    snr_db;
    uint8_t  len;
    uint8_t  data[64];   // adjust to your payload size
} rx_pkt_t;

#define RING_CAP 128
static rx_pkt_t ring[RING_CAP];
static size_t head = 0;
static size_t count_pkts = 0;
static SemaphoreHandle_t ring_mux;

// ---------- Radio + ISR ----------
static sx1276_t radio;
static spi_device_handle_t spi = NULL;
static TaskHandle_t rx_task_handle = NULL;

static void ring_push(const rx_pkt_t* p) {
    xSemaphoreTake(ring_mux, portMAX_DELAY);
    ring[head] = *p;
    head = (head + 1) % RING_CAP;
    if (count_pkts < RING_CAP) count_pkts++; // else overwrite oldest
    xSemaphoreGive(ring_mux);
}

static void ring_dump(void) {
    xSemaphoreTake(ring_mux, portMAX_DELAY);
    size_t local_head = head, local_cnt = count_pkts;
    xSemaphoreGive(ring_mux);

    if (!local_cnt) { ESP_LOGI(TAG, "[BUF] Empty"); return; }

    size_t tail = (local_head + RING_CAP - local_cnt) % RING_CAP;
    for (size_t i = 0; i < local_cnt; ++i) {
        size_t idx = (tail + i) % RING_CAP;
        rx_pkt_t* p = &ring[idx];
        printf("#%u t=%llu us RSSI=%d SNR=%.2f len=%u data=",
               (unsigned)i, (unsigned long long)p->ts_us, (int)p->rssi_dbm, p->snr_db, p->len);
        for (uint8_t j = 0; j < p->len; ++j) {
            printf("%s%02X", j ? " " : "", p->data[j]);
        }
        printf("\n");
    }
}

static void ring_clear(void) {
    xSemaphoreTake(ring_mux, portMAX_DELAY);
    head = 0; count_pkts = 0;
    xSemaphoreGive(ring_mux);
    ESP_LOGI(TAG, "[BUF] Cleared");
}

static void IRAM_ATTR dio0_isr(void* arg) {
    BaseType_t hp = pdFALSE;
    vTaskNotifyGiveFromISR(rx_task_handle, &hp);
    if (hp) portYIELD_FROM_ISR();
}

static void rx_task(void* arg) {
    // Configure DIO0 interrupt (rising edge)
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << PIN_DIO0),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };
    gpio_config(&io);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_DIO0, dio0_isr, NULL);

    // SPI bus (ESP32‑C6 uses SPI2_HOST for general‑purpose SPI)
    spi_bus_config_t bus = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t dev = {
        .clock_speed_hz = 8 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = -1,   // manual CS via GPIO
        .queue_size = 4
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev, &spi));

    // Prepare driver context
    radio.spi      = spi;
    radio.pin_cs   = PIN_CS;
    radio.pin_rst  = PIN_RST;
    radio.pin_dio0 = PIN_DIO0;

    ESP_ERROR_CHECK(sx1276_init(&radio));
    ESP_ERROR_CHECK(sx1276_set_frequency(&radio, (float)LORA_FREQ_HZ));
    ESP_ERROR_CHECK(sx1276_config_lora(&radio, (uint8_t)bw_to_reg(LORA_BW_KHZ),
                                       LORA_SF, LORA_CR, LORA_CRC_ON));
    ESP_ERROR_CHECK(sx1276_start_rx_continuous(&radio));
    ESP_LOGI(TAG, "Listening @ %.1f MHz, BW %u kHz, SF%u, CR4/%u",
             LORA_FREQ_HZ/1e6, LORA_BW_KHZ, LORA_SF, (unsigned)(LORA_CR+4));

    for (;;) {
        // Wait for ISR or timeout (also covers missed edges)
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(200));

        lora_packet_t lp = {0};
        uint8_t irq = 0;
        if (sx1276_read_packet(&radio, &lp, &irq) == ESP_OK) {
            rx_pkt_t p = {0};
            p.ts_us = esp_timer_get_time();
            p.rssi_dbm = lp.rssi_dbm;
            p.snr_db   = lp.snr_db;
            p.len      = (lp.len > sizeof(p.data)) ? sizeof(p.data) : lp.len;
            memcpy(p.data, lp.data, p.len);
            ring_push(&p);
        }
    }
}

static void console_task(void* arg) {
    for (;;) {
        int c = fgetc(stdin);
        if (c == EOF) { vTaskDelay(pdMS_TO_TICKS(50)); continue; }
        if (c == 'p' || c == 'P') ring_dump();
        else if (c == 'c' || c == 'C') ring_clear();
    }
}

void app_main(void) {
    ring_mux = xSemaphoreCreateMutex();
    xTaskCreate(rx_task,      "rx_task",      4096, NULL, 6, &rx_task_handle);
    xTaskCreate(console_task, "console_task", 4096, NULL, 4, NULL);
    ESP_LOGI(TAG, "Commands: 'p' = print buffer, 'c' = clear");
}