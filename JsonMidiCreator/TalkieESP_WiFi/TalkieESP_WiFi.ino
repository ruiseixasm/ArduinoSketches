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

// To upload a sketch to an ESP32, when the "......." appears press the button BOOT for a while

// LED_BUILTIN is already defined by ESP32 platform
// Typically GPIO2 for most ESP32 boards
#ifndef LED_BUILTIN
  #define LED_BUILTIN 2  // Fallback definition if not already defined
#endif

#include "src/sockets/BroadcastESP_WiFi.hpp"
#include "src/manifestos/Spy.h"
#include "src/manifestos/BlueManifesto.hpp"
#include "src/manifestos/MessageTester.hpp"
#include "src/MessageRepeater.hpp"

const char ssid[] = "wifiName";
const char password[] = "wifiPassword";


// TALKERS 
// Ethernet Socket Repeater
// Spy Talker (being sockless devoids it of answering direct remote calls, and that also works, but differently)
const char t_spy_name[] = "spy";
const char t_spy_desc[] = "I'm a Spy and I spy the talkers' pings";
Spy spy_manifesto;
JsonTalker t_spy = JsonTalker(t_spy_name, t_spy_desc, &spy_manifesto);

// Sockless Talker (blue led)
const char l_blue_name[] = "blue";
const char l_blue_desc[] = "I turn led Blue on and off";
BlueManifesto blue_manifesto(2);
JsonTalker l_blue = JsonTalker(l_blue_name, l_blue_desc, &blue_manifesto);

// Sockless Talker (JsonMessage tester)
const char t_tester_name[] = "test";
const char t_tester_desc[] = "I test the JsonMessage class";
MessageTester message_tester;
JsonTalker t_tester = JsonTalker(t_tester_name, t_tester_desc, &message_tester);


// SOCKETS

// Singleton requires the & (to get a reference variable)
auto& ethernet_socket = BroadcastESP_WiFi::instance();


// SETTING THE REPEATER
BroadcastSocket* uplinked_sockets[] = { &ethernet_socket };
JsonTalker* downlinked_talkers[] = { &t_spy, &t_tester, &l_blue };
MessageRepeater message_repeater(
		uplinked_sockets, sizeof(uplinked_sockets)/sizeof(BroadcastSocket*),
		downlinked_talkers, sizeof(downlinked_talkers)/sizeof(JsonTalker*)
	);


WiFiUDP udp;

// Network settings
#define PORT 5005   // UDP port


void setup() {
    // Initialize pins FIRST before anything else
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); // Start with LED off
    
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
    
    // STEP 1: Initialize WiFi
    Serial.println("Step 1: Initializing WiFi...");
    WiFi.begin(ssid, password);
    Serial.println("WiFi initialized successfully");
    delay(500);

    // Give Ethernet time to stabilize
    delay(1500);

    // STEP 2: Check connection status
    Serial.println("Step 3: Checking Ethernet status...");
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Subnet Mask: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("Gateway IP: ");
    Serial.println(WiFi.gatewayIP());

    // STEP 3: Initialize UDP and broadcast socket
    Serial.println("Step 4: Initializing UDP...");
    if (udp.begin(PORT)) {
        Serial.println("UDP started successfully on port " + String(PORT));
    } else {
        Serial.println("Failed to start UDP!");
    }

    // STEP 5: Setting up broadcast sockets
    Serial.println("Step 5: Setting up broadcast sockets...");
    ethernet_socket.set_port(PORT);
    ethernet_socket.set_udp(&udp);

    // Final startup indication
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("Setup completed - Ready for JSON communication!");
}


void loop() {
	message_repeater.loop();
}

