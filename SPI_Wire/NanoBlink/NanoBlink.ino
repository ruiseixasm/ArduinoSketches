/*
JsonTalkie - Json Talkie is intended for direct IoT communication.
Original Copyright (c) 2025 Rui Seixas Monteiro. All right reserved.
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.
https://github.com/ruiseixasm/JsonTalkie
*/

#include <SPI.h>

// Define LED pin with safety check
#define LED_PIN 2  // Change this to test different pins

// SPI pins on Arduino Nano
#define SPI_PINS_COUNT 4
const int spiPins[SPI_PINS_COUNT] = {10, 11, 12, 13};  // SS, MOSI, MISO, SCK

volatile bool commandReceived = false;
volatile byte spiData[32];
volatile byte dataIndex = 0;
bool ledAvailable = true;

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);

  delay(500);
  Serial.print("\n\nDefault Board LED_BUILTIN pin number: ");
  Serial.println(LED_BUILTIN);
  
  // Check if LED pin conflicts with SPI pins
  ledAvailable = true;
  for (int i = 0; i < SPI_PINS_COUNT; i++) {
    if (LED_PIN == spiPins[i]) {
      ledAvailable = false;
      break;
    }
  }
  
  // Initialize LED pin if safe
  if (ledAvailable) {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    delay(2000);
    digitalWrite(LED_PIN, LOW);
    Serial.print("LED initialized on pin ");
    Serial.println(LED_PIN);
  } else {
    Serial.println("ERROR: LED pin conflicts with SPI pins!");
    Serial.println("SPI pins: 10(SS), 11(MOSI), 12(MISO), 13(SCK)");
    Serial.println("Please change LED_PIN to a non-SPI pin");
  }
  
  // Initialize SPI as SLAVE (always done, regardless of LED)
  pinMode(MISO, OUTPUT);
  SPCR |= _BV(SPE);

//   // SET SPI MODE EXPLICITLY - CRITICAL!
//   SPCR = 0;  // Clear SPI settings
  
//   // Configure for SPI Mode 0 (most common)
//   SPCR |= _BV(SPE);    // SPI Enable
//   SPCR |= _BV(SPIE);   // SPI Interrupt Enable
//   // Mode 0: CPOL=0, CPHA=0
//   SPCR &= ~_BV(CPOL);  // Clock polarity 0
//   SPCR &= ~_BV(CPHA);  // Clock phase 0
  
//   // Optional: Set clock rate (though slave ignores this)
//   SPCR &= ~_BV(SPR1);  // Clear rate bits
//   SPCR &= ~_BV(SPR0);  // Clear rate bits


  SPI.attachInterrupt();
  
  Serial.println("Nano SPI Slave Ready");
}

ISR(SPI_STC_vect) {
  byte receivedByte = SPDR;
  
  if (dataIndex < sizeof(spiData)) {
    spiData[dataIndex++] = receivedByte;
    
    if (receivedByte == '\n' || dataIndex >= sizeof(spiData)) {
      commandReceived = true;
    }
  }
}

void processCommand() {
  String command = "";
  for (int i = 0; i < dataIndex; i++) {
    if (spiData[i] == '\n') break;
    command += (char)spiData[i];
  }
  
  command.trim();
  Serial.print("Received: ");
  Serial.println(command);
  
  // Process LED commands only if LED is available
  if (command == "LED_ON") {
    if (ledAvailable) {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED turned ON");
    } else {
      Serial.println("ERROR: Cannot control LED - pin conflict with SPI");
    }
  }
  else if (command == "LED_OFF") {
    if (ledAvailable) {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED turned OFF");
    } else {
      Serial.println("ERROR: Cannot control LED - pin conflict with SPI");
    }
  }
  else if (command == "LED_TOGGLE") {
    if (ledAvailable) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      Serial.println("LED toggled");
    } else {
      Serial.println("ERROR: Cannot control LED - pin conflict with SPI");
    }
  }
  else if (command == "LED_STATUS") {
    if (ledAvailable) {
      Serial.print("LED is currently: ");
      Serial.println(digitalRead(LED_PIN) ? "ON" : "OFF");
    } else {
      Serial.println("LED not available - SPI pin conflict");
    }
  }
  else if (command == "PING") {
    Serial.println("PONG - Nano is alive!");
  }
  else {
    Serial.println("Unknown command");
  }
  
  // Reset for next command
  dataIndex = 0;
  commandReceived = false;
}

void loop() {
  if (commandReceived) {
    processCommand();
  }
  
  // Your other code can run here safely
}
