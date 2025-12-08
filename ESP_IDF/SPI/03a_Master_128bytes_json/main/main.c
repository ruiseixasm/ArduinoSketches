// esp32_spi_master_128byte.c
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// SPI Pins (HSPI)
#define MOSI_PIN 13
#define MISO_PIN 12
#define SCLK_PIN 14
#define CS_PIN   15

// LED Pin (GPIO2 is typically the onboard blue LED)
#define LED_PIN  2

// 128 BYTES buffer (1024 bits)
#define BUFFER_SIZE 128

spi_device_handle_t spi;

// The two strings to send alternatively
const char* string_on = "{'t':'Nano','m':2,'n':'ON','f':'Talker-9f','i':3540751170,'c':24893}";
const char* string_off = "{'t':'Nano','m':2,'n':'OFF','f':'Talker-9f','i':3540751170,'c':24893}";

void delay_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void setup_gpio() {
    // Configure LED pin as output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Turn off LED initially
    gpio_set_level(LED_PIN, 0);
    
    printf("LED configured on GPIO%d\n", LED_PIN);
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
    printf("  LED:  GPIO%d\n", LED_PIN);
    printf("  Clock: 8 MHz\n");
    printf("  Buffer: %d bytes (1024 bits)\n", BUFFER_SIZE);
    printf("  Transfer time: ~128µs at 8MHz\n\n");
}

void fill_buffer(uint8_t *buffer, uint32_t counter, bool send_on_string) {
    // Clear buffer first
    memset(buffer, 0, BUFFER_SIZE);
    
    // Select which string to send
    const char* current_string = send_on_string ? string_on : string_off;
    size_t str_len = strlen(current_string);
    
    // Copy the string to buffer (ensuring it fits)
    if (str_len < BUFFER_SIZE) {
        memcpy(buffer, current_string, str_len);
    } else {
        memcpy(buffer, current_string, BUFFER_SIZE);
    }
    
    // Add transmission info at the end
    char info[32];
    snprintf(info, sizeof(info), "|CNT:%lu|", counter);
    size_t info_len = strlen(info);
    
    // Append info if there's space
    if (str_len + info_len < BUFFER_SIZE) {
        memcpy(&buffer[str_len], info, info_len);
    }
    
    // Add transmission status indicator
    if (BUFFER_SIZE > 10) {
        buffer[BUFFER_SIZE-3] = send_on_string ? 'O' : 'F';
        buffer[BUFFER_SIZE-2] = 'N';
        buffer[BUFFER_SIZE-1] = '\0';
    }
}

void send_128byte_buffer_with_led(uint8_t *buffer) {
    // Turn ON LED at start of transmission
    gpio_set_level(LED_PIN, 1);
    printf("💡 LED ON - Starting transmission...\n");
    
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
    
    // Keep LED on for 100ms total (including transmission time)
    uint64_t elapsed_time = esp_timer_get_time() - start;
    int64_t remaining_time = 100000 - elapsed_time; // 100ms in microseconds
    
    if (remaining_time > 0) {
        // Wait for remaining time to complete 100ms
        esp_rom_delay_us(remaining_time);
    }
    
    // Turn OFF LED after 100ms
    gpio_set_level(LED_PIN, 0);
    printf("💡 LED OFF after 100ms\n");
}

void print_buffer_preview(uint8_t *buffer, uint32_t counter, bool send_on_string) {
    printf("[Transmission #%lu - %s]\n", counter, send_on_string ? "ON" : "OFF");
    
    // Find actual string length (until null terminator)
    int str_len = 0;
    for (str_len = 0; str_len < BUFFER_SIZE && buffer[str_len] != 0; str_len++);
    
    printf("String (%d chars): ", str_len);
    for (int i = 0; i < str_len && i < 80; i++) {
        printf("%c", buffer[i]);
    }
    if (str_len > 80) printf("...");
    printf("\n");
    
    // Show first 16 bytes as hex
    printf("First 16 bytes (hex): ");
    for (int i = 0; i < 16; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
    
    // Show last 16 bytes as hex
    printf("Last 16 bytes (hex):  ");
    for (int i = BUFFER_SIZE-16; i < BUFFER_SIZE; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
}

void app_main() {
    printf("\n================================\n");
    printf("ESP32 SPI Master - 128-BYTE Transmitter\n");
    printf("Sending ON/OFF strings alternatively every 2 seconds\n");
    printf("Blue LED will blink for 100ms during each transfer\n");
    printf("================================\n\n");
    
    // Setup GPIO for LED first
    setup_gpio();
    
    // Setup SPI
    setup_spi_master();
    
    uint8_t tx_buffer[BUFFER_SIZE];
    uint32_t counter = 0;
    bool send_on_string = true;  // Start with ON string
    
    printf("Starting string transmission loop...\n\n");
    printf("String 1 (ON):  %s\n", string_on);
    printf("String 2 (OFF): %s\n\n", string_off);
    printf("String length - ON: %d chars, OFF: %d chars\n\n", 
           strlen(string_on), strlen(string_off));
    
    // Blink LED 3 times quickly to indicate startup
    for (int i = 0; i < 3; i++) {
        gpio_set_level(LED_PIN, 1);
        delay_ms(100);
        gpio_set_level(LED_PIN, 0);
        delay_ms(100);
    }
    
    while (1) {
        // Fill buffer with appropriate string
        fill_buffer(tx_buffer, counter, send_on_string);
        
        // Show preview
        print_buffer_preview(tx_buffer, counter, send_on_string);
        
        // Send the 128-byte buffer with LED indication
        send_128byte_buffer_with_led(tx_buffer);
        
        counter++;
        
        // Toggle between ON and OFF strings for next transmission
        send_on_string = !send_on_string;
        
        printf("\nWaiting 2 seconds...\n\n");
        
        // Wait exactly 2 seconds
        delay_ms(2000);
    }
}
