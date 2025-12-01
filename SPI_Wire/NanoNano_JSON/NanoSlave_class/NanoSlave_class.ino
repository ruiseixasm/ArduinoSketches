// SPI Master Code - Pure String Commands
#include <SPI.h>
#include <ArduinoJson.h>    // Include ArduinoJson Library
#include "src/Slave_class.hpp"

const int GREEN_LED_PIN = 2; // External GREEN_LED_PIN pin
const int YELLOW_LED_PIN = 21;  // A7 = digital pin 21

Slave_class slave_class = Slave_class(SS_PIN);

void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(500);

    Serial.println("\n\nSPI Slave Initialized - JSON class Mode");
}

void loop() {
    slave_class.read();
}


