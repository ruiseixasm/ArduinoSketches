#include <Arduino.h>


// Most ESP32 dev boards have LED on GPIO 2
#define LED_PIN 2

void setup() {
    // put your setup code here, to run once:
    pinMode(LED_PIN, OUTPUT);
    Serial.begin(115200);
    Serial.println("ESP32 Blink Started!");
}

void loop() {
    // put your main code here, to run repeatedly:
    digitalWrite(LED_PIN, HIGH);
    Serial.println("LED ON");
    delay(1000);

    digitalWrite(LED_PIN, LOW);
    Serial.println("LED OFF");
    delay(1000);
}

