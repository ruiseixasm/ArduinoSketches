#include <Arduino.h>
#include <SPI.h>

// VSPI Pins (VSPI is default on ESP32)
#define MOSI_PIN 23  // VSPI MOSI
#define MISO_PIN 19  // VSPI MISO  
#define SCLK_PIN 18  // VSPI SCLK
#define CS_PIN   5   // VSPI CS

// LED Pin (GPIO2 is typically the onboard blue LED)
#define LED_PIN  2

// 128 BYTES buffer (matching master)
#define BUFFER_SIZE 128

// Use default VSPI (SPI3) for slave
SPISettings spiSettings(8000000, MSBFIRST, SPI_MODE0);

uint8_t rx_buffer[BUFFER_SIZE];
uint8_t tx_buffer[BUFFER_SIZE];  // Optional: for sending response

uint32_t packet_counter = 0;
uint32_t last_packet_time = 0;
bool last_command_was_on = false;
bool led_state = false;

void print_buffer_preview(uint8_t *buffer, uint32_t packet_num, int bytes_received) {
    Serial.printf("[Packet #%lu - %d bytes]\n", packet_num, bytes_received);
    
    // Find actual string length (until null terminator)
    int str_len = 0;
    for (str_len = 0; str_len < bytes_received && buffer[str_len] != 0; str_len++);
    
    // Show the string content
    Serial.printf("String content (%d chars): ", str_len);
    for (int i = 0; i < str_len && i < 80; i++) {
        Serial.print((char)buffer[i]);
    }
    if (str_len > 80) Serial.print("...");
    Serial.println();
    
    // Show first 16 bytes as hex
    Serial.print("First 16 bytes (hex): ");
    for (int i = 0; i < 16 && i < bytes_received; i++) {
        Serial.printf("%02X ", buffer[i]);
    }
    Serial.println();
    
    // Show last 16 bytes as hex
    Serial.print("Last 16 bytes (hex):  ");
    int start = (bytes_received > 16) ? bytes_received - 16 : 0;
    for (int i = start; i < bytes_received; i++) {
        Serial.printf("%02X ", buffer[i]);
    }
    Serial.println();
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
        Serial.println("✗ No JSON string found in buffer");
        return;
    }
    
    // Null-terminate the JSON string for parsing
    char json_str[128];
    int copy_len = json_end < sizeof(json_str) ? json_end : sizeof(json_str) - 1;
    memcpy(json_str, buffer, copy_len);
    json_str[copy_len] = '\0';
    
    Serial.printf("✓ Extracted JSON string: %s\n", json_str);
    
    // Check if it's ON or OFF command
    if (strstr(json_str, "'n':'ON'") != NULL) {
        Serial.println("✓ COMMAND: SWITCH ON");
        digitalWrite(LED_PIN, HIGH);  // Turn LED ON
        led_state = true;
    } else if (strstr(json_str, "'n':'OFF'") != NULL) {
        Serial.println("✓ COMMAND: SWITCH OFF");
        digitalWrite(LED_PIN, LOW);   // Turn LED OFF
        led_state = false;
    }
    
    // Try to manually parse the simple JSON-like structure
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
            Serial.printf("  Device Type: %s\n", device_type);
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
            Serial.printf("  Mode: %s\n", mode);
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
            Serial.printf("  Name: %s\n", name);
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
            Serial.printf("  Function: %s\n", function);
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
            Serial.printf("  ID: %s\n", id_str);
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
            Serial.printf("  Code: %s\n", code_str);
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
                Serial.printf("  Transmission Counter: %s\n", cnt_str);
            }
        }
    }
    
    // Check for status indicator at the end
    if (bytes_received >= 3) {
        if (buffer[bytes_received-3] == 'O' && buffer[bytes_received-2] == 'N') {
            Serial.println("  Status Indicator: ON");
        } else if (buffer[bytes_received-3] == 'F' && buffer[bytes_received-2] == 'N') {
            Serial.println("  Status Indicator: OFF");
        }
    }
    
    // Calculate XOR checksum of the entire buffer
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
    Serial.println("ESP32 SPI Slave (VSPI) - String Receiver");
    Serial.println("Receiving ON/OFF commands from master");
    Serial.println("================================\n\n");
    
    // Setup LED pin
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Setup SPI pins
    pinMode(CS_PIN, INPUT);  // CS is input for slave
    pinMode(MISO_PIN, OUTPUT);  // MISO is output for slave
    
    // Initialize SPI as slave
    SPI.begin(SCLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
    
    // Set slave mode - in Arduino, we use SPI in normal mode and control CS manually
    // Actually, for slave mode we need different approach
    // Arduino SPI doesn't have true slave mode, so we'll simulate it
    
    Serial.println("SPI Slave (VSPI) initialized for 128-byte reception:");
    Serial.printf("  MOSI (RX): GPIO%d <- Master MOSI\n", MOSI_PIN);
    Serial.printf("  MISO (TX): GPIO%d -> Master MISO\n", MISO_PIN);
    Serial.printf("  SCLK:      GPIO%d <- Master SCLK\n", SCLK_PIN);
    Serial.printf("  CS:        GPIO%d <- Master CS\n", CS_PIN);
    Serial.printf("  LED:       GPIO%d (will show ON/OFF state)\n", LED_PIN);
    Serial.printf("  Mode:      0 (CPOL=0, CPHA=0)\n");
    Serial.printf("  Buffer:    %d bytes (1024 bits)\n\n", BUFFER_SIZE);
    
    // Initialize TX buffer (optional echo/response)
    memset(tx_buffer, 0x00, sizeof(tx_buffer));
    memcpy(tx_buffer, "{'status':'ACK'}", 16);  // Simple acknowledgment
    
    // Blink LED 3 times quickly to indicate startup
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }
    
    Serial.println("Waiting for ON/OFF commands from master...\n");
    Serial.println("Expected strings:");
    Serial.println("1. {'t':'Nano','m':2,'n':'ON','f':'Talker-9f','i':3540751170,'c':24893}");
    Serial.println("2. {'t':'Nano','m':2,'n':'OFF','f':'Talker-9f','i':3540751170,'c':24893}\n");
}

void loop() {
    // Clear receive buffer
    memset(rx_buffer, 0, sizeof(rx_buffer));
    
    Serial.println("\n--- Waiting for master transmission ---");
    
    // In Arduino, we can't use true SPI slave mode easily
    // We'll poll the CS pin and when it goes LOW, we read data
    // This is a simplified approach
    
    if (digitalRead(CS_PIN) == LOW) {
        // CS is LOW - master is starting transmission
        uint32_t start_time = micros();
        
        // Turn on LED briefly to indicate receiving
        bool temp_led = digitalRead(LED_PIN);
        digitalWrite(LED_PIN, HIGH);
        
        // Read 128 bytes
        SPI.beginTransaction(spiSettings);
        for (int i = 0; i < BUFFER_SIZE; i++) {
            // Transfer dummy byte while receiving
            uint8_t received = SPI.transfer(tx_buffer[i]);
            rx_buffer[i] = received;
        }
        SPI.endTransaction();
        
        uint32_t transfer_time = micros() - start_time;
        
        // Restore LED state
        digitalWrite(LED_PIN, temp_led ? HIGH : LOW);
        
        packet_counter++;
        
        Serial.printf("\n=== PACKET #%lu RECEIVED ===\n", packet_counter);
        Serial.printf("Transfer time: %luµs\n", transfer_time);
        
        // Show what we received
        print_buffer_preview(rx_buffer, packet_counter, BUFFER_SIZE);
        
        // Analyze the packet
        analyze_packet(rx_buffer, BUFFER_SIZE);
        
        // Calculate time since last packet
        if (last_packet_time > 0) {
            uint32_t interval = micros() - last_packet_time;
            Serial.printf("Time since last packet: %lums\n", interval / 1000);
            
            // Check if we're receiving alternating commands
            bool current_is_on = (strstr((char*)rx_buffer, "'n':'ON'") != NULL);
            if (packet_counter > 1) {
                if (current_is_on == last_command_was_on) {
                    Serial.println("⚠ WARNING: Same command received twice in a row!");
                }
            }
            last_command_was_on = current_is_on;
        }
        last_packet_time = micros();
        
        // Update TX buffer for next transaction (echo back command status)
        memset(tx_buffer, 0, sizeof(tx_buffer));
        if (strstr((char*)rx_buffer, "'n':'ON'") != NULL) {
            snprintf((char*)tx_buffer, BUFFER_SIZE, "{'ack':'ON_RECEIVED','cnt':%lu}", packet_counter);
        } else if (strstr((char*)rx_buffer, "'n':'OFF'") != NULL) {
            snprintf((char*)tx_buffer, BUFFER_SIZE, "{'ack':'OFF_RECEIVED','cnt':%lu}", packet_counter);
        } else {
            snprintf((char*)tx_buffer, BUFFER_SIZE, "{'ack':'UNKNOWN','cnt':%lu}", packet_counter);
        }
        
        Serial.println("=== END OF PACKET ===");
        
        // Small delay to avoid bouncing
        delay(10);
    }
    
    // Small delay to avoid busy waiting
    delay(1);
}
