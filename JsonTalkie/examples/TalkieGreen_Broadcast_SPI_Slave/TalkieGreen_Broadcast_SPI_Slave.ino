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


// GREEN_LED is already defined by ESP32 platform
// Typically GPIO2 for most ESP32 boards
#ifndef GREEN_LED
  #define GREEN_LED 2  // Fallback definition if not already defined
#endif

#ifndef YELLOW_LED_PIN
#define YELLOW_LED_PIN 19   // A5
#endif


#include <JsonTalkie.hpp>
#include "M_GreenManifesto.hpp"
#include "M_LedManifesto.hpp"
#include "S_Basic_SPI_Arduino_Slave.h"


const char green_name[] = "green";
const char green_desc[] = "I'm a green talker";
M_GreenManifesto green_manifesto;
JsonTalker green = JsonTalker(green_name, green_desc, &green_manifesto);

const char yellow_name[] = "yellow";
const char yellow_desc[] = "I'm a yellow talker";
M_LedManifesto yellow_manifesto(YELLOW_LED_PIN);
JsonTalker yellow = JsonTalker(yellow_name, yellow_desc, &yellow_manifesto);

// Singleton requires the & (to get a reference variable)
auto& spi_socket = S_Basic_SPI_Arduino_Slave::instance();

// SETTING THE REPEATER
BroadcastSocket* uplinked_sockets[] = { &spi_socket };
JsonTalker* downlinked_talkers[] = { &green, &yellow };
const MessageRepeater message_repeater(
		uplinked_sockets, sizeof(uplinked_sockets)/sizeof(BroadcastSocket*),
		downlinked_talkers, sizeof(downlinked_talkers)/sizeof(JsonTalker*)
	);


void setup() {
    // Initialize pins FIRST before anything else
    pinMode(GREEN_LED, OUTPUT);
    digitalWrite(GREEN_LED, LOW); // Start with LED off
	pinMode(YELLOW_LED_PIN, OUTPUT);
	digitalWrite(YELLOW_LED_PIN, LOW);

    // Then start Serial
    Serial.begin(115200);
    delay(100); // Important: Give time for serial to initialize
    digitalWrite(YELLOW_LED_PIN, HIGH);
    digitalWrite(YELLOW_LED_PIN, LOW);
    Serial.println(F("\n\n=== Arduino with SPI STARTING ==="));

    // Add a small LED blink to confirm code is running
    digitalWrite(GREEN_LED, HIGH);
    delay(100);
    digitalWrite(GREEN_LED, LOW);
    delay(100);
    digitalWrite(GREEN_LED, HIGH);
    delay(100);
    digitalWrite(GREEN_LED, LOW);
    
    Serial.println(F("Pins initialized successfully"));



    // STEP 1: Initialize SPI only
    // Defines the CS pin by Talker name here
	
    Serial.println(F("Step 1: Starting SPI..."));
	spi_socket.bridgeSocket();	// Makes sure it accepts LOCAL messages too
    Serial.println(F("SPI started successfully"));

    // Final startup indication
    digitalWrite(GREEN_LED, HIGH);
    delay(500);
    digitalWrite(GREEN_LED, LOW);
    delay(5000);	// Waits 5 seconds for all other SPIs to start (it's a slave, so, it depends on a master)

    Serial.println(F("Setup completed - Ready for JSON communication!"));
}


void loop() {
    message_repeater.loop();
}

