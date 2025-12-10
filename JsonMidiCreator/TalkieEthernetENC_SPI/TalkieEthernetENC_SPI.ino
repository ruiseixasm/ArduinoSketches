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

// ONLY THE CHANGED LIBRARY ALLOWS THE RECEPTION OF BROADCASTED UDP PACKAGES TO 255.255.255.255
#include "src/sockets/BroadcastSocket_Changed_EthernetENC.hpp"
#include "src/sockets/BroadcastSocket_SPI_ESP_Arduino_Master_HSPI.hpp"
#include "src/LocalTalker.hpp"
#include "src/RemoteTalker.hpp"


const char local_name[] = "local";
const char local_desc[] = "I'm a local talker";
LocalTalker local = LocalTalker(local_name, local_desc);
const char remote_name[] = "remote";
const char remote_desc[] = "I'm a remote";
RemoteTalker remote = RemoteTalker(remote_name, remote_desc);
JsonTalker* local_talkers[] = { &local };   // It's an array of pointers
JsonTalker* remote_talkers[] = { &remote };   // It's an array of pointers
// Singleton requires the & (to get a reference variable)
auto& ethernet_socket = BroadcastSocket_EthernetENC::instance(local_talkers, sizeof(local_talkers)/sizeof(JsonTalker*));
auto& spi_socket = BroadcastSocket_SPI_ESP_Arduino_Master_HSPI::instance(remote_talkers, sizeof(remote_talkers)/sizeof(JsonTalker*));
JsonTalker* talkers[] = { &local, &remote };   // It's an array of pointers



EthernetUDP udp;

// uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};   // DEFAULT

uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01};
// uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x02};
// uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x03};
// uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x04};

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
    
    Serial.println("Pins initialized successfully");

    // STEP 1: Initialize SPI only
    
    Serial.println("Step 1: Starting SPI...");
	int talkers_spi_pins[] = {4, 16};
    spi_socket.setup(talkers_spi_pins, sizeof(talkers_spi_pins)/sizeof(int));
    Serial.println("SPI started successfully");
    delay(1000);

    // STEP 2: Initialize Ethernet with CS pin
    const int CS_PIN = 5;  // Defines CS pin here (Enc28j60)
    Serial.println("Step 2: Initializing EthernetENC...");
    Ethernet.init(CS_PIN);
    Serial.println("Ethernet initialized successfully");
    delay(500);

    // STEP 3: Begin Ethernet connection with DHCP
    Serial.println("Step 3: Starting Ethernet connection with DHCP...");
    if (Ethernet.begin(mac) == 0) {
        Serial.println("Failed to configure Ethernet using DHCP");
        // Optional: Fallback to static IP
        // Ethernet.begin(mac, IPAddress(192, 168, 1, 100));
        // while (Ethernet.localIP() == INADDR_NONE) {
        //     delay(1000);
        // }
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
    ethernet_socket.set_port(PORT);
    ethernet_socket.set_udp(&udp);

    Serial.println("Talker ready with EthernetENC!");
    Serial.println("Connecting Talkers with each other");

    // Connect the talkers with each other (static variable)
    JsonTalker::connectTalkers(talkers, sizeof(talkers)/sizeof(JsonTalker*));

    // Final startup indication
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("Setup completed - Ready for JSON communication!");
}


void loop() {
    // Maintain DHCP lease (important for long-running applications)
    Ethernet.maintain();
    
    ethernet_socket.receive();
    spi_socket.receive();
}

