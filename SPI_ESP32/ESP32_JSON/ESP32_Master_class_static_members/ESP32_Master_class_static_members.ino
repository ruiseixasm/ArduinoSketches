// SPI Master Code - Pure String Commands
#include <SPI.h>
#include <ArduinoJson.h>    // Include ArduinoJson Library
#include "src/Master_class.hpp"

// ESP32 pins for external SPI
const int SS_PIN = 5;  // Changed from 10 to 5 (ESP32 common)
const int BUZZ_PIN = 2; // ESP32 built-in LED


Master_class master_class = Master_class(SS_PIN);


void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(2000);    // Give some extra time to Slave start up completely

    pinMode(BUZZ_PIN, OUTPUT);
    digitalWrite(BUZZ_PIN, LOW);
    
    Serial.println("\n\nSPI Master Initialized - JSON class Mode");
}


void loop() {
    if(master_class.ready()) {
        Serial.println("----------------------------**TESTING**----------------------------");
        if(!master_class.test()) {
            Serial.println("----------------------------TEST FAILED----------------------------");
            digitalWrite(BUZZ_PIN, HIGH);
            delay(500); // Buzzer on for 1/2 second
            digitalWrite(BUZZ_PIN, LOW);
            delay(60000);    // Avoids fast loops of failure
        } else {
            Serial.println("----------------------------TEST PASSED----------------------------");
        }
    } else {
        Serial.println("---------------------------**NOT READY**----------------------------");
        delay(60000);    // Avoids fast loops of tries
    }
}


