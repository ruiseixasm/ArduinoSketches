// Your protocol with 10µs timing - NO FreeRTOS

#include <stdio.h>
#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define GPIO_MOSI 13
#define GPIO_MISO 12
#define GPIO_SCLK 14
#define GPIO_CS 15

#define BUFFER_SIZE 64
#define RECEIVE 0x01
#define END 0x00
#define VOID 0xFF
#define ERROR 0xEE

spi_device_handle_t spi;

// Initialize SPI once
void setup_spi() {
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI,
        .miso_io_num = GPIO_MISO,
        .sclk_io_num = GPIO_SCLK
    };
    
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 8000000,  // 8 MHz
        .mode = 0,
        .spics_io_num = GPIO_CS,
        .queue_size = 1
    };
    
    spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_DISABLED);
    spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
}

// Single byte transfer with timing control
uint8_t spi_transfer_timed(uint8_t data, uint64_t* last_time, uint32_t delay_us) {
    if (last_time && *last_time > 0) {
        uint64_t now = esp_timer_get_time();
        int64_t elapsed = now - *last_time;
        if (elapsed < delay_us) {
            esp_rom_delay_us(delay_us - elapsed);
        }
    }
    
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
        .rx_buffer = &data
    };
    
    spi_device_transmit(spi, &t);
    
    if (last_time) {
        *last_time = esp_timer_get_time();
    }
    
    return data;
}

// Your sendString adapted
size_t sendString(const char* command) {
    size_t length = 0;
    uint8_t c;
    uint64_t last_time = 0;
    const uint32_t byte_delay_us = 10;  // 10µs between bytes
    
    for (size_t s = 0; length == 0 && s < 3; s++) {
        // CS low
        gpio_set_level(GPIO_CS, 0);
        esp_rom_delay_us(5);
        
        // Ask slave to receive
        c = spi_transfer_timed(RECEIVE, &last_time, byte_delay_us);
        
        for (uint8_t i = 0; i < BUFFER_SIZE + 1; i++) {
            esp_rom_delay_us(5);  // Your send_delay_us
            
            if (i > 0) {
                if (c == VOID) {
                    length = 1;
                    break;
                } else if (command[i - 1] == '\0') {
                    c = spi_transfer_timed(END, &last_time, byte_delay_us);
                    if (c == '\0') {
                        length = i;
                        break;
                    } else {
                        length = 0;
                        break;
                    }
                } else {
                    c = spi_transfer_timed(command[i], &last_time, byte_delay_us);
                }
                
                if (c != command[i - 1]) {
                    length = 0;
                    break;
                }
            } else if (command[0] != '\0') {
                c = spi_transfer_timed(command[0], &last_time, byte_delay_us);
            } else {
                length = 1;
                break;
            }
        }
        
        if (length == 0) {
            spi_transfer_timed(ERROR, &last_time, byte_delay_us);
        }
        
        esp_rom_delay_us(5);
        gpio_set_level(GPIO_CS, 1);
        
        if (length > 0) {
            printf("Success: %s\n", command);
        } else {
            printf("Failed, retrying...\n");
            // Buzzer simulation
            gpio_set_level(16, 1);  // Assuming buzzer on GPIO 16
            esp_rom_delay_us(10000);  // 10ms
            gpio_set_level(16, 0);
            esp_rom_delay_us(500000); // 500ms
        }
    }
    
    if (length > 0) length--;
    return length;
}

void app_main(void) {
    // Setup
    setup_spi();
    gpio_set_direction(GPIO_CS, GPIO_MODE_OUTPUT);
    gpio_set_direction(16, GPIO_MODE_OUTPUT);  // Buzzer
    
    printf("SPI Master - Your protocol, NO FreeRTOS\n");
    
    // Main loop
    while (1) {
        size_t len = sendString("TEST");
        printf("Sent length: %d\n", len);
        
        // Wait 1 second (non-FreeRTOS)
        for (int i = 0; i < 1000; i++) {
            esp_rom_delay_us(1000);  // 1ms
        }
    }
}
