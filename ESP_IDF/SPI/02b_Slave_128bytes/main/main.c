// esp32_spi_master_128byte.c
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_timer.h"

// SPI Pins (HSPI)
#define MOSI_PIN 13
#define MISO_PIN 12
#define SCLK_PIN 14
#define CS_PIN   15

// 128 BYTES buffer (1024 bits)
#define BUFFER_SIZE 128

spi_device_handle_t spi;

void delay_ms(uint32_t ms) {
    esp_rom_delay_us(ms * 1000);
}

void setup_spi_master() {
    printf("Setting up SPI Master for 128-byte transfers...\n");
    
    // SPI bus configuration
    spi_bus_config_t buscfg = {
        .mosi_io_num = MOSI_PIN,
        .miso_io_num = MISO_PIN,
        .sclk_io_num = SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = BUFFER_SIZE  // 128 bytes
    };
    
    // SPI device configuration - need DMA for large buffer
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 8000000,  // 8 MHz for faster transfer
        .mode = 0,                  // SPI Mode 0
        .spics_io_num = CS_PIN,     // CS pin
        .queue_size = 1,
        .flags = 0
    };
    
    // Initialize SPI bus WITH DMA for large transfers
    esp_err_t ret = spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        printf("SPI bus init failed: %s\n", esp_err_to_name(ret));
        return;
    }
    
    // Attach device
    ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
    if (ret != ESP_OK) {
        printf("SPI device add failed: %s\n", esp_err_to_name(ret));
        return;
    }
    
    printf("SPI Master initialized for 128-byte transfers:\n");
    printf("  MOSI: GPIO%d\n", MOSI_PIN);
    printf("  MISO: GPIO%d\n", MISO_PIN);
    printf("  SCLK: GPIO%d\n", SCLK_PIN);
    printf("  CS:   GPIO%d\n", CS_PIN);
    printf("  Clock: 8 MHz\n");
    printf("  Buffer: %d bytes (1024 bits)\n", BUFFER_SIZE);
    printf("  Transfer time: ~128µs at 8MHz\n\n");
}

void fill_buffer(uint8_t *buffer, uint32_t counter) {
    // Fill with pattern: counter + incremental values
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = (counter + i) & 0xFF;
    }
    
    // Add some recognizable pattern
    memcpy(&buffer[0], "START-", 6);
    memcpy(&buffer[BUFFER_SIZE-7], "-END!", 6);
    buffer[BUFFER_SIZE-1] = '\0';
    
    // Add timestamp at position 16-19
    uint32_t timestamp = (uint32_t)time(NULL);
    buffer[16] = (timestamp >> 24) & 0xFF;
    buffer[17] = (timestamp >> 16) & 0xFF;
    buffer[18] = (timestamp >> 8) & 0xFF;
    buffer[19] = timestamp & 0xFF;
}

void send_128byte_buffer(uint8_t *buffer) {
    spi_transaction_t trans = {
        .length = BUFFER_SIZE * 8,  // 128 bytes * 8 = 1024 bits
        .tx_buffer = buffer,
        .rx_buffer = NULL
    };
    
    uint64_t start = esp_timer_get_time();
    esp_err_t ret = spi_device_transmit(spi, &trans);
    uint64_t duration = esp_timer_get_time() - start;
    
    if (ret == ESP_OK) {
        printf("✓ 128-byte buffer sent in %lluµs\n", duration);
    } else {
        printf("✗ SPI transmit failed: %s\n", esp_err_to_name(ret));
    }
}

void print_buffer_preview(uint8_t *buffer, uint32_t counter) {
    printf("[Transmission #%lu]\n", counter);
    printf("First 16 bytes: ");
    for (int i = 0; i < 16; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
    
    printf("Last 16 bytes:  ");
    for (int i = BUFFER_SIZE-16; i < BUFFER_SIZE; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
    
    // Show as ASCII if printable
    printf("As string: ");
    for (int i = 0; i < 32 && i < BUFFER_SIZE; i++) {
        if (buffer[i] >= 32 && buffer[i] < 127) {
            printf("%c", buffer[i]);
        } else {
            printf(".");
        }
    }
    printf("\n");
}

void app_main() {
    printf("\n================================\n");
    printf("ESP32 SPI Master - 128-BYTE Transmitter\n");
    printf("Sending 128-byte (1024-bit) buffer every 2 seconds\n");
    printf("================================\n\n");
    
    setup_spi_master();
    
    uint8_t tx_buffer[BUFFER_SIZE];
    uint32_t counter = 0;
    
    printf("Starting 128-byte transmission loop...\n\n");
    
    while (1) {
        // Fill buffer with new data
        fill_buffer(tx_buffer, counter);
        
        // Show preview
        print_buffer_preview(tx_buffer, counter);
        
        // Send the 128-byte buffer
        send_128byte_buffer(tx_buffer);
        
        counter++;
        
        printf("\nWaiting 2 seconds...\n\n");
        
        // Wait exactly 2 seconds
        delay_ms(2000);
    }
}
