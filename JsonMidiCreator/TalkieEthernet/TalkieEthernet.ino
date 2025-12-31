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

// Needed for the SPI module connection
#include <SPI.h>


// ONLY THE CHANGED LIBRARY ALLOWS THE RECEPTION OF BROADCASTED UDP PACKAGES TO 255.255.255.255
#include "src/sockets/BroadcastSocket_Ethernet.hpp"
#include "src/manifestos/MegaManifesto.hpp"
#include "src/JsonTalker.h"
#include "src/MessageRepeater.hpp"

const char mega_name[] = "mega";
const char mega_desc[] = "I'm a Mega talker";
JsonTalker mega = JsonTalker(mega_name, mega_desc);

JsonTalker* downlinked_talkers[] = { &mega };    // Only an array of pointers preserves polymorphism!!
// Singleton requires the & (to get a reference variable)
auto& ethernet_socket = BroadcastSocket_EtherCard::instance();
BroadcastSocket* uplinked_sockets[] = { &ethernet_socket };	// list of pointers

MessageRepeater message_repeater(
		uplinked_sockets, sizeof(uplinked_sockets)/sizeof(BroadcastSocket*),
		downlinked_talkers, sizeof(downlinked_talkers)/sizeof(JsonTalker*)
	);


EthernetUDP udp;

// uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};   // DEFAULT

// uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01};
// uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x02};
// uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x03};
uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x04};

// uint8_t mac[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01};
// uint8_t mac[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x02};
// uint8_t mac[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x03};
// uint8_t mac[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x04};
// uint8_t mac[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x05};

// uint8_t mac[] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x01};
// uint8_t mac[] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x02};
// uint8_t mac[] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x03};
// uint8_t mac[] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x04};
// uint8_t mac[] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x05};

// uint8_t mac[] = {0x02, 0xAA, 0xFA, 0xCE, 0x10, 0x01};
// uint8_t mac[] = {0x02, 0xAA, 0xFA, 0xCE, 0x10, 0x02};
// uint8_t mac[] = {0x02, 0xAA, 0xFA, 0xCE, 0x10, 0x03};
// uint8_t mac[] = {0x02, 0xAA, 0xFA, 0xCE, 0x10, 0x04};
// uint8_t mac[] = {0x02, 0xAA, 0xFA, 0xCE, 0x10, 0x05};

// // IN DEVELOPMENT


// Network settings
#define PORT 5005   // UDP port

// Arduino communicates with both the W5100 and SD card using the SPI bus (through the ICSP header).
// This is on digital pins 10, 11, 12, and 13 on the Uno and pins 50, 51, and 52 on the Mega. On both boards,
// pin 10 is used to select the W5100 and pin 4 for the SD card. These pins cannot be used for general I/O.
// On the Mega, the hardware SS pin, 53, is not used to select either the W5100 or the SD card,
// but it must be kept as an output or the SPI interface won't work.


void setup() {
    // Initialize pins FIRST before anything else
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); // Start with LED off
    
    // Then start Serial
    Serial.begin(115200);
    delay(2000); // Important: Give time for serial to initialize
    Serial.println("\n=== ARDUINO MEGA W5100 STARTING ===");

    // Add a small LED blink to confirm code is running
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);

    // CRITICAL FOR MEGA: Set pin 53 as OUTPUT
    pinMode(53, OUTPUT);
    digitalWrite(53, HIGH);  // Not used as CS, but must be output
    
    Serial.println("Pins initialized successfully");

    // STEP 1: Initialize SPI only
    const int CS_PIN = 10;  // Defines CS pin here (W5500/W5100)
    
    Serial.println("Step 1: Starting SPI...");
    SPI.begin();
    Serial.println("SPI started successfully");
    delay(1000);

    // STEP 2: Initialize Ethernet with CS pin
    Serial.println("Step 2: Initializing EthernetENC...");
    Ethernet.init(CS_PIN);
    Serial.println("Ethernet initialized successfully");
    delay(500);

    // STEP 3: Begin Ethernet connection with DHCP
    Serial.println("Step 3: Starting Ethernet connection with DHCP...");
    if (Ethernet.begin(mac) == 0) {
        Serial.println("Failed to configure Ethernet using DHCP");
        // Optional: Fallback to static IP
        Ethernet.begin(mac, IPAddress(192, 168, 1, 100));
        while (Ethernet.localIP() == INADDR_NONE) {
            delay(1000);
        }
    } else {
        Serial.println("DHCP successful!");
    }

    // Give Ethernet time to stabilize
    delay(1500);

    // STEP 4: Check connection status
    Serial.println("Step 4: Checking Ethernet status...");
    Serial.print("Local IP: ");
    Serial.println(Ethernet.localIP());
    Serial.print("Subnet Mask: ");
    Serial.println(Ethernet.subnetMask());
    Serial.print("Gateway IP: ");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("DNS Server: ");
    Serial.println(Ethernet.dnsServerIP());

    // STEP 5: Initialize UDP and broadcast socket
    Serial.println("Step 5: Initializing UDP...");
    if (udp.begin(PORT)) {
        Serial.println("UDP started successfully on port " + String(PORT));
    } else {
        Serial.println("Failed to start UDP!");
    }

    Serial.println("Setting up broadcast socket...");
    broadcast_socket.set_port(PORT);
    broadcast_socket.set_udp(&udp);

    Serial.println("Talker ready with EthernetENC!");
    Serial.println("Connecting Talkers with each other");

    // Final startup indication
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("Setup completed - Ready for JSON communication!");
}


void loop() {
    Ethernet.maintain();	// Maintain DHCP lease (important for long-running applications)
    broadcast_socket.loop();
}



