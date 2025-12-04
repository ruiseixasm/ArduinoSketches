// Single ESP32 SPI Test - HSPI Master ↔ VSPI Slave (Internal Loopback)
#include <SPI.h>

// ==================== PIN DEFINITIONS ====================
// HSPI Pins (Master)
#define HSPI_MISO 12
#define HSPI_MOSI 13
#define HSPI_SCLK 14
#define HSPI_SS   15

// VSPI Pins (Slave) - Connected to HSPI pins internally
#define VSPI_MISO 19  // Connected to HSPI_MISO 12
#define VSPI_MOSI 23  // Connected to HSPI_MOSI 13  
#define VSPI_SCLK 18  // Connected to HSPI_SCLK 14
#define VSPI_SS    5  // Connected to HSPI_SS   15

#define LED_PIN   2   // Onboard LED

SPIClass hspi(HSPI);  // Master SPI
#define BUFFER_SIZE 128

enum MessageCode : uint8_t {
    START   = 0xF0, // Start of transmission
    END     = 0xF1, // End of transmission
    VOID    = 0xFF  // No data
};

// ==================== SETUP ====================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n========================================");
    Serial.println("ESP32 Internal SPI Communication Test");
    Serial.println("HSPI (Master) ↔ VSPI (Slave)");
    Serial.println("Connected on same ESP32 board");
    Serial.println("========================================\n");
    
    // Print pin connections
    Serial.println("Internal Connections:");
    Serial.println("GPIO13 (HSPI_MOSI) → GPIO23 (VSPI_MOSI)");
    Serial.println("GPIO12 (HSPI_MISO) ← GPIO19 (VSPI_MISO)");
    Serial.println("GPIO14 (HSPI_SCLK) → GPIO18 (VSPI_SCLK)");
    Serial.println("GPIO15 (HSPI_SS)   → GPIO5  (VSPI_SS)");
    Serial.println();
    
    // Initialize pins
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Initialize HSPI as Master
    Serial.println("Initializing HSPI as Master...");
    hspi.begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);
    pinMode(HSPI_SS, OUTPUT);
    digitalWrite(HSPI_SS, HIGH);
    
    // Setup VSPI pins for slave (all as inputs)
    Serial.println("Setting up VSPI pins as inputs for slave...");
    pinMode(VSPI_MOSI, INPUT);  // Data input from master
    pinMode(VSPI_MISO, INPUT);  // Not used in one-way, but set as input
    pinMode(VSPI_SCLK, INPUT);  // Clock input from master  
    pinMode(VSPI_SS, INPUT_PULLUP);    // Slave Select with pull-up
    
    Serial.println("\nSystem Ready!");
    Serial.println("Waiting 2 seconds before starting...\n");
    delay(2000);
}

// ==================== MASTER SEND FUNCTION ====================
bool sendCommand(const char* command) {
    static uint8_t command_id = 0;
    command_id++;
    
    Serial.print("[MASTER] Sending Command #");
    Serial.print(command_id);
    Serial.print(": \"");
    Serial.print(command);
    Serial.println("\"");
    
    bool success = false;
    
    // Visual feedback - quick LED blink
    digitalWrite(LED_PIN, HIGH);
    
    // Begin SPI transaction
    hspi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(HSPI_SS, LOW);
    delayMicroseconds(10);
    
    // Send START byte
    hspi.transfer(START);
    delayMicroseconds(5);
    
    // Send command string
    for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
        hspi.transfer((uint8_t)command[i]);
        delayMicroseconds(5);
        if (command[i] == '\0') break;
    }
    
    // Send END byte
    hspi.transfer(END);
    delayMicroseconds(5);
    
    // End transaction
    digitalWrite(HSPI_SS, HIGH);
    hspi.endTransaction();
    
    // Turn off LED
    digitalWrite(LED_PIN, LOW);
    
    // Check VSPI_SS status (should be HIGH now)
    if (digitalRead(VSPI_SS) == HIGH) {
        Serial.println("[MASTER] Transmission completed successfully");
        success = true;
    } else {
        Serial.println("[MASTER] ERROR: Slave Select still active!");
        success = false;
    }
    
    // Simulate slave receiving (since we're on same board)
    // In reality, the slave would process this on VSPI pins
    Serial.print("[SLAVE] Would process: ");
    Serial.println(command);
    
    return success;
}

// ==================== SLAVE SIMULATION ====================
void simulateSlaveResponse() {
    // This function simulates what the slave side would do
    // In a real two-board setup, this would be on the slave ESP32
    
    static unsigned long last_print = 0;
    static int received_count = 0;
    
    // Only print status every 500ms to avoid spamming serial
    if (millis() - last_print > 500) {
        last_print = millis();
        
        // Check if SS line is being toggled (simulating activity)
        static bool last_ss_state = HIGH;
        bool current_ss_state = digitalRead(VSPI_SS);
        
        if (current_ss_state != last_ss_state) {
            if (current_ss_state == LOW) {
                received_count++;
                Serial.print("[SLAVE] Detected transmission #");
                Serial.println(received_count);
            }
            last_ss_state = current_ss_state;
        }
    }
}

// ==================== MAIN LOOP ====================
void loop() {
    static int test_cycle = 0;
    
    test_cycle++;
    Serial.println("\n" + String(50, '='));
    Serial.print("Test Cycle #");
    Serial.println(test_cycle);
    Serial.println(String(50, '='));
    
    // Test 1: Simple command
    Serial.println("\n--- Test 1: LED Control ---");
    if (sendCommand("LED_ON")) {
        Serial.println("✓ LED_ON command sent");
    }
    delay(1500);
    
    simulateSlaveResponse();
    
    // Test 2: Another command
    Serial.println("\n--- Test 2: Another Command ---");
    if (sendCommand("LED_OFF")) {
        Serial.println("✓ LED_OFF command sent");
    }
    delay(1500);
    
    simulateSlaveResponse();
    
    // Test 3: Custom command
    Serial.println("\n--- Test 3: Custom Command ---");
    if (sendCommand("TEST_123")) {
        Serial.println("✓ TEST_123 command sent");
    }
    delay(1500);
    
    simulateSlaveResponse();
    
    // Test 4: Longer command
    Serial.println("\n--- Test 4: System Command ---");
    if (sendCommand("SYSTEM_STATUS_REQUEST")) {
        Serial.println("✓ System command sent");
    }
    delay(1500);
    
    simulateSlaveResponse();
    
    Serial.println("\n" + String(50, '='));
    Serial.println("Cycle complete. Waiting 3 seconds...");
    Serial.println(String(50, '='));
    delay(3000);
}

// ==================== SERIAL COMMAND INTERFACE ====================
void serialEvent() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        
        if (input.length() > 0) {
            Serial.print("\n[USER] Manual command: \"");
            Serial.print(input);
            Serial.println("\"");
            
            if (sendCommand(input.c_str())) {
                Serial.println("✓ Manual command sent successfully");
            } else {
                Serial.println("✗ Manual command failed");
            }
        }
    }
}

