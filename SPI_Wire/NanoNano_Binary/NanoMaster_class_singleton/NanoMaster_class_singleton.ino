// SPI Master Code - Pure String Commands
#include <SPI.h>
#include <ArduinoJson.h>    // Include ArduinoJson Library
#include "src/Master_class.hpp"

const int SS_PIN = 10;  // Slave Select pin
const int BUZZ_PIN = 2; // External BUZZER pin

// Master_class master_class = Master_class(SS_PIN);  // WRONG!
Master_class& master_class = Master_class::instance(SS_PIN);  // CORRECT!

void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(2000);    // Give some extra time to Slave start up completely

    pinMode(BUZZ_PIN, OUTPUT);
    digitalWrite(BUZZ_PIN, LOW);
    
    Serial.println(F("\n\nSPI Master Initialized - JSON class Mode"));
}


void loop() {
    if(master_class.ready()) {
        Serial.println(F("----------------------------**TESTING**----------------------------"));
        if(!master_class.test()) {
            Serial.println(F("----------------------------TEST FAILED----------------------------"));
            // digitalWrite(BUZZ_PIN, HIGH);
            // delay(500); // Buzzer on for 1/2 second
            // digitalWrite(BUZZ_PIN, LOW);
            delay(60000);    // Avoids fast loops of failure
        } else {
            Serial.println(F("----------------------------TEST PASSED----------------------------"));
        }
    } else {
        Serial.println(F("-------------------------**NOT READY**----------------------------"));
        delay(60000);    // Avoids fast loops of tries
    }
}


