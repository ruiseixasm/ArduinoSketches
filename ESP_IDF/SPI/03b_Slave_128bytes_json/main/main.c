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

// LED Pin (GPIO2 is typically the onboard blue LED)
#define LED_PIN  2

// 128 BYTES buffer (matching master)
#define BUFFER_SIZE 128

void delay_ms(uint32_t ms) {
    esp_rom_delay_us(ms * 1000);
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
    
    // Find actual string length (until null terminator)
    int str_len = 0;
    for (str_len = 0; str_len < bytes_received && buffer[str_len] != 0; str_len++);
    
    // Show the string content
    printf("String content (%d chars): ", str_len);
    for (int i = 0; i < str_len && i < 80; i++) {
        printf("%c", buffer[i]);
    }
    if (str_len > 80) printf("...");
    printf("\n");
    
    // Show first 16 bytes as hex
    printf("First 16 bytes (hex): ");
    for (int i = 0; i < 16 && i < bytes_received; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
    
    // Show last 16 bytes as hex
    printf("Last 16 bytes (hex):  ");
    int start = (bytes_received > 16) ? bytes_received - 16 : 0;
    for (int i = start; i < bytes_received; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
}

void parse_and_display_json_string(uint8_t *buffer, int bytes_received) {
    // Find the actual JSON string (up to null terminator or buffer end)
    int json_end = 0;
    for (json_end = 0; json_end < bytes_received; json_end++) {
        if (buffer[json_end] == 0 || buffer[json_end] == '|') {
            break;
        }
    }
    
    if (json_end == 0) {
        printf("✗ No JSON string found in buffer\n");
        return;
    }
    
    // Null-terminate the JSON string for parsing
    char json_str[128];
    int copy_len = json_end < sizeof(json_str) ? json_end : sizeof(json_str) - 1;
    memcpy(json_str, buffer, copy_len);
    json_str[copy_len] = '\0';
    
    printf("✓ Extracted JSON string: %s\n", json_str);
    
    // Check if it's ON or OFF command
    if (strstr(json_str, "'n':'ON'") != NULL) {
        printf("✓ COMMAND: SWITCH ON\n");
    } else if (strstr(json_str, "'n':'OFF'") != NULL) {
        printf("✓ COMMAND: SWITCH OFF\n");
    }
    
    // Try to manually parse the simple JSON-like structure
    // This is a simple parser for the specific format
    char *ptr = json_str;
    
    // Find device type
    char *t_start = strstr(ptr, "'t':'");
    if (t_start) {
        t_start += 5;
        char *t_end = strchr(t_start, '\'');
        if (t_end) {
            char device_type[32];
            int len = t_end - t_start;
            if (len > sizeof(device_type) - 1) len = sizeof(device_type) - 1;
            memcpy(device_type, t_start, len);
            device_type[len] = '\0';
            printf("  Device Type: %s\n", device_type);
        }
    }
    
    // Find mode
    char *m_start = strstr(ptr, "'m':");
    if (m_start) {
        m_start += 4;
        char *m_end = strchr(m_start, ',');
        if (m_end) {
            char mode[32];
            int len = m_end - m_start;
            if (len > sizeof(mode) - 1) len = sizeof(mode) - 1;
            memcpy(mode, m_start, len);
            mode[len] = '\0';
            printf("  Mode: %s\n", mode);
        }
    }
    
    // Find name (ON/OFF)
    char *n_start = strstr(ptr, "'n':'");
    if (n_start) {
        n_start += 5;
        char *n_end = strchr(n_start, '\'');
        if (n_end) {
            char name[32];
            int len = n_end - n_start;
            if (len > sizeof(name) - 1) len = sizeof(name) - 1;
            memcpy(name, n_start, len);
            name[len] = '\0';
            printf("  Name: %s\n", name);
        }
    }
    
    // Find function
    char *f_start = strstr(ptr, "'f':'");
    if (f_start) {
        f_start += 5;
        char *f_end = strchr(f_start, '\'');
        if (f_end) {
            char function[32];
            int len = f_end - f_start;
            if (len > sizeof(function) - 1) len = sizeof(function) - 1;
            memcpy(function, f_start, len);
            function[len] = '\0';
            printf("  Function: %s\n", function);
        }
    }
    
    // Find ID
    char *i_start = strstr(ptr, "'i':");
    if (i_start) {
        i_start += 4;
        char *i_end = strchr(i_start, ',');
        if (i_end) {
            char id_str[32];
            int len = i_end - i_start;
            if (len > sizeof(id_str) - 1) len = sizeof(id_str) - 1;
            memcpy(id_str, i_start, len);
            id_str[len] = '\0';
            printf("  ID: %s\n", id_str);
        }
    }
    
    // Find code
    char *c_start = strstr(ptr, "'c':");
    if (c_start) {
        c_start += 4;
        char *c_end = strchr(c_start, '}');
        if (c_end) {
            char code_str[32];
            int len = c_end - c_start;
            if (len > sizeof(code_str) - 1) len = sizeof(code_str) - 1;
            memcpy(code_str, c_start, len);
            code_str[len] = '\0';
            printf("  Code: %s\n", code_str);
        }
    }
}

void analyze_packet(uint8_t *buffer, int bytes_received) {
    // Parse and display the JSON string
    parse_and_display_json_string(buffer, bytes_received);
    
    // Check for transmission info (after the JSON string)
    char *info_start = NULL;
    for (int i = 0; i < bytes_received; i++) {
        if (buffer[i] == '|' && i + 1 < bytes_received && buffer[i+1] == 'C') {
            info_start = (char*)&buffer[i];
            break;
        }
    }
    
    if (info_start) {
        // Extract counter info
        char *cnt_start = strstr(info_start, "CNT:");
        if (cnt_start) {
            cnt_start += 4;
            char *cnt_end = strchr(cnt_start, '|');
            if (cnt_end) {
                char cnt_str[32];
                int len = cnt_end - cnt_start;
                if (len > sizeof(cnt_str) - 1) len = sizeof(cnt_str) - 1;
                memcpy(cnt_str, cnt_start, len);
                cnt_str[len] = '\0';
                printf("  Transmission Counter: %s\n", cnt_str);
            }
        }
    }
    
    // Check for status indicator at the end
    if (bytes_received >= 3) {
        if (buffer[bytes_received-3] == 'O' && buffer[bytes_received-2] == 'N') {
            printf("  Status Indicator: ON\n");
        } else if (buffer[bytes_received-3] == 'F' && buffer[bytes_received-2] == 'N') {
            printf("  Status Indicator: OFF\n");
        }
    }
    
    // Calculate XOR checksum of the entire buffer
    uint8_t checksum = 0;
    for (int i = 0; i < bytes_received; i++) {
        checksum ^= buffer[i];
    }
    printf("✓ XOR Checksum: 0x%02X\n", checksum);
}

void app_main() {
    printf("\n================================\n");
    printf("ESP32 SPI Slave (VSPI) - String Receiver\n");
    printf("Receiving ON/OFF commands from master\n");
    printf("================================\n\n");
    
    // Setup GPIO for LED first
    setup_gpio();
    
    setup_spi_slave();
    
    uint8_t rx_buffer[BUFFER_SIZE];
    uint8_t tx_buffer[BUFFER_SIZE];  // Optional: for sending response
    
    // Blink LED 3 times quickly to indicate startup
    for (int i = 0; i < 3; i++) {
        gpio_set_level(LED_PIN, 1);
        delay_ms(100);
        gpio_set_level(LED_PIN, 0);
        delay_ms(100);
    }
    
    // Initialize TX buffer (optional echo/response)
    memset(tx_buffer, 0x00, sizeof(tx_buffer));
    memcpy(tx_buffer, "{'status':'ACK'}", 16);  // Simple acknowledgment
    
    uint32_t packet_counter = 0;
    uint64_t last_packet_time = 0;
    bool last_command_was_on = false;
    
    printf("Waiting for ON/OFF commands from master...\n\n");
    printf("Expected strings:\n");
    printf("1. {'t':'Nano','m':2,'n':'ON','f':'Talker-9f','i':3540751170,'c':24893}\n");
    printf("2. {'t':'Nano','m':2,'n':'OFF','f':'Talker-9f','i':3540751170,'c':24893}\n\n");
    
    while (1) {
        // Clear receive buffer
        memset(rx_buffer, 0, sizeof(rx_buffer));
        
        // Setup transaction for receiving 128 bytes
        spi_slave_transaction_t trans = {
            .length = BUFFER_SIZE * 8,  // 1024 bits
            .rx_buffer = rx_buffer,
            .tx_buffer = tx_buffer  // Optional: send response while receiving
        };
        
        printf("\n--- Waiting for master transmission ---\n");
        
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
        
        printf("\n=== PACKET #%lu RECEIVED ===\n", packet_counter);
        printf("Transfer time: %lluµs\n", transfer_time);
        
        // Show what we received
        print_buffer_preview(rx_buffer, packet_counter, bytes_received);
        
        // Analyze the packet
        analyze_packet(rx_buffer, bytes_received);
        
        // Calculate time since last packet
        if (last_packet_time > 0) {
            uint64_t interval = esp_timer_get_time() - last_packet_time;
            printf("Time since last packet: %llums\n", interval / 1000);
            
            // Check if we're receiving alternating commands
            bool current_is_on = (strstr((char*)rx_buffer, "'n':'ON'") != NULL);
            if (packet_counter > 1) {
                if (current_is_on == last_command_was_on) {
                    printf("⚠ WARNING: Same command received twice in a row!\n");
                }
            }
            last_command_was_on = current_is_on;
        }
        last_packet_time = esp_timer_get_time();
        
        // Update TX buffer for next transaction (echo back command status)
        memset(tx_buffer, 0, sizeof(tx_buffer));
        if (strstr((char*)rx_buffer, "'n':'ON'") != NULL) {
        	gpio_set_level(LED_PIN, 1);
            snprintf((char*)tx_buffer, BUFFER_SIZE, "{'ack':'ON_RECEIVED','cnt':%lu}", packet_counter);
        } else if (strstr((char*)rx_buffer, "'n':'OFF'") != NULL) {
        	gpio_set_level(LED_PIN, 0);
            snprintf((char*)tx_buffer, BUFFER_SIZE, "{'ack':'OFF_RECEIVED','cnt':%lu}", packet_counter);
        } else {
            snprintf((char*)tx_buffer, BUFFER_SIZE, "{'ack':'UNKNOWN','cnt':%lu}", packet_counter);
        }
        
        printf("=== END OF PACKET ===\n");
    }
}
