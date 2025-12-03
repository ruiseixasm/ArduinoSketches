// SPI Master Code - Pure String Commands

#include "src/Slave_class.hpp"


ISR(SPI_STC_vect) {
    Slave_class::isrWrapper();
}


// Create instance
Slave_class slave_class;    // Sets the LEDS


void setup() {
    // Initialize serial
    Serial.begin(115200);
    pinMode(GREEN_LED_PIN, OUTPUT);
    digitalWrite(YELLOW_LED_PIN, HIGH);
    delay(500);
    digitalWrite(YELLOW_LED_PIN, LOW);

    // Enable global interrupts
    sei();

    Serial.println("\n\nSPI Slave Initialized - JSON class Mode");
    Serial.print("YELLOW_LED_PIN: ");  // Led 13 is already used by SCK
    Serial.println(YELLOW_LED_PIN);
}


void loop() {
    slave_class.process();
}


