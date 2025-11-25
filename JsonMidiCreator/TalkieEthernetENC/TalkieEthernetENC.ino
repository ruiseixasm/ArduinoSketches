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

// Needed for the SPI module connection
#include <SPI.h>

// ONLY THE CHANGED LIBRARY ALLOWS THE RECEPTION OF BROADCASTED UDP PACKAGES TO 255.255.255.255
#include "src/sockets/BroadcastSocket_Changed_EthernetENC.hpp"
#include "src/JsonTalker.h"
#include "src/MultiPlayer.hpp"

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
auto& broadcast_socket = BroadcastSocket_EthernetENC::instance(talkers, sizeof(talkers)/sizeof(JsonTalker*));



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

// // IN DEVELOPMENT


// JsonTalkie json_talkie;
// SinglePlayer single_player(&broadcast_socket);
// MultiplePlayer player_1(&broadcast_socket);


// Network settings
#define PORT 5005   // UDP port


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
    const int CS_PIN = 5;  // Defines CS pin here (Enc28j60)
    
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
    JsonTalker::setTalkers(talkers, sizeof(talkers)/sizeof(JsonTalker*));

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

    // json_talkie.listen();
    // single_player.listen();
    // player_1.listen();

    // static unsigned long lastSend = 0;
    // if (millis() - lastSend > 39000) {

    //     // AVOID GLOBAL JSONDOCUMENT VARIABLES, HIGH RISK OF MEMORY LEAKS
    //     #if ARDUINOJSON_VERSION_MAJOR >= 7
    //     JsonDocument message_doc;
    //     if (message_doc.overflowed()) {
    //         Serial.println("CRITICAL: Insufficient RAM");
    //     } else {
    //         JsonObject message = message_doc.to<JsonObject>();
    //         message["m"] = 0;   // talk
    //         json_talkie.talk(message);
    //     }
    //     #else
    //     StaticJsonDocument<BROADCAST_SOCKET_BUFFER_SIZE> message_doc;
    //     if (message_doc.capacity() < BROADCAST_SOCKET_BUFFER_SIZE) {  // Absolute minimum
    //         Serial.println("CRITICAL: Insufficient RAM");
    //     } else {
    //         JsonObject message = message_doc.to<JsonObject>();
    //         message["m"] = 0;   // talk
    //         json_talkie.talk(message);
    //     }
    //     #endif
        
    //     lastSend = millis();
    // }
}


// long total_runs = 0;
// long _duration = 5;  // Example variable

// // Command implementations
// bool buzz(JsonObject json_message) {
//     (void)json_message; // Silence unused parameter warning
//     #ifndef BROADCASTSOCKET_SERIAL
//     digitalWrite(buzzer_pin, HIGH);
//     delay(_duration); 
//     digitalWrite(buzzer_pin, LOW);
//     #endif
//     total_runs++;
//     return true;
// }


// bool is_led_on = false;  // keep track of state yourself, by default it's off

// bool led_on(JsonObject json_message) {
//     if (!is_led_on) {
//         digitalWrite(LED_BUILTIN, HIGH);
//         is_led_on = true;
//         total_runs++;
//     } else {
//         json_message["r"] = "Already On!";
//         json_talkie.talk(json_message);
//         return false;
//     }
//     return true;
// }

// bool led_off(JsonObject json_message) {
//     if (is_led_on) {
//         digitalWrite(LED_BUILTIN, LOW);
//         is_led_on = false;
//         total_runs++;
//     } else {
//         json_message["r"] = "Already Off!";
//         json_talkie.talk(json_message);
//         return false;
//     }
//     return true;
// }


// bool set_duration(JsonObject json_message, long duration) {
//     (void)json_message; // Silence unused parameter warning
//     _duration = duration;
//     return true;
// }

// long get_duration(JsonObject json_message) {
//     (void)json_message; // Silence unused parameter warning
//     return _duration;
// }

// long get_total_runs(JsonObject json_message) {
//     (void)json_message; // Silence unused parameter warning
//     return total_runs;
// }


// bool process_response(JsonObject json_message) {
//     Serial.print(json_message["f"].as<String>());
//     Serial.print(" - ");
//     if (json_message["r"].is<String>()) {
//         Serial.println(json_message["r"].as<String>());
//     } else if (json_message["d"].is<String>()) {
//         Serial.println(json_message["d"].as<String>());
//     } else {
//         Serial.println("Empty echo received!");
//     }
//     return false;
// }

