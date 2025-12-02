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



// LED_BUILTIN is already defined by ESP32 platform
// Typically GPIO2 for most ESP32 boards
#ifndef LED_BUILTIN
  #define LED_BUILTIN 2  // Fallback definition if not already defined
#endif

// ONLY THE CHANGED LIBRARY ALLOWS THE RECEPTION OF BROADCASTED UDP PACKAGES TO 255.255.255.255
#include "src/sockets/BroadcastSocket_SPI_Master.hpp"
#include "src/JsonTalker.h"
#include "src/MultiPlayer.hpp"

const char talker_name[] = "talker";
const char talker_desc[] = "I'm a talker";
JsonTalker talker = JsonTalker(talker_name, talker_desc);
const char player_name[] = "player";
const char player_desc[] = "I'm a player";
MultiPlayer player = MultiPlayer(player_name, player_desc);
JsonTalker* talkers[] = { &talker, player.mute() };   // It's an array of pointers
// Singleton requires the & (to get a reference variable)
auto& broadcast_socket = BroadcastSocket_SPI_Master::instance(talkers, sizeof(talkers)/sizeof(JsonTalker*));


// JsonDocument in the stack makes sure its memory is released (NOT GLOBAL)
#if ARDUINOJSON_VERSION_MAJOR >= 7
JsonDocument ss_pins_doc;
JsonObject talkers_ss_pins = ss_pins_doc.to<JsonObject>(); // NEW: Persistent object for device control
#else
StaticJsonDocument<BROADCAST_SOCKET_BUFFER_SIZE> ss_pins_doc;
JsonObject talkers_ss_pins = ss_pins_doc.to<JsonObject>();    // NEW
#endif


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
    talkers_ss_pins["a"] = 4;
    talkers_ss_pins["b"] = 16;
    Serial.println("Step 1: Starting SPI...");
    broadcast_socket.setup(&talkers_ss_pins);
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

