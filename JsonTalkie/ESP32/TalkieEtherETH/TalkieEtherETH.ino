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

#define SOURCE_LIBRARY_MODE 1
//      0 - Arduino Library
//      1 - Project Library
//      2 - Arduino Copy Library


#if SOURCE_LIBRARY_MODE == 0
#include <JsonTalkie.hpp>
#include <sockets/BroadcastSocket_ETH.hpp>

#elif SOURCE_LIBRARY_MODE == 1
#include "src/JsonTalkie.hpp"
#include "src/sockets/BroadcastSocket_ETH.hpp"

#elif SOURCE_LIBRARY_MODE == 2
#include <Copy_JsonTalkie.hpp>
#include <sockets/BroadcastSocket_ETH.hpp>
#endif

// ESP32 native Ethernet - no SPI needed for built-in Ethernet
#include <ETH.h>
#include <WiFiUdp.h>

auto& broadcast_socket = BroadcastSocket_ETH::instance();

WiFiUDP udp;
// No MAC needed - ETH.h handles it automatically

// IN DEVELOPMENT

JsonTalkie json_talkie;

// Network settings
#define PORT 5005   // UDP port

// LED_BUILTIN is already defined by ESP32 platform
// Typically GPIO2 for most ESP32 boards
#ifndef LED_BUILTIN
  #define LED_BUILTIN 2  // Fallback definition if not already defined
#endif

// MANIFESTO DEFINITION

// Define the commands (stored in RAM)
JsonTalkie::Device device = {
    "ESP32", "I do a 500ms buzz!"
};

bool buzz(JsonObject json_message);
bool led_on(JsonObject json_message);
bool led_off(JsonObject json_message);
JsonTalkie::Run runCommands[] = {
    {"buzz", "Triggers buzzing", buzz},
    {"on", "Turns led On", led_on},
    {"off", "Turns led Off", led_off}
};

bool set_duration(JsonObject json_message, long duration);
JsonTalkie::Set setCommands[] = {
    {"duration", "Sets duration", set_duration}
};

long get_total_runs(JsonObject json_message);
long get_duration(JsonObject json_message);
JsonTalkie::Get getCommands[] = {
    {"total_runs", "Gets the total number of runs", get_total_runs},
    {"duration", "Gets duration", get_duration}
};

bool process_response(JsonObject json_message);

// MANIFESTO DECLARATION

JsonTalkie::Manifesto manifesto(
    &device,
    runCommands, sizeof(runCommands)/sizeof(JsonTalkie::Run),
    setCommands, sizeof(setCommands)/sizeof(JsonTalkie::Set),
    getCommands, sizeof(getCommands)/sizeof(JsonTalkie::Get),
    process_response, nullptr
);

// END OF MANIFESTO

// Buzzer pin
#define buzzer_pin 3

// Ethernet event handler
void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_ETH_START:
            Serial.println("ETH Started");
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            Serial.println("ETH Connected");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            Serial.print("ETH MAC: ");
            Serial.print(ETH.macAddress());
            Serial.print(", IPv4: ");
            Serial.print(ETH.localIP());
            if (ETH.fullDuplex()) {
                Serial.print(", FULL_DUPLEX");
            }
            Serial.print(", ");
            Serial.print(ETH.linkSpeed());
            Serial.println("Mbps");
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            Serial.println("ETH Disconnected");
            break;
        case ARDUINO_EVENT_ETH_STOP:
            Serial.println("ETH Stopped");
            break;
        default:
            break;
    }
}

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
    Serial.println("\n\n=== ESP32 with Native Ethernet STARTING ===");

    // Add a small LED blink to confirm code is running
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    
    Serial.println("Pins initialized successfully");

    // Setup Ethernet event handler
    WiFi.onEvent(WiFiEvent);

    // STEP 1: Initialize ESP32 Native Ethernet
    Serial.println("Step 1: Starting Native Ethernet...");
    
    // ETH.begin() with default parameters
    // For specific boards, you might need additional parameters:
    // ETH.begin(phy_addr, power_pin, mdc_pin, mdio_pin, phy_type, clock_speed)
    
    if (ETH.begin()) {
        Serial.println("Ethernet initialization started successfully");
    } else {
        Serial.println("Ethernet initialization failed!");
        while (1) {
            // Blink LED rapidly to indicate error
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            delay(200);
        }
    }

    // STEP 2: Wait for Ethernet connection
    Serial.println("Step 2: Waiting for Ethernet connection...");
    unsigned long startTime = millis();
    const unsigned long timeout = 15000; // 15 second timeout
    
    while (!ETH.linkUp()) {
        if (millis() - startTime > timeout) {
            Serial.println("Ethernet connection timeout!");
            break;
        }
        delay(500);
        Serial.print(".");
    }
    
    if (ETH.linkUp()) {
        Serial.println("\nEthernet link is UP!");
    } else {
        Serial.println("\nEthernet link is DOWN - check cable!");
    }

    // STEP 3: Wait for IP address
    Serial.println("Step 3: Waiting for IP address...");
    startTime = millis();
    
    while (ETH.localIP() == INADDR_NONE) {
        if (millis() - startTime > timeout) {
            Serial.println("IP address timeout!");
            break;
        }
        delay(500);
        Serial.print(".");
    }
    
    if (ETH.localIP() != INADDR_NONE) {
        Serial.println("\nIP address obtained!");
    } else {
        Serial.println("\nFailed to get IP address!");
    }

    // STEP 4: Display network status
    Serial.println("Step 4: Network Status:");
    Serial.print("IP Address: ");
    Serial.println(ETH.localIP());
    Serial.print("Subnet Mask: ");
    Serial.println(ETH.subnetMask());
    Serial.print("Gateway: ");
    Serial.println(ETH.gatewayIP());
    Serial.print("DNS: ");
    Serial.println(ETH.dnsIP());
    Serial.print("Link Speed: ");
    Serial.print(ETH.linkSpeed());
    Serial.println(" Mbps");
    Serial.print("Full Duplex: ");
    Serial.println(ETH.fullDuplex() ? "Yes" : "No");

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

    Serial.println("Setting JsonTalkie...");
    json_talkie.set_manifesto(&manifesto);
    json_talkie.plug_socket(&broadcast_socket);

    Serial.println("Talker ready with Native Ethernet!");

    // Final startup indication
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("Setup completed - Ready for JSON communication!");
}

void loop() {
    // Check Ethernet status periodically
    static unsigned long lastStatusCheck = 0;
    if (millis() - lastStatusCheck > 10000) {
        if (!ETH.linkUp()) {
            Serial.println("Ethernet link lost!");
        }
        lastStatusCheck = millis();
    }
    
    json_talkie.listen();

    static unsigned long lastSend = 0;
    if (millis() - lastSend > 39000) {

        // AVOID GLOBAL JSONDOCUMENT VARIABLES, HIGH RISK OF MEMORY LEAKS
        #if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument message_doc;
        if (message_doc.overflowed()) {
            Serial.println("CRITICAL: Insufficient RAM");
        } else {
            JsonObject message = message_doc.to<JsonObject>();
            message["m"] = 0;   // talk
            json_talkie.talk(message);
        }
        #else
        StaticJsonDocument<BROADCAST_SOCKET_BUFFER_SIZE> message_doc;
        if (message_doc.capacity() < BROADCAST_SOCKET_BUFFER_SIZE) {  // Absolute minimum
            Serial.println("CRITICAL: Insufficient RAM");
        } else {
            JsonObject message = message_doc.to<JsonObject>();
            message["m"] = 0;   // talk
            json_talkie.talk(message);
        }
        #endif
        
        lastSend = millis();
    }
}

long total_runs = 0;
long _duration = 5;  // Example variable

// Command implementations
bool buzz(JsonObject json_message) {
    (void)json_message; // Silence unused parameter warning
    #ifndef BROADCASTSOCKET_SERIAL
    digitalWrite(buzzer_pin, HIGH);
    delay(_duration); 
    digitalWrite(buzzer_pin, LOW);
    #endif
    total_runs++;
    return true;
}

bool is_led_on = false;  // keep track of state yourself, by default it's off

bool led_on(JsonObject json_message) {
    (void)json_message; // Silence unused parameter warning
    if (!is_led_on) {
        digitalWrite(LED_BUILTIN, HIGH);
        is_led_on = true;
        total_runs++;
    } else {
        json_message["r"] = "Already On!";
        json_talkie.talk(json_message);
        return false;
    }
    return true;
}

bool led_off(JsonObject json_message) {
    (void)json_message; // Silence unused parameter warning
    if (is_led_on) {
        digitalWrite(LED_BUILTIN, LOW);
        is_led_on = false;
        total_runs++;
    } else {
        json_message["r"] = "Already Off!";
        json_talkie.talk(json_message);
        return false;
    }
    return true;
}

bool set_duration(JsonObject json_message, long duration) {
    (void)json_message; // Silence unused parameter warning
    _duration = duration;
    return true;
}

long get_duration(JsonObject json_message) {
    (void)json_message; // Silence unused parameter warning
    return _duration;
}

long get_total_runs(JsonObject json_message) {
    (void)json_message; // Silence unused parameter warning
    return total_runs;
}

bool process_response(JsonObject json_message) {
    Serial.print(json_message["f"].as<String>());
    Serial.print(" - ");
    if (json_message["r"].is<String>()) {
        Serial.println(json_message["r"].as<String>());
    } else if (json_message["d"].is<String>()) {
        Serial.println(json_message["d"].as<String>());
    } else {
        Serial.println("Empty echo received!");
    }
    return false;
}

