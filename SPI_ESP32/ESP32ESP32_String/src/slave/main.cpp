#include <Arduino.h>
#include <driver/spi_slave.h>
#include <driver/gpio.h>
#include <esp_timer.h>

// VSPI Pins
#define MOSI_PIN GPIO_NUM_23  // VSPI MOSI
#define MISO_PIN GPIO_NUM_19  // VSPI MISO  
#define SCLK_PIN GPIO_NUM_18  // VSPI SCLK
#define CS_PIN   GPIO_NUM_5   // VSPI CS

// LED Pin
#ifndef LED_PIN
#define LED_PIN  GPIO_NUM_2  // Default if not defined by build flags
#endif

// 128 BYTES buffer
#define BUFFER_SIZE 128


spi_slave_transaction_t slave_trans;
uint8_t _rx_buffer[BUFFER_SIZE];
uint8_t _tx_buffer[BUFFER_SIZE];
uint32_t packet_counter = 0;
uint64_t last_packet_time = 0;
bool last_command_was_on = false;
bool led_state = false;

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
    
    gpio_set_level(LED_PIN, 0);
    Serial.println("LED configured");
}

void setup_spi_slave() {
    Serial.println("Setting up SPI Slave (VSPI) for 128-byte reception...");
    
    // SPI bus configuration for VSPI
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = MOSI_PIN;
    buscfg.miso_io_num = MISO_PIN;
    buscfg.sclk_io_num = SCLK_PIN;
    buscfg.quadwp_io_num = GPIO_NUM_NC;
    buscfg.quadhd_io_num = GPIO_NUM_NC;
    buscfg.max_transfer_sz = BUFFER_SIZE;
    
    // SPI slave configuration
    spi_slave_interface_config_t slvcfg = {};
    slvcfg.mode = 0;                  // SPI Mode 0
    slvcfg.spics_io_num = CS_PIN;     // CS pin
    slvcfg.queue_size = 2;            // Queue depth
    slvcfg.flags = 0;
    slvcfg.post_setup_cb = NULL;
    slvcfg.post_trans_cb = NULL;
    
    // Initialize SPI slave WITH DMA
    esp_err_t ret = spi_slave_initialize(VSPI_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        Serial.printf("SPI Slave init failed: %s\n", esp_err_to_name(ret));
        return;
    }
    
    Serial.println("SPI Slave (VSPI) initialized:");
    Serial.printf("  MOSI (RX): GPIO%d\n", MOSI_PIN);
    Serial.printf("  MISO (TX): GPIO%d\n", MISO_PIN);
    Serial.printf("  SCLK:      GPIO%d\n", SCLK_PIN);
    Serial.printf("  CS:        GPIO%d\n", CS_PIN);
    Serial.printf("  LED:       GPIO%d\n", LED_PIN);
    Serial.printf("  Buffer:    %d bytes\n\n", BUFFER_SIZE);
}

void print_buffer_preview(uint8_t *buffer, uint32_t packet_num, int bytes_received) {
    Serial.printf("[Packet #%lu - %d bytes]\n", packet_num, bytes_received);
    
    // Find actual string length
    int str_len = 0;
    for (str_len = 0; str_len < bytes_received && buffer[str_len] != 0; str_len++);
    
    Serial.printf("String content (%d chars): ", str_len);
    for (int i = 0; i < str_len && i < 80; i++) {
        Serial.print((char)buffer[i]);
    }
    if (str_len > 80) Serial.print("...");
    Serial.println();
    
    Serial.print("First 16 bytes (hex): ");
    for (int i = 0; i < 16 && i < bytes_received; i++) {
        Serial.printf("%02X ", buffer[i]);
    }
    Serial.println();
}

void parse_and_display_json_string(uint8_t *buffer, int bytes_received) {
    // Find the actual JSON string
    int json_end = 0;
    for (json_end = 0; json_end < bytes_received; json_end++) {
        if (buffer[json_end] == 0 || buffer[json_end] == '|') {
            break;
        }
    }
    
    if (json_end == 0) {
        Serial.println("✗ No JSON string found");
        return;
    }
    
    // Null-terminate the JSON string
    char json_str[128];
    int copy_len = json_end < sizeof(json_str) ? json_end : sizeof(json_str) - 1;
    memcpy(json_str, buffer, copy_len);
    json_str[copy_len] = '\0';
    
    Serial.printf("✓ JSON string: %s\n", json_str);
    
    // Check ON/OFF
    if (strstr(json_str, "'n':'ON'") != NULL) {
        Serial.println("✓ COMMAND: SWITCH ON");
        gpio_set_level(LED_PIN, 1);
        led_state = true;
    } else if (strstr(json_str, "'n':'OFF'") != NULL) {
        Serial.println("✓ COMMAND: SWITCH OFF");
        gpio_set_level(LED_PIN, 0);
        led_state = false;
    }
}

void analyze_packet(uint8_t *buffer, int bytes_received) {
    parse_and_display_json_string(buffer, bytes_received);
    
    // Calculate XOR checksum
    uint8_t checksum = 0;
    for (int i = 0; i < bytes_received; i++) {
        checksum ^= buffer[i];
    }
    Serial.printf("✓ XOR Checksum: 0x%02X\n", checksum);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n================================\n");
    Serial.println("ESP32 SPI Slave - String Receiver");
    Serial.println("Receiving ON/OFF commands from master");
    Serial.println("================================\n\n");
    
    // Setup GPIO for LED
    setup_gpio();
    
    // Setup SPI Slave
    setup_spi_slave();
    
    // Initialize TX buffer (echo/response)
    memset(_tx_buffer, 0x00, sizeof(_tx_buffer));
    memcpy(_tx_buffer, "{'status':'ACK'}", 16);
    
    // Blink LED 3 times for startup
    for (int i = 0; i < 3; i++) {
        gpio_set_level(LED_PIN, 1);
        delay(100);
        gpio_set_level(LED_PIN, 0);
        delay(100);
    }
    
    Serial.println("Waiting for ON/OFF commands from master...\n");
}

void loop() {
    // Clear receive buffer
    memset(_rx_buffer, 0, sizeof(_rx_buffer));
    
    // Setup transaction structure
    slave_trans.length = BUFFER_SIZE * 8;
    slave_trans.rx_buffer = _rx_buffer;
    slave_trans.tx_buffer = _tx_buffer;
    
    // Wait for master to initiate transfer (blocking)
    esp_err_t ret = spi_slave_transmit(VSPI_HOST, &slave_trans, portMAX_DELAY);
    
    if (ret != ESP_OK) {
        Serial.printf("SPI Slave error: %s\n", esp_err_to_name(ret));
        delay(100);
        return;
    }
    
    packet_counter++;
    
    // Calculate bytes received
    int bits_received = slave_trans.trans_len;
    int bytes_received = bits_received / 8;
    
    Serial.printf("\n=== PACKET #%lu RECEIVED ===\n", packet_counter);
    
    // Show what we received
    print_buffer_preview(_rx_buffer, packet_counter, bytes_received);
    
    // Analyze the packet
    analyze_packet(_rx_buffer, bytes_received);
    
    // Check if alternating commands
    bool current_is_on = (strstr((char*)_rx_buffer, "'n':'ON'") != NULL);
    if (packet_counter > 1 && current_is_on == last_command_was_on) {
        Serial.println("⚠ WARNING: Same command received twice!");
    }
    last_command_was_on = current_is_on;
    
    // Update TX buffer for next transaction
    memset(_tx_buffer, 0, sizeof(_tx_buffer));
    if (current_is_on) {
        snprintf((char*)_tx_buffer, BUFFER_SIZE, "{'ack':'ON_RECEIVED','cnt':%lu}", packet_counter);
    } else {
        snprintf((char*)_tx_buffer, BUFFER_SIZE, "{'ack':'OFF_RECEIVED','cnt':%lu}", packet_counter);
    }
    
    Serial.println("=== END OF PACKET ===\n");
    
    // Small delay
    delay(10);
}

