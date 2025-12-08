// spi_slave_echo_working.c - Slave that ACTUALLY echoes every byte
#include <stdio.h>
#include <string.h>  // ADD THIS!
#include "driver/spi_slave.h"
#include "driver/gpio.h"

#define GPIO_MOSI 23
#define GPIO_MISO 19  
#define GPIO_SCLK 18
#define GPIO_CS   5

void app_main() {
    printf("=== SPI Slave ECHO ===\n");
    printf("Will echo EVERY byte received\n\n");
    
    // Setup SPI Slave
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI,
        .miso_io_num = GPIO_MISO,
        .sclk_io_num = GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };
    
    spi_slave_interface_config_t slvcfg = {
        .mode = 0,              // SPI mode 0
        .spics_io_num = GPIO_CS,
        .queue_size = 1,        // Single transaction queue
        .flags = 0
    };
    
    esp_err_t ret = spi_slave_initialize(HSPI_HOST, &buscfg, &slvcfg, SPI_DMA_DISABLED);
    if (ret != ESP_OK) {
        printf("SPI Slave init failed: %d\n", ret);
        return;
    }
    
    printf("Slave ready on pins:\n");
    printf("  MOSI(RX): GPIO%d\n", GPIO_MOSI);
    printf("  MISO(TX): GPIO%d\n", GPIO_MISO);
    printf("  SCLK:     GPIO%d\n", GPIO_SCLK);
    printf("  CS:       GPIO%d\n", GPIO_CS);
    printf("\nWaiting for master...\n\n");
    
    // Transaction variables
    spi_slave_transaction_t trans;
    uint8_t rx_byte;     // Byte received from master
    uint8_t tx_byte = 0x00;  // Byte to send to master (starts with 0x00)
    
    int transaction_count = 0;
    
    while(1) {
        transaction_count++;
        
        // Clear transaction structure
        memset(&trans, 0, sizeof(trans));
        
        // Setup 1-byte transaction
        trans.length = 8;            // 8 bits = 1 byte
        trans.rx_buffer = &rx_byte;  // Where to store received byte
        trans.tx_buffer = &tx_byte;  // What to send (echo of PREVIOUS byte)
        
        // DEBUG: Show what we're about to send
        // printf("Preparing to send: 0x%02X\n", tx_byte);
        
        // Wait for master to initiate transaction
        ret = spi_slave_transmit(HSPI_HOST, &trans, portMAX_DELAY);
        if (ret != ESP_OK) {
            printf("SPI error: %d\n", ret);
            continue;
        }
        
        // Print transaction details
        printf("Transaction #%d:\n", transaction_count);
        printf("  MOSI (Master -> Slave): 0x%02X", rx_byte);
        if (rx_byte >= 32 && rx_byte < 127) {
            printf(" ('%c')", rx_byte);
        }
        printf("\n");
        
        printf("  MISO (Slave -> Master): 0x%02X", tx_byte);
        if (tx_byte >= 32 && tx_byte < 127) {
            printf(" ('%c')", tx_byte);
        }
        printf("\n");
        
        // Interpret what master sent
        if (rx_byte == 0x01) {
            printf("  ^--- RECEIVE command\n");
        } else if (rx_byte == 0x00) {
            printf("  ^--- END command\n");
        } else if (rx_byte == 0xEE) {
            printf("  ^--- ERROR command\n");
        } else if (rx_byte == 0xFF) {
            printf("  ^--- VOID command\n");
        } else if (rx_byte >= 32 && rx_byte < 127) {
            printf("  ^--- Data character\n");
        }
        
        printf("\n");
        
        // CRITICAL: Prepare for NEXT transaction
        // Next time, echo back what we JUST received
        tx_byte = rx_byte;
    }
}
