// SPI Master Code - Pure String Commands

#include "src/Slave_class.hpp"


ISR(SPI_STC_vect) {
    Slave_class::handleSPI_Interrupt();
}


// Create instance
Slave_class slave_class;    // Sets the LEDS


void setup() {
    // Initialize serial
    Serial.begin(115200);
    pinMode(GREEN_LED_PIN, OUTPUT);

    // Enable global interrupts
    sei();

    Serial.println("\n\nSPI Slave Initialized - JSON class Mode");
}


void loop() {
    slave_class.process();
}


