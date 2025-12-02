// SPI Master Code - Pure String Commands

#include "src/Slave_class.hpp"


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
    slave_class.process();
}


