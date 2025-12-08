// esp32_spi_slave_128byte.c
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "driver/spi_slave.h"
#include "driver/gpio.h"
#include "esp_timer.h"

// VSPI Pins (different from HSPI!)
#define MOSI_PIN 23  // VSPI MOSI
#define MISO_PIN 19  // VSPI MISO  
#define SCLK_PIN 18  // VSPI SCLK
#define CS_PIN   5   // VSPI CS

// 128 BYTES buffer (matching master)
#define BUFFER_SIZE 128

void delay_ms(uint32_t ms) {
    esp_rom_delay_us(ms * 1000);
}

void setup_spi_slave() {
    printf("Setting up SPI Slave (VSPI) for 128-byte reception...\n");
    
    // SPI bus configuration for VSPI
    spi_bus_config_t buscfg = {
        .mosi_io_num = MOSI_PIN,
        .miso_io_num = MISO_PIN,
        .sclk_io_num = SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = BUFFER_SIZE  // 128 bytes
    };
    
    // SPI slave configuration
    spi_slave_interface_config_t slvcfg = {
        .mode = 0,                  // MUST match master's mode!
        .spics_io_num = CS_PIN,     // CS pin
        .queue_size = 2,            // Queue depth
        .flags = 0,                 // No special flags
        .post_setup_cb = NULL,      // No pre-callback
        .post_trans_cb = NULL       // No post-callback
    };
    
    // Initialize SPI slave WITH DMA for large transfers
    esp_err_t ret = spi_slave_initialize(VSPI_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        printf("SPI Slave init failed: %s\n", esp_err_to_name(ret));
        return;
    }
    
    printf("SPI Slave (VSPI) initialized for 128-byte reception:\n");
    printf("  MOSI (RX): GPIO%d <- Master MOSI\n", MOSI_PIN);
    printf("  MISO (TX): GPIO%d -> Master MISO\n", MISO_PIN);
    printf("  SCLK:      GPIO%d <- Master SCLK\n", SCLK_PIN);
    printf("  CS:        GPIO%d <- Master CS\n", CS_PIN);
    printf("  Mode:      0 (CPOL=0, CPHA=0)\n");
    printf("  Buffer:    %d bytes (1024 bits)\n\n", BUFFER_SIZE);
}

void print_buffer_preview(uint8_t *buffer, uint32_t packet_num, int bytes_received) {
    printf("[Packet #%lu - %d bytes]\n", packet_num, bytes_received);
    
    // Show first 16 bytes
    printf("First 16 bytes: ");
    for (int i = 0; i < 16 && i < bytes_received; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
    
    // Show last 16 bytes
    printf("Last 16 bytes:  ");
    int start = (bytes_received > 16) ? bytes_received - 16 : 0;
    for (int i = start; i < bytes_received; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
    
    // Show as ASCII if printable
    printf("As string (first 32): ");
    for (int i = 0; i < 32 && i < bytes_received; i++) {
        if (buffer[i] >= 32 && buffer[i] < 127) {
            printf("%c", buffer[i]);
        } else {
            printf(".");
        }
    }
    printf("\n");
}

void analyze_packet(uint8_t *buffer, int bytes_received) {
    // Check for our known pattern
    if (bytes_received >= 6) {
        if (memcmp(buffer, "START-", 6) == 0) {
            printf("✓ Detected START pattern\n");
        }
    }
    
    if (bytes_received >= 7) {
        if (memcmp(&buffer[bytes_received-6], "-END!", 6) == 0) {
            printf("✓ Detected END pattern\n");
        }
    }
    
    // Extract timestamp if present (bytes 16-19)
    if (bytes_received >= 20) {
        uint32_t timestamp = (buffer[16] << 24) | (buffer[17] << 16) | 
                            (buffer[18] << 8) | buffer[19];
        printf("✓ Timestamp: %lu (", timestamp);
        
        // Convert to human readable
        time_t ts = timestamp;
        char time_str[32];
        ctime_r(&ts, time_str);
        time_str[strlen(time_str)-1] = '\0';  // Remove newline
        printf("%s)\n", time_str);
    }
    
    // Calculate checksum
    uint8_t checksum = 0;
    for (int i = 0; i < bytes_received; i++) {
        checksum ^= buffer[i];
    }
    printf("✓ XOR Checksum: 0x%02X\n", checksum);
}

void app_main() {
    printf("\n================================\n");
    printf("ESP32 SPI Slave (VSPI) - 128-byte Receiver\n");
    printf("Receiving 128-byte buffers from master\n");
    printf("================================\n\n");
    
    setup_spi_slave();
    
    uint8_t rx_buffer[BUFFER_SIZE];
    uint8_t tx_buffer[BUFFER_SIZE];  // Optional: for sending response
    
    // Initialize TX buffer (optional echo/response)
    memset(tx_buffer, 0x00, sizeof(tx_buffer));
    memcpy(tx_buffer, "SLAVE-ACK-----", 14);  // Response pattern
    
    uint32_t packet_counter = 0;
    uint64_t last_packet_time = 0;
    
    printf("Waiting for 128-byte packets from master...\n\n");
    
    while (1) {
        // Clear receive buffer
        memset(rx_buffer, 0, sizeof(rx_buffer));
        
        // Setup transaction for receiving 128 bytes
        spi_slave_transaction_t trans = {
            .length = BUFFER_SIZE * 8,  // 1024 bits
            .rx_buffer = rx_buffer,
            .tx_buffer = tx_buffer  // Optional: send response while receiving
        };
        
        printf("Waiting for master to send 128-byte packet...\n");
        
        // Wait for master to initiate transfer (CS goes low)
        uint64_t start_time = esp_timer_get_time();
        esp_err_t ret = spi_slave_transmit(VSPI_HOST, &trans, portMAX_DELAY);
        uint64_t transfer_time = esp_timer_get_time() - start_time;
        
        if (ret != ESP_OK) {
            printf("SPI Slave error: %s\n", esp_err_to_name(ret));
            delay_ms(100);
            continue;
        }
        
        packet_counter++;
        
        // Calculate actual bytes received
        int bits_received = trans.trans_len;
        int bytes_received = bits_received / 8;
        
        printf("\n=== PACKET RECEIVED ===\n");
        printf("Transfer time: %lluµs\n", transfer_time);
        
        // Show what we received
        print_buffer_preview(rx_buffer, packet_counter, bytes_received);
        
        // Analyze the packet
        analyze_packet(rx_buffer, bytes_received);
        
        // Calculate time since last packet
        if (last_packet_time > 0) {
            uint64_t interval = esp_timer_get_time() - last_packet_time;
            printf("Time since last packet: %llums\n", interval / 1000);
        }
        last_packet_time = esp_timer_get_time();
        
        // Update TX buffer for next transaction (optional)
        // Could echo back checksum or status
        if (bytes_received > 0) {
            // Simple echo: next time send first byte of received data
            tx_buffer[0] = rx_buffer[0];
        }
        
        printf("=== END OF PACKET ===\n\n");
        
        // Small delay before next reception
        delay_ms(10);
    }
}
