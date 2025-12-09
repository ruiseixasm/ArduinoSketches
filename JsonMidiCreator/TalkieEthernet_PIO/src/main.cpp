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


// Needed for the SPI module connection
#include <SPI.h>

// ESP32 wiring with the ENC28J60 (SPI)

//     MOSI (D23)  =   SI
//     MISO (D19)  =   SO
//     SCK  (D18)  =   SCK
//     SS   (D5)   =   CS
//     GND         =   GND
//     MUTUAL EXCLUSIVE:
//         VIN     =   5V
//         3V3     =   Q3


// ESP32 (3.3V)        →   Nano (5V)          Risk Level
// ─────────────────────────────────────────────────────
// ESP32 MOSI (D13)    →   Nano MOSI (D11)    SAFE (Nano input)
// ESP32 MISO (D12)    ←   Nano MISO (D12)    DANGEROUS! (Nano output=5V)
// ESP32 SCK  (D14)    →   Nano SCK  (D13)    SAFE (Nano input) (Also DANGEROUS due to LED_BUILTIN on pin 13)
// ESP32 SS   (D15)    →   Nano SS   (D10)    SAFE (Nano input)

// Use level shifter or resistors
//     ESP32 → Nano: Direct (3.3V → 5V input is fine)
//     Nano → ESP32: 1K series resistor (limits current to safe level)

// Direct connection often works:
//     ESP32 MOSI (3.3V) → Nano D11 (5V input)  // SAFE
//     ESP32 SCK  (3.3V) → Nano D13 (5V input)  // RISKY due to LED_BUILTIN on pin 13 
//     ESP32 SS   (3.3V) → Nano D10 (5V input)  // SAFE
//     Nano MISO  (5V)   → ESP32 MISO (3.3V)    // RISKY but usually survives

// Key Parameters:
//     Nano output: 5V, can source ~20mA max
//     ESP32 input: Has ESD protection diodes to 3.3V rail
//     Diode forward voltage: ~0.6V each

// Calculations:
//     V_resistor = V_nano - V_esp32 = 5V - 3.9V = 1.1V
//     I = V_resistor / R = 1.1V / 1000Ω = 1.1mA



// #define SOURCE_LIBRARY_MODE 1
// //      0 - Arduino Library
// //      1 - Project Library
// //      2 - Arduino Copy Library


// #if SOURCE_LIBRARY_MODE == 0
// #include <JsonTalkie.hpp>

// #elif SOURCE_LIBRARY_MODE == 1
// #include "src/JsonTalkie.hpp"

// #elif SOURCE_LIBRARY_MODE == 2
// #include <Copy_JsonTalkie.hpp>
// #endif

// LED_BUILTIN is already defined by ESP32 platform
// Typically GPIO2 for most ESP32 boards
#ifndef LED_BUILTIN
  #define LED_BUILTIN 2  // Fallback definition if not already defined
#endif

// ONLY THE CHANGED LIBRARY ALLOWS THE RECEPTION OF BROADCASTED UDP PACKAGES TO 255.255.255.255
#include "sockets/BroadcastSocket_Ethernet.hpp"
#include "JsonTalker.h"
#include "MultiPlayer.hpp"

// #include "SinglePlayer.hpp"
// #include "MultiplePlayer.hpp"

const char talker_name[] = "talker";
const char talker_desc[] = "I'm a talker";
JsonTalker talker = JsonTalker(talker_name, talker_desc);
const char player_name[] = "player";
const char player_desc[] = "I'm a player";
MultiPlayer player = MultiPlayer(player_name, player_desc);
JsonTalker* talkers[] = { &talker, &player };   // It's an array of pointers
// Singleton requires the & (to get a reference variable)
auto& broadcast_socket = BroadcastSocket_Ethernet::instance(talkers, sizeof(talkers)/sizeof(JsonTalker*));



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


// JsonTalkie json_talkie;
// SinglePlayer single_player(&broadcast_socket);
// MultiplePlayer player_1(&broadcast_socket);


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
    delay(4000); // Important: Give time for serial to initialize (GIVES TIME FOR UPLOAD)
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
    // const int CS_PIN = 5;  // Defines CS pin here (Enc28j60)
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
        // Ethernet.begin(mac, IPAddress(192, 168, 1, 100));
        // while (Ethernet.localIP() == INADDR_NONE) {
        //     delay(1000);
        // }
    } else {
        Serial.println("DHCP successful!");
    }

    // // CRITICAL: Enable broadcast reception
    // Ethernet.setBroadcast(true);
    // Serial.println("Broadcast reception enabled");

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

    // Hardware status check (EthernetENC may not have hardwareStatus())
    // if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    //     Serial.println("WARNING: Ethernet hardware not detected!");
    // } else {
    //     Serial.println("Ethernet hardware detected");
    // }

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

    // Serial.println("Setting JsonTalkie...");
    // json_talkie.set_manifesto(&manifesto);
    // json_talkie.plug_socket(&broadcast_socket);

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
    
    broadcast_socket.receive();

}



