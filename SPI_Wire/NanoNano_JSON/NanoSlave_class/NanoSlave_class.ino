// SPI Master Code - Pure String Commands
#include <SPI.h>
#include <ArduinoJson.h>    // Include ArduinoJson Library
#include "src/Slave_class.hpp"

const int SS_PIN = 10;  // Slave Select pin
const int BUZZ_PIN = 2; // External BUZZER pin

Slave_class master_class = Slave_class(SS_PIN);

void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(500);

    pinMode(BUZZ_PIN, OUTPUT);
    digitalWrite(BUZZ_PIN, LOW);
    
    Serial.println("\n\nSPI Slave Initialized - JSON class Mode");
}

void loop() {
    

}


