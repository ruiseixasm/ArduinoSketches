// SPI Master Code - Pure String Commands
#include <SPI.h>
#include <ArduinoJson.h>    // Include ArduinoJson Library
#include "src/Master_class.hpp"

// ESP32 pins for external SPI
const int SS_PIN = 5;  // Changed from 10 to 5 (ESP32 common)
const int BLUE_LED = 2; // ESP32 built-in LED


Master_class master_class = Master_class(SS_PIN);


void setup() {
    pinMode(BLUE_LED, OUTPUT);
    digitalWrite(BLUE_LED, HIGH);
    // Initialize serial
    Serial.begin(115200);
    delay(2000);    // Give some extra time to Slave start up completely
    digitalWrite(BLUE_LED, LOW);

    
    Serial.println("\n\nSPI Master Initialized - JSON class Mode");
}


void loop() {
    if(master_class.ready()) {
        Serial.println("----------------------------**TESTING**----------------------------");
        if(!master_class.test()) {
            Serial.println("----------------------------TEST FAILED----------------------------");
            digitalWrite(BLUE_LED, HIGH);
            delay(60000);    // Avoids fast loops of failure
            digitalWrite(BLUE_LED, LOW);
        } else {
            Serial.println("----------------------------TEST PASSED----------------------------");
        }
    } else {
        Serial.println("---------------------------**NOT READY**----------------------------");
        delay(60000);    // Avoids fast loops of tries
    }
}


