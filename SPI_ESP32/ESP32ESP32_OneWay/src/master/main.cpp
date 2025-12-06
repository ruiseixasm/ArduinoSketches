#include <Arduino.h>
#include "SPI_Comm.h"

// ESP32 HSPI Pins (Master)
// HSPI: GPIO14(SCK), GPIO12(MISO), GPIO13(MOSI), GPIO15(SS)*
#define HSPI_SCK   14
#define HSPI_MISO  12
#define HSPI_MOSI  13
#define HSPI_SS    15

// Master LED pin
#define MASTER_LED 2

// Create HSPI instance
SPIClass* hspi = new SPIClass(HSPI);
SPI_Comm spiMaster(HSPI_SS, hspi);

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("ESP32 SPI Master Starting...");
    Serial.println("Using HSPI for Master");
    
    pinMode(MASTER_LED, OUTPUT);
    digitalWrite(MASTER_LED, LOW);
    
    // Initialize SPI Master
    spiMaster.beginMaster();
    
    Serial.println("SPI Master initialized");
    Serial.println("Waiting 2 seconds for slave to be ready...");
    delay(2000);
    
    Serial.println("\n=== Master Ready ===");
    Serial.println("Send commands via Serial Monitor:");
    Serial.println("1 - Turn Slave LED ON");
    Serial.println("2 - Turn Slave LED OFF");
    Serial.println("3 - Toggle Slave LED");
    Serial.println("4 - Get Slave LED status");
    Serial.println("5 - Ping Slave");
    Serial.println("=========================\n");
}

void loop() {
    // Check for serial commands
    if (Serial.available()) {
        char command = Serial.read();
        
        switch(command) {
            case '1': // Turn LED ON
                digitalWrite(MASTER_LED, HIGH);
                Serial.print("Sending LED ON command... ");
                if (spiMaster.ledOn()) {
                    Serial.println("SUCCESS - Slave LED turned ON");
                } else {
                    Serial.println("FAILED - No response from slave");
                }
                digitalWrite(MASTER_LED, LOW);
                break;
                
            case '2': // Turn LED OFF
                digitalWrite(MASTER_LED, HIGH);
                Serial.print("Sending LED OFF command... ");
                if (spiMaster.ledOff()) {
                    Serial.println("SUCCESS - Slave LED turned OFF");
                } else {
                    Serial.println("FAILED - No response from slave");
                }
                digitalWrite(MASTER_LED, LOW);
                break;
                
            case '3': // Toggle LED
                digitalWrite(MASTER_LED, HIGH);
                Serial.print("Sending LED TOGGLE command... ");
                if (spiMaster.ledToggle()) {
                    Serial.println("SUCCESS - Slave LED toggled");
                } else {
                    Serial.println("FAILED - No response from slave");
                }
                digitalWrite(MASTER_LED, LOW);
                break;
                
            case '4': // Get LED status
                digitalWrite(MASTER_LED, HIGH);
                Serial.print("Getting LED status... ");
                if (spiMaster.getLedStatus()) {
                    Serial.println("Slave LED is ON");
                } else {
                    Serial.println("Slave LED is OFF");
                }
                digitalWrite(MASTER_LED, LOW);
                break;
                
            case '5': // Ping slave
                digitalWrite(MASTER_LED, HIGH);
                Serial.print("Pinging slave... ");
                if (spiMaster.ping()) {
                    Serial.println("SUCCESS - Slave is responding");
                } else {
                    Serial.println("FAILED - Slave not responding");
                }
                digitalWrite(MASTER_LED, LOW);
                break;
                
            case '\n':
            case '\r':
                // Ignore newlines
                break;
                
            default:
                Serial.println("Unknown command. Use 1-5");
                break;
        }
    }
    
    // Blink master LED to show it's alive
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 1000) {
        lastBlink = millis();
        digitalWrite(MASTER_LED, !digitalRead(MASTER_LED));
    }
    
    delay(10);
}

