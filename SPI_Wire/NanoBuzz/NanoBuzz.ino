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

#define BUZZER_PIN 2  // Buzzer on pin 2

// SPI pins on Arduino Nano
#define SPI_PINS_COUNT 4
const int spiPins[SPI_PINS_COUNT] = {10, 11, 12, 13};  // SS, MOSI, MISO, SCK

volatile bool commandReceived = false;
volatile byte spiData[32];
volatile byte dataIndex = 0;

void setup() {
    
  // Initialize Serial for debugging
  Serial.begin(115200);
  delay(500);
  Serial.print("\n\n\nI'M A BUZZER!!");
  Serial.print("\nDefault Board LED_BUILTIN pin number: ");
  Serial.println(LED_BUILTIN);
  
  // Initialize Buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("Buzzer initialized on pin 2");
  
  // Initialize SPI as SLAVE
  pinMode(MISO, OUTPUT);
  SPCR |= _BV(SPE);
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

void buzz() {
  Serial.println("BUZZ! 200ms beep");
  digitalWrite(BUZZER_PIN, HIGH);  // Turn buzzer on
  delay(200);                      // Wait 200ms
  digitalWrite(BUZZER_PIN, LOW);   // Turn buzzer off automatically
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
  
  if (command == "BUZZ") {
    buzz();  // Execute the buzz command - automatically turns off after 200ms
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
}

