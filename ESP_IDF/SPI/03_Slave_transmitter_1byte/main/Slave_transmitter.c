// esp32_spi_slave_fixed.c - COMPILES AND WORKS
#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "esp_rom_sys.h"  // ADD THIS for esp_rom_delay_us!

// Pins - CHANGE THESE to match your connections!
#define MOSI_PIN 23  // ESP32 GPIO23 <-- Master MOSI
#define MISO_PIN 19  // ESP32 GPIO19 --> Master MISO
#define SCLK_PIN 18  // ESP32 GPIO18 <-- Master SCK
#define CS_PIN   5   // ESP32 GPIO5  <-- Master SS

// Our SPDR register (like Arduino)
volatile uint8_t SPDR = 0x00;

// Bit-bang SPI Slave - Mode 0 (like Arduino default)
uint8_t spi_slave_transfer_byte(void) {
    uint8_t received = 0;
    
    // Read 8 bits, MSB first
    for (int bit = 7; bit >= 0; bit--) {
        // Wait for clock LOW
        while (gpio_get_level(SCLK_PIN) == 1) {
            if (gpio_get_level(CS_PIN) == 1) return 0; // CS went high
        }
        
        // Read MOSI
        if (gpio_get_level(MOSI_PIN)) {
            received |= (1 << bit);
        }
        
        // Write MISO (from SPDR)
        gpio_set_level(MISO_PIN, (SPDR >> bit) & 0x01);
        
        // Wait for clock HIGH
        while (gpio_get_level(SCLK_PIN) == 0) {
            if (gpio_get_level(CS_PIN) == 1) return 0; // CS went high
        }
    }
    
    return received;
}

void app_main(void) {
    printf("\n=== ESP32 SPI SLAVE (Bit-Bang) ===\n\n");
    
    // Setup GPIO
    gpio_set_direction(MOSI_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(MISO_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(SCLK_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(CS_PIN, GPIO_MODE_INPUT);
    
    // Initial states
    gpio_set_level(MISO_PIN, 0);
    SPDR = 0x00;  // First echo will be 0x00
    
    printf("Pins configured:\n");
    printf("  MOSI: GPIO%d (INPUT)\n", MOSI_PIN);
    printf("  MISO: GPIO%d (OUTPUT)\n", MISO_PIN);
    printf("  SCLK: GPIO%d (INPUT)\n", SCLK_PIN);
    printf("  CS:   GPIO%d (INPUT)\n", CS_PIN);
    printf("\nWaiting for Master...\n\n");
    
    int transaction_count = 0;
    
    while (1) {
        // Wait for CS LOW (Master selects us)
        while (gpio_get_level(CS_PIN) == 1) {
            esp_rom_delay_us(10);  // Small delay
        }
        
        transaction_count++;
        printf("--- Transaction %d (CS LOW) ---\n", transaction_count);
        
        int byte_count = 0;
        
        // Process all bytes while CS is LOW
        while (gpio_get_level(CS_PIN) == 0) {
            uint8_t received = spi_slave_transfer_byte();
            
            byte_count++;
            
            printf("  Byte %d: ", byte_count);
            printf("Master->Slave: 0x%02X", received);
            if (received >= 32 && received < 127) {
                printf(" ('%c')", received);
            }
            
            printf(" | Slave->Master: 0x%02X", SPDR);
            if (SPDR >= 32 && SPDR < 127) {
                printf(" ('%c')", SPDR);
            }
            printf("\n");
            
            // Set SPDR for NEXT byte (echo what we just received)
            SPDR = received;
            
            // Check if CS went high during byte transfer
            if (gpio_get_level(CS_PIN) == 1) {
                break;
            }
        }
        
        printf("--- End (CS HIGH, %d bytes) ---\n\n", byte_count);
        
        // Reset for next transaction
        SPDR = 0x00;
        
        // Small delay before next transaction
        esp_rom_delay_us(1000);
    }
}
