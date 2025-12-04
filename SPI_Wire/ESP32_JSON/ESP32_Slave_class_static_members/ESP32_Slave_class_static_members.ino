// SPI Master Code - Pure String Commands

#include "src/Slave_class.hpp"


// NO ISR on ESP32 - handled internally by SPI driver


// Create instance
Slave_class slave_class;    // Sets the LEDS


void setup() {
    // Initialize serial
    Serial.begin(115200);
    pinMode(GREEN_LED_PIN, OUTPUT);

    // NO sei() on ESP32 - FreeRTOS handles interrupts

    Serial.println("\n\nSPI Slave Initialized - JSON class Mode");
}


void loop() {
    slave_class.process();
}


