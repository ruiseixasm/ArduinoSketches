// spi_slave_echo_fixed.c - SPI Slave that IMMEDIATELY echoes back each byte
#include <stdio.h>
#include <string.h>
#include "driver/spi_slave.h"
#include "driver/gpio.h"
#include "esp_timer.h"

// Pins for Slave
#define GPIO_MOSI 23  // Slave RX
#define GPIO_MISO 19  // Slave TX  
#define GPIO_SCLK 18  // Slave CLK
#define GPIO_CS   5   // Slave CS

#define RECEIVE 0x01
#define END     0x00
#define VOID    0xFF
#define ERROR   0xEE

// SPI Slave that echoes immediately (CRITICAL FOR YOUR MASTER!)
void run_spi_slave_immediate_echo() {
    spi_slave_transaction_t trans;
    uint8_t rx_byte = 0;
    
    // The trick: tx_buffer MUST point to a variable that contains the echo
    // We'll update it AFTER receiving, but BEFORE next transaction
    static uint8_t tx_byte = 0;  // Will contain last received byte for echo
    
    printf("\n=== SPI Slave Echo Mode ===\n");
    printf("Echoing EVERY byte immediately back to master\n");
    printf("Master should receive exactly what it sent!\n\n");
    
    while (1) {
        // Setup transaction for 1 byte
        memset(&trans, 0, sizeof(trans));
        trans.length = 8;           // 1 byte = 8 bits
        trans.rx_buffer = &rx_byte; // Where to store received byte
        trans.tx_buffer = &tx_byte; // What to send back (previous echo)
        
        // DEBUG: Show what we're about to send
        // printf("About to send: 0x%02X, waiting for master...\n", tx_byte);
        
        // Wait for master to initiate transaction
        esp_err_t ret = spi_slave_transmit(HSPI_HOST, &trans, portMAX_DELAY);
        
        if (ret != ESP_OK) {
            printf("SPI error: %d\n", ret);
            esp_rom_delay_us(1000000); // 1s delay on error
            continue;
        }
        
        // DEBUG: Show what we received
        if (rx_byte != 0) {  // Don't print zeros (idle)
            printf("Received: 0x%02X", rx_byte);
            if (rx_byte >= 32 && rx_byte < 127) {
                printf(" ('%c')", rx_byte);
            }
            
            // Protocol interpretation
            switch (rx_byte) {
                case RECEIVE: printf(" [RECEIVE cmd]"); break;
                case END:     printf(" [END cmd]"); break;
                case ERROR:   printf(" [ERROR cmd]"); break;
                case VOID:    printf(" [VOID cmd]"); break;
                default:      printf(" [data]"); break;
            }
            printf("\n");
        }
        
        // CRITICAL: Prepare echo for NEXT transaction
        // For your protocol, we echo back exactly what we received
        tx_byte = rx_byte;
        
        // Special case: If master sends END, we should also send END back
        if (rx_byte == END) {
            // Master expects END echoed back
            // tx_byte is already END (from above assignment)
            printf("  -> Echoing END back to master\n");
        }
        
        // Reset for next (optional)
        rx_byte = 0;
        
        // Small delay if needed (but be careful with timing!)
        // esp_rom_delay_us(10);
    }
}

// Alternative: Echo slave with protocol awareness
void run_spi_slave_smart_echo() {
    spi_slave_transaction_t trans;
    uint8_t rx_buffer[64];
    uint8_t tx_buffer[64];
    int byte_count = 0;
    bool receiving = false;
    
    printf("\n=== SPI Slave Smart Echo ===\n");
    printf("Protocol-aware echo for your master\n\n");
    
    while (1) {
        memset(&trans, 0, sizeof(trans));
        
        // We'll handle up to 64 bytes at once
        trans.length = sizeof(rx_buffer) * 8;
        trans.rx_buffer = rx_buffer;
        trans.tx_buffer = tx_buffer;
        
        // Prepare echo buffer (echo back what we receive)
        memset(tx_buffer, 0, sizeof(tx_buffer));
        
        // Wait for master
        esp_err_t ret = spi_slave_transmit(HSPI_HOST, &trans, portMAX_DELAY);
        
        if (ret != ESP_OK) {
            printf("SPI error: %d\n", ret);
            continue;
        }
        
        // Calculate bytes received
        int bytes = trans.trans_len / 8;
        if (bytes > sizeof(rx_buffer)) bytes = sizeof(rx_buffer);
        
        if (bytes > 0) {
            printf("Received %d bytes: ", bytes);
            
            // Process and prepare echo
            for (int i = 0; i < bytes; i++) {
                printf("%02X ", rx_buffer[i]);
                
                // Echo back exactly what we received
                tx_buffer[i] = rx_buffer[i];
                
                // Protocol logic
                if (rx_buffer[i] == RECEIVE) {
                    printf("\n  -> RECEIVE command, ready for data\n");
                    receiving = true;
                    byte_count = 0;
                } else if (rx_buffer[i] == END) {
                    printf("\n  -> END command, transmission complete\n");
                    receiving = false;
                } else if (rx_buffer[i] == ERROR) {
                    printf("\n  -> ERROR command\n");
                    receiving = false;
                } else if (receiving) {
                    // Data byte during reception
                    byte_count++;
                }
            }
            printf("\n");
            
            if (receiving && byte_count > 0) {
                printf("  Receiving data, byte %d\n", byte_count);
            }
        }
    }
}

// Setup function
void setup_spi_slave() {
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI,
        .miso_io_num = GPIO_MISO,
        .sclk_io_num = GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    spi_slave_interface_config_t slvcfg = {
        .mode = 0,
        .spics_io_num = GPIO_CS,
        .queue_size = 3,
        .flags = 0
    };

    printf("Initializing SPI Slave Echo...\n");
    
    esp_err_t ret = spi_slave_initialize(HSPI_HOST, &buscfg, &slvcfg, SPI_DMA_DISABLED);
    
    if (ret != ESP_OK) {
        printf("SPI Slave init failed: %d\n", ret);
        return;
    }
    
    printf("SPI Slave Echo Ready!\n");
    printf("Pins: MOSI(RX)=GPIO%d, MISO(TX)=GPIO%d, SCLK=GPIO%d, CS=GPIO%d\n",
           GPIO_MOSI, GPIO_MISO, GPIO_SCLK, GPIO_CS);
}

void app_main(void) {
    printf("\n================================\n");
    printf("ESP32 SPI Slave - Immediate Echo\n");
    printf("================================\n\n");
    
    setup_spi_slave();
    
    // Run the immediate echo slave (RECOMMENDED for your master)
    run_spi_slave_immediate_echo();
    
    // Or the smart echo version
    // run_spi_slave_smart_echo();
}
