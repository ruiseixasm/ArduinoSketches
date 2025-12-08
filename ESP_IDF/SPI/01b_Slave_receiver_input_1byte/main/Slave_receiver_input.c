// spi_slave_receiver.c - Simple SPI Slave that prints received data
#include <stdio.h>
#include <string.h>
#include "driver/spi_slave.h"
#include "driver/gpio.h"
#include "esp_timer.h"

// Pins for Slave (DIFFERENT from master if on same ESP32!)
// For testing on SAME ESP32, use these alternate pins:
#define GPIO_MOSI 23  // Slave RX (connected to Master MOSI)
#define GPIO_MISO 19  // Slave TX (connected to Master MISO)  
#define GPIO_SCLK 18  // Slave CLK
#define GPIO_CS   5   // Slave CS

// Or for separate ESP32, use same pins as master:
// #define GPIO_MOSI 13  // Slave RX
// #define GPIO_MISO 12  // Slave TX
// #define GPIO_SCLK 14  // CLK
// #define GPIO_CS   15  // CS

#define BUFFER_SIZE 128
#define RECEIVE 0x01
#define END     0x00
#define VOID    0xFF
#define ERROR   0xEE

// Global receive buffer
uint8_t rx_buffer[BUFFER_SIZE];
spi_slave_interface_config_t slv_config;

// Simple delay function
void delay_ms(uint32_t ms) {
    esp_rom_delay_us(ms * 1000);
}

// SPI Slave initialization
void setup_spi_slave() {
    // Configuration for the SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI,
        .miso_io_num = GPIO_MISO,
        .sclk_io_num = GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    // Configuration for the SPI slave interface
    slv_config = (spi_slave_interface_config_t){
        .mode = 0,                    // SPI mode 0
        .spics_io_num = GPIO_CS,      // CS pin
        .queue_size = 3,              // Transaction queue size
        .flags = 0,                   // No special flags
        .post_setup_cb = NULL,        // No callback before transmission
        .post_trans_cb = NULL         // No callback after transmission
    };

    printf("Initializing SPI Slave...\n");
    
    // Initialize SPI slave
    esp_err_t ret = spi_slave_initialize(
        HSPI_HOST,      // Use HSPI for slave
        &buscfg,        // Bus configuration
        &slv_config,    // Slave configuration
        SPI_DMA_DISABLED // No DMA for simplicity
    );
    
    if (ret != ESP_OK) {
        printf("SPI Slave init failed: %d\n", ret);
        return;
    }
    
    printf("SPI Slave ready on pins:\n");
    printf("  MOSI(RX): GPIO%d\n", GPIO_MOSI);
    printf("  MISO(TX): GPIO%d\n", GPIO_MISO);
    printf("  SCLK:     GPIO%d\n", GPIO_SCLK);
    printf("  CS:       GPIO%d\n", GPIO_CS);
    printf("Waiting for master transmission...\n\n");
}

// Simple slave that echoes back received bytes
void run_spi_slave_echo() {
    spi_slave_transaction_t trans;
    uint8_t tx_buffer[BUFFER_SIZE];
    uint8_t rx_buffer[BUFFER_SIZE];
    
    memset(&trans, 0, sizeof(trans));
    
    while (1) {
        // Clear buffers
        memset(tx_buffer, 0, sizeof(tx_buffer));
        memset(rx_buffer, 0, sizeof(rx_buffer));
        
        // Setup transaction
        trans.length = sizeof(rx_buffer) * 8;  // In bits
        trans.rx_buffer = rx_buffer;
        trans.tx_buffer = tx_buffer;
        trans.user = NULL;
        
        printf("Slave: Waiting for transaction...\n");
        
        // Wait for master to initiate transaction
        esp_err_t ret = spi_slave_transmit(
            HSPI_HOST,  // SPI host
            &trans,     // Transaction
            portMAX_DELAY
        );
        
        if (ret != ESP_OK) {
            printf("Slave transmit error: %d\n", ret);
            delay_ms(1000);
            continue;
        }
        
        // Process received data
        printf("\n--- Slave Received Data ---\n");
        printf("Trans length: %d bits (%d bytes)\n", 
               trans.trans_len, trans.trans_len / 8);
        
        // Print bytes in hex and ASCII
        int bytes_received = trans.trans_len / 8;
        if (bytes_received > BUFFER_SIZE) bytes_received = BUFFER_SIZE;
        
        printf("Hex:    ");
        for (int i = 0; i < bytes_received; i++) {
            printf("%02X ", rx_buffer[i]);
        }
        printf("\nASCII:  ");
        for (int i = 0; i < bytes_received; i++) {
            if (rx_buffer[i] >= 32 && rx_buffer[i] < 127) {
                printf("%c  ", rx_buffer[i]);
            } else {
                printf(".  ");
            }
        }
        printf("\n");
        
        // Interpret protocol
        printf("\nProtocol Analysis:\n");
        if (bytes_received > 0) {
            switch (rx_buffer[0]) {
                case RECEIVE:
                    printf("  Command: RECEIVE (0x01)\n");
                    if (bytes_received > 1) {
                        printf("  Data: ");
                        for (int i = 1; i < bytes_received; i++) {
                            if (rx_buffer[i] == END) {
                                printf("[END]");
                                break;
                            } else if (rx_buffer[i] == ERROR) {
                                printf("[ERROR]");
                                break;
                            } else if (rx_buffer[i] >= 32 && rx_buffer[i] < 127) {
                                printf("%c", rx_buffer[i]);
                            } else {
                                printf("[0x%02X]", rx_buffer[i]);
                            }
                        }
                        printf("\n");
                    }
                    break;
                case END:
                    printf("  Command: END (0x00)\n");
                    break;
                case ERROR:
                    printf("  Command: ERROR (0xEE)\n");
                    break;
                default:
                    printf("  Unknown command: 0x%02X\n", rx_buffer[0]);
            }
        }
        
        printf("--- End of Transmission ---\n\n");
        
        // Prepare echo response (optional)
        // For now, just send zeros back
        memset(tx_buffer, 0, sizeof(tx_buffer));
        
        delay_ms(100);  // Small delay between transactions
    }
}

// Advanced slave that implements your protocol
void run_spi_slave_protocol() {
    spi_slave_transaction_t trans;
    uint8_t tx_byte = 0;
    uint8_t rx_byte = 0;
    
    printf("Slave Protocol Mode - Echoing back received bytes\n");
    printf("Will respond with: same byte (echo), END for null, ERROR for problems\n\n");
    
    while (1) {
        // Single byte transaction
        memset(&trans, 0, sizeof(trans));
        trans.length = 8;  // 1 byte at a time
        trans.rx_buffer = &rx_byte;
        trans.tx_buffer = &tx_byte;
        
        // Wait for master
        esp_err_t ret = spi_slave_transmit(HSPI_HOST, &trans, portMAX_DELAY);
        
        if (ret != ESP_OK) {
            printf("Slave error: %d\n", ret);
            continue;
        }
        
        // Print what we received
        printf("Slave received: 0x%02X", rx_byte);
        if (rx_byte >= 32 && rx_byte < 127) {
            printf(" ('%c')", rx_byte);
        }
        printf("\n");
        
        // Prepare response based on protocol
        switch (rx_byte) {
            case RECEIVE:
                printf("  -> Master wants to send data\n");
                tx_byte = rx_byte;  // Echo back RECEIVE as ACK
                break;
                
            case END:
                printf("  -> End of transmission\n");
                tx_byte = END;  // Echo END back
                break;
                
            case ERROR:
                printf("  -> Master sent ERROR\n");
                tx_byte = ERROR;
                break;
                
            default:
                // For data bytes, echo them back
                printf("  -> Echoing data byte back\n");
                tx_byte = rx_byte;
        }
        
        // Small delay to not overwhelm console
        if (rx_byte == END || rx_byte == ERROR) {
            delay_ms(100);
        }
    }
}

// Main application
void app_main(void) {
    // Setup
    printf("\n================================\n");
    printf("ESP32 SPI Slave Receiver\n");
    printf("================================\n\n");
    
    setup_spi_slave();
    
    // Choose which slave mode to run:
    printf("Select slave mode:\n");
    printf("1. Echo Mode (simple byte-by-byte)\n");
    printf("2. Protocol Mode (implements your protocol)\n");
    printf("Running Protocol Mode...\n\n");
    
    // Run the slave
    run_spi_slave_protocol();
    
    // Or for simpler debugging:
    // run_spi_slave_echo();
}
