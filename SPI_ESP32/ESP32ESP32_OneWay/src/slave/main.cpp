#include <Arduino.h>
#include <SPI.h>
#include "SPI_Comm.h"

// ESP32 VSPI Pins (Slave)
// VSPI: GPIO18(SCK), GPIO19(MISO), GPIO23(MOSI), GPIO5(SS)
#define VSPI_SCK   18
#define VSPI_MISO  19
#define VSPI_MOSI  23
#define VSPI_SS    5

// Slave LED pin
#define SLAVE_LED 2

// Create VSPI instance
SPIClass* vspi = new SPIClass(VSPI);
SPI_Comm spiSlave(vspi, SLAVE_LED);

// Variables for SPI slave mode
volatile bool spiCommandReceived = false;
volatile uint8_t receivedCommand = 0;
volatile uint8_t receivedData = 0;

// Simple SPI Slave implementation
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
    // Simple SPI slave mode - listen for commands
    static uint8_t lastCommand = 0;
    static uint32_t lastCommandTime = 0;
    
    // Manually check if SS is LOW (Master is selecting us)
    if (digitalRead(VSPI_SS) == LOW) {
        // Read command from Master
        uint8_t cmd = SPI.transfer(0);
        uint8_t data = SPI.transfer(0);
        
        // Process the command
        uint8_t response = spiSlave.processCommand(cmd, data);
        
        // Send response back to Master
        SPI.transfer(response);
        
        if (cmd != lastCommand || millis() - lastCommandTime > 1000) {
            Serial.print("Command: 0x");
            Serial.print(cmd, HEX);
            Serial.print(", Response: 0x");
            Serial.println(response, HEX);
            
            // Visual feedback
            digitalWrite(SLAVE_LED, HIGH);
            delay(20);
            digitalWrite(SLAVE_LED, LOW);
            
            lastCommand = cmd;
            lastCommandTime = millis();
        }
    }
    
    // Blink slave LED slowly to show it's alive
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 2000) {
        lastBlink = millis();
        digitalWrite(SLAVE_LED, !digitalRead(SLAVE_LED));
        Serial.print("Slave alive - LED: ");
        Serial.println(digitalRead(SLAVE_LED) ? "ON" : "OFF");
    }
    
    delay(10);
}

