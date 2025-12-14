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
#include "src/sockets/BroadcastSocket_SPI_ESP_Arduino_Master.hpp"
#include "src/RepeaterTalker.hpp"


const char t_ethernet_name[] = "t_ethernet";
const char t_ethernet_desc[] = "I'm an Ethernet talker";
RepeaterTalker t_ethernet = RepeaterTalker(t_ethernet_name, t_ethernet_desc);
const char t_spi_name[] = "t_spi";
const char t_spi_desc[] = "I'm a SPI talker";
RepeaterTalker t_spi = RepeaterTalker(t_spi_name, t_spi_desc);
JsonTalker* t_ethernet_talkers[] = { &t_ethernet };   // It's an array of pointers
JsonTalker* t_spi_talkers[] = { &t_spi };   // It's an array of pointers
// Singleton requires the & (to get a reference variable)
auto& ethernet_socket = BroadcastSocket_EthernetENC::instance(t_ethernet_talkers, sizeof(t_ethernet_talkers)/sizeof(JsonTalker*));
int spi_pins[] = {4, 16};
// int spi_pins[] = {4};
auto& spi_socket = BroadcastSocket_SPI_ESP_Arduino_Master::instance(
	t_spi_talkers, sizeof(t_spi_talkers)/sizeof(JsonTalker*), spi_pins, sizeof(spi_pins)/sizeof(int)
);
JsonTalker* talkers[] = { &t_ethernet, &t_spi };   // It's an array of pointers



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
    
    // STEP 1: Initialize Ethernet with CS pin
    const int CS_PIN = 5;  // Defines CS pin here (Enc28j60)
    Serial.println("Step 1: Initializing EthernetENC...");
	// This forces the Ethernet to use the default SPI
    Ethernet.init(CS_PIN);	// Uses global SPI (VSPI)
	// // As alternative it is possible to give a specific SPI
	// SPIClass* ethSPI = new SPIClass(HSPI);
	// ethSPI->begin(14, 12, 13, 15);  // SCK, MISO, MOSI, SS (dummy)
	// EthernetENC(uint8_t csPin, SPIClass* ethSPI)
    Serial.println("Ethernet initialized successfully");
    delay(500);

    // STEP 2: Begin Ethernet connection with DHCP
    Serial.println("Step 2: Starting Ethernet connection with DHCP...");
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

    // STEP 3: Check connection status
    Serial.println("Step 3: Checking Ethernet status...");
    Serial.print("Local IP: ");
    Serial.println(Ethernet.localIP());
    Serial.print("Subnet Mask: ");
    Serial.println(Ethernet.subnetMask());
    Serial.print("Gateway IP: ");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("DNS Server: ");
    Serial.println(Ethernet.dnsServerIP());

    // STEP 4: Initialize UDP and broadcast socket
    Serial.println("Step 4: Initializing UDP...");
    if (udp.begin(PORT)) {
        Serial.println("UDP started successfully on port " + String(PORT));
    } else {
        Serial.println("Failed to start UDP!");
    }

    // STEP 5: Setting up broadcast sockets
    Serial.println("Step 5: Setting up broadcast sockets...");
	SPIClass* hspi = new SPIClass(HSPI);  // heap variable!
	// ================== INITIALIZE HSPI ==================
	// Initialize SPI with HSPI pins: SCK=14, MISO=12, MOSI=13, SS=15
	hspi->begin(14, 12, 13, 15);  // SCK, MISO, MOSI, SS
    spi_socket.begin(hspi);
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

