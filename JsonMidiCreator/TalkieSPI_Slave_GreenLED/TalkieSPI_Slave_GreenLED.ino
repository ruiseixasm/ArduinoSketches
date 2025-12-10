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


// LED_BUILTIN is already defined by ESP32 platform
// Typically GPIO2 for most ESP32 boards
#ifndef LED_BUILTIN
  #define LED_BUILTIN 2  // Fallback definition if not already defined
#endif



// COMPILE WITH ARDUINO BOARD
#include "src/sockets/BroadcastSocket_SPI_ESP_Arduino_Slave.hpp"
#include "src/JsonTalker.h"

const char talker_name[] = "talker";
const char talker_desc[] = "I'm a talker";
JsonTalker talker = JsonTalker(talker_name, talker_desc);
JsonTalker* talkers[] = { &talker };   // It's an array of pointers
// Singleton requires the & (to get a reference variable)

auto& broadcast_socket = BroadcastSocket_SPI_ESP_Arduino_Slave::instance(talkers, sizeof(talkers)/sizeof(JsonTalker*));



// Buzzer pin
#define buzzer_pin 3



void setup() {
    // Initialize pins FIRST before anything else
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); // Start with LED off
    
    #ifndef BROADCAST_SOCKET_SERIAL_HPP
    pinMode(buzzer_pin, OUTPUT);
    digitalWrite(buzzer_pin, LOW);
    #endif

    // Then start Serial
    Serial.begin(115200);
    delay(2000); // Important: Give time for serial to initialize
    Serial.println("\n\n=== ESP32 with EthernetENC STARTING ===");

    // Add a small LED blink to confirm code is running
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    
    Serial.println("Pins initialized successfully");



    // STEP 1: Initialize SPI only
    // Defines the CS pin by Talker name here
	
    Serial.println("Step 1: Starting SPI...");
    Serial.println("SPI started successfully");
    delay(1000);

    // Connect the talkers with each other (static variable)
    Serial.println("Connecting Talkers with each other");
    JsonTalker::connectTalkers(talkers, sizeof(talkers)/sizeof(JsonTalker*));

    // Final startup indication
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("Setup completed - Ready for JSON communication!");
}



void loop() {
    broadcast_socket.receive();
}

