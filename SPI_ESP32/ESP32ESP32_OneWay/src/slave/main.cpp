#include <Arduino.h>
#include <SPI.h>
#include "SPI_Comm.h"

// ESP32 VSPI Pins (Slave)
// VSPI: GPIO18(SCK), GPIO19(MISO), GPIO23(MOSI), GPIO5(SS)*
#define VSPI_SCK   18
#define VSPI_MISO  19
#define VSPI_MOSI  23
#define VSPI_SS    5

// Slave LED pin
#define SLAVE_LED 2

// Create VSPI instance
SPIClass* vspi = new SPIClass(VSPI);
SPI_Comm spiSlave(vspi);

// Variables for SPI slave mode
volatile bool spiCommandReceived = false;
volatile uint8_t receivedCommand = 0;
volatile uint8_t receivedData = 0;

// Interrupt Service Routine for SPI Slave
void IRAM_ATTR spiSlaveISR() {
    spiCommandReceived = true;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("ESP32 SPI Slave Starting...");
    Serial.println("Using VSPI for Slave");
    
    pinMode(SLAVE_LED, OUTPUT);
    digitalWrite(SLAVE_LED, LOW);
    
    // Set up Slave Select pin with interrupt
    pinMode(VSPI_SS, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(VSPI_SS), spiSlaveISR, FALLING);
    
    // Initialize SPI in Slave mode
    SPI.begin(VSPI_SCK, VSPI_MISO, VSPI_MOSI, VSPI_SS);
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    SPI.setFrequency(1000000);
    
    Serial.println("SPI Slave initialized");
    Serial.println("Waiting for commands from Master...");
    Serial.print("SS Pin: GPIO"); Serial.println(VSPI_SS);
    Serial.println("=========================\n");
}

void loop() {
    // Check if a command was received via SPI
    if (spiCommandReceived) {
        // Wait a bit for the data to stabilize
        delayMicroseconds(10);
        
        // Read command from Master
        if (SPI.available()) {
            uint8_t cmd = SPI.transfer(0);
            uint8_t data = SPI.transfer(0);
            
            Serial.print("Received command: 0x");
            Serial.print(cmd, HEX);
            Serial.print(", data: 0x");
            Serial.println(data, HEX);
            
            // Process the command
            uint8_t response = spiSlave.processCommand(cmd, data, SLAVE_LED);
            
            // Send response back to Master
            SPI.transfer(response);
            
            Serial.print("Sent response: 0x");
            Serial.println(response, HEX);
            
            // Visual feedback
            digitalWrite(SLAVE_LED, HIGH);
            delay(50);
            digitalWrite(SLAVE_LED, LOW);
        }
        
        spiCommandReceived = false;
    }
    
    // Blink slave LED slowly to show it's alive
    static unsigned long lastBlink = 0;
    static bool ledState = false;
    if (millis() - lastBlink > 2000) {
        lastBlink = millis();
        ledState = !ledState;
        
        // Only blink if not controlled by master recently
        static unsigned long lastCommandTime = 0;
        if (millis() - lastCommandTime > 100) {
            digitalWrite(SLAVE_LED, ledState);
        }
        
        Serial.print(".");
    }
    
    delay(10);
}

