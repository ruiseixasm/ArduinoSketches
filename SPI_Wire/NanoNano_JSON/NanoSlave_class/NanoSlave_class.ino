// SPI Master Code - Pure String Commands
#include <SPI.h>
#include <ArduinoJson.h>    // Include ArduinoJson Library
#include "src/Slave_class.hpp"

const int GREEN_LED_PIN = 2; // External GREEN_LED_PIN pin
const int YELLOW_LED_PIN = 21;  // A7 = digital pin 21

ISR(SPI_STC_vect) {
    Slave_class::isrWrapper();
}

// Create instance
Slave_class slave_class;

void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(500);

    // Enable global interrupts
    sei();

    Serial.println("\n\nSPI Slave Initialized - JSON class Mode");
}

void loop() {
    // Process received messages
    if (slave_class.process()) {
        // Message was processed
        Serial.print(F("Processed: "));
        Serial.println(slave_class.getLastCommand());

        // Add small delay to prevent Serial flooding
        delay(10);
    }
}


