#include <Arduino.h>
#include <SPI.h>

// SPI Pins (HSPI on ESP32 - VSPI is default)
#define MOSI_PIN 13
#define MISO_PIN 12
#define SCLK_PIN 14
#define CS_PIN   15

// LED Pin (GPIO2 is typically the onboard blue LED)
#define LED_PIN  2

// 128 BYTES buffer (1024 bits)
#define BUFFER_SIZE 128

// The two strings to send alternatively
const char* string_on = "{'t':'Nano','m':2,'n':'ON','f':'Talker-9f','i':3540751170,'c':24893}";
const char* string_off = "{'t':'Nano','m':2,'n':'OFF','f':'Talker-9f','i':3540751170,'c':24893}";

// Use HSPI (SPI2) instead of default VSPI (SPI3)
SPIClass hspi(HSPI);

uint8_t _tx_buffer[BUFFER_SIZE];
uint32_t counter = 0;
bool send_on_string = true;  // Start with ON string

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
    digitalWrite(LED_PIN, HIGH);
    Serial.println("💡 LED ON - Starting transmission...");
    
    uint32_t start = micros();
    
    // Manual CS control
    digitalWrite(CS_PIN, LOW);
    
    // Send all 128 bytes using SPI transfer
    for (int i = 0; i < BUFFER_SIZE; i++) {
        hspi.transfer(buffer[i]);
    }
    
    digitalWrite(CS_PIN, HIGH);
    
    uint32_t duration = micros() - start;
    
    Serial.printf("✓ 128-byte buffer sent in %luµs\n", duration);
    
    // Keep LED on for 100ms total (including transmission time)
    uint32_t elapsed_time = micros() - start;
    int32_t remaining_time = 100000 - elapsed_time; // 100ms in microseconds
    
    if (remaining_time > 0) {
        // Wait for remaining time to complete 100ms
        delayMicroseconds(remaining_time);
    }
    
    // Turn OFF LED after 100ms
    digitalWrite(LED_PIN, LOW);
    Serial.println("💡 LED OFF after 100ms");
}

void print_buffer_preview(uint8_t *buffer, uint32_t counter, bool send_on_string) {
    Serial.printf("[Transmission #%lu - %s]\n", counter, send_on_string ? "ON" : "OFF");
    
    // Find actual string length (until null terminator)
    int str_len = 0;
    for (str_len = 0; str_len < BUFFER_SIZE && buffer[str_len] != 0; str_len++);
    
    Serial.printf("String (%d chars): ", str_len);
    for (int i = 0; i < str_len && i < 80; i++) {
        Serial.print((char)buffer[i]);
    }
    if (str_len > 80) Serial.print("...");
    Serial.println();
    
    // Show first 16 bytes as hex
    Serial.print("First 16 bytes (hex): ");
    for (int i = 0; i < 16; i++) {
        Serial.printf("%02X ", buffer[i]);
    }
    Serial.println();
    
    // Show last 16 bytes as hex
    Serial.print("Last 16 bytes (hex):  ");
    for (int i = BUFFER_SIZE-16; i < BUFFER_SIZE; i++) {
        Serial.printf("%02X ", buffer[i]);
    }
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n================================\n");
    Serial.println("ESP32 SPI Master - 128-BYTE Transmitter");
    Serial.println("Sending ON/OFF strings alternatively every 2 seconds");
    Serial.println("Blue LED will blink for 100ms during each transfer");
    Serial.println("================================\n\n");
    
    // Setup LED pin
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Setup CS pin
    pinMode(CS_PIN, OUTPUT);
    digitalWrite(CS_PIN, HIGH);
    
    // Initialize HSPI with custom pins
    hspi.begin(SCLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
    
    // Set SPI settings: 8MHz, MSB first, SPI Mode 0
    hspi.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    
    Serial.println("SPI Master initialized for 128-byte transfers:");
    Serial.printf("  MOSI: GPIO%d\n", MOSI_PIN);
    Serial.printf("  MISO: GPIO%d\n", MISO_PIN);
    Serial.printf("  SCLK: GPIO%d\n", SCLK_PIN);
    Serial.printf("  CS:   GPIO%d\n", CS_PIN);
    Serial.printf("  LED:  GPIO%d\n", LED_PIN);
    Serial.printf("  Clock: 8 MHz\n");
    Serial.printf("  Buffer: %d bytes (1024 bits)\n", BUFFER_SIZE);
    Serial.printf("  Transfer time: ~128µs at 8MHz\n\n");
    
    Serial.println("Starting string transmission loop...\n");
    Serial.printf("String 1 (ON):  %s\n", string_on);
    Serial.printf("String 2 (OFF): %s\n\n", string_off);
    Serial.printf("String length - ON: %d chars, OFF: %d chars\n\n", 
                  strlen(string_on), strlen(string_off));
    
    // Blink LED 3 times quickly to indicate startup
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }
}

void loop() {
    // Fill buffer with appropriate string
    fill_buffer(_tx_buffer, counter, send_on_string);
    
    // Show preview
    print_buffer_preview(_tx_buffer, counter, send_on_string);
    
    // Send the 128-byte buffer with LED indication
    send_128byte_buffer_with_led(_tx_buffer);
    
    counter++;
    
    // Toggle between ON and OFF strings for next transmission
    send_on_string = !send_on_string;
    
    Serial.println("\nWaiting 2 seconds...\n");
    
    // Wait exactly 2 seconds
    delay(2000);
}
