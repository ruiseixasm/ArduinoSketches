// RUN:
// 		pio run --project-dir .
// UPLOAD:
// 		pio run -t upload --project-dir .
// MONITOR:
//		pio device monitor --project-dir .


#include "Slave_class.hpp"


// ESP32 doesn't use ISR() macro like AVR
// We need to handle SPI differently
// For now, comment out the ISR


// Create instance
Slave_class slave_class;    // Sets the LEDS


void setup() {
    // Initialize serial
    Serial.begin(115200);
    pinMode(GREEN_LED_PIN, OUTPUT);

    // ESP32 doesn't have sei() - FreeRTOS handles interrupts
    // sei();  // AVR only - remove for ESP32

    Serial.println("\n\nSPI Slave Initialized - JSON class Mode");
}


void loop() {
    slave_class.process();
}



