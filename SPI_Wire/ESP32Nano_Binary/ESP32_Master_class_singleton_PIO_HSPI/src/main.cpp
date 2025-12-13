// COMPILE:
// 		pio run --project-dir .
// UPLOAD:
// 		pio run -t upload --project-dir .
// MONITOR:
//		pio device monitor --project-dir .


// SPI Master Code - Pure String Commands
#include <SPI.h>
#include <ArduinoJson.h>    // Include ArduinoJson Library
#include "Master_class.hpp"

// ESP32 pins for external SPI
const int SS_PIN = 4;  // D4 because it's the specific case of the big board
const int BLUE_LED = 2; // ESP32 built-in LED


// Master_class master_class = Master_class(SS_PIN);  // WRONG!
Master_class& master_class = Master_class::instance(SS_PIN);  // CORRECT!


void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(100);		// Give some time for serial to begin
    Serial.println("\n\nSPI Master Initialized - SINGLETON class Mode");
    pinMode(BLUE_LED, OUTPUT);
    digitalWrite(BLUE_LED, HIGH);
    delay(3000);    // Give some extra time to Slave start up completely
    digitalWrite(BLUE_LED, LOW);

    Serial.println(F("Testing Yellow remote led"));
	for (uint8_t i = 0; i < 10; i++) {
		master_class.yellow_on();
		delay(50);
		master_class.yellow_off();
		delay(100);
	}
}


void loop() {
    if(master_class.ready()) {
        Serial.println(F("----------------------------**TESTING**----------------------------"));
        if(!master_class.test()) {
			master_class.yellow_on();
            Serial.println(F("----------------------------TEST FAILED----------------------------"));
            digitalWrite(BLUE_LED, HIGH);
            delay(60000);    // Avoids fast loops of failure
            digitalWrite(BLUE_LED, LOW);
			master_class.yellow_off();
        } else {
            Serial.println(F("----------------------------TEST PASSED----------------------------"));
        }
    } else {
        Serial.println(F("-------------------------**NOT READY**----------------------------"));
        delay(60000);    // Avoids fast loops of tries
    }
}


