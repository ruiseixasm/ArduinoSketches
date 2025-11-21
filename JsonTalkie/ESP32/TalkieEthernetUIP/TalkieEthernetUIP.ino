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

#define SOURCE_LIBRARY_MODE 1

#if SOURCE_LIBRARY_MODE == 0
#include <JsonTalkie.hpp>
#include <sockets/BroadcastSocket_EthernetUIP.hpp>

#elif SOURCE_LIBRARY_MODE == 1
#include "src/JsonTalkie.hpp"
#include "src/sockets/BroadcastSocket_EthernetUIP.hpp"

#elif SOURCE_LIBRARY_MODE == 2
#include <Copy_JsonTalkie.hpp>
#include <sockets/BroadcastSocket_EthernetUIP.hpp>
#endif

#include <SPI.h>

auto& broadcast_socket = BroadcastSocket_EthernetUIP::instance();

UIPUDP udp;
uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

#define ETHERNET_BUFFER_SIZE 800
uint8_t uip_eth_buffer[ETHERNET_BUFFER_SIZE];

JsonTalkie json_talkie;

#define PORT 5005

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

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

JsonTalkie::Manifesto manifesto(
    &device,
    runCommands, sizeof(runCommands)/sizeof(JsonTalkie::Run),
    setCommands, sizeof(setCommands)/sizeof(JsonTalkie::Set),
    getCommands, sizeof(getCommands)/sizeof(JsonTalkie::Get),
    process_response, nullptr
);

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
    Serial.println("\n\n=== ESP32 with UIPEthernet STARTING ===");

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

    // STEP 2: Initialize UIPEthernet
    Serial.println("Step 2: Initializing UIPEthernet...");
    
    // UIPEthernet requires init() first, then begin()
    Ethernet.init(CS_PIN);
    
    // Try DHCP with retries
    bool dhcpSuccess = false;
    for (int i = 0; i < 3; i++) {
        Serial.print("DHCP attempt ");
        Serial.println(i + 1);
        
        if (Ethernet.begin(mac) == 1) {
            dhcpSuccess = true;
            Serial.println("DHCP successful!");
            break;
        }
        delay(3000); // Wait longer between attempts
    }
    
    // If DHCP fails, use static IP
    if (!dhcpSuccess) {
        Serial.println("DHCP failed, using static IP...");
        IPAddress ip(192, 168, 31, 208);  // Use your network range
        IPAddress dns(8, 8, 8, 8);
        IPAddress gateway(192, 168, 31, 77);
        IPAddress subnet(255, 255, 255, 0);
        Ethernet.begin(mac, ip, dns, gateway, subnet);
    }
    
    delay(2000);

    // STEP 3: Check connection status
    Serial.println("Step 3: Checking network status...");
    Serial.print("Local IP: ");
    Serial.println(Ethernet.localIP());
    Serial.print("Subnet Mask: ");
    Serial.println(Ethernet.subnetMask());
    Serial.print("Gateway IP: ");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("DNS Server: ");
    Serial.println(Ethernet.dnsServerIP());

    // Validate IP address
    IPAddress currentIP = Ethernet.localIP();
    if (currentIP == IPAddress(0,0,0,0) || currentIP == IPAddress(5,0,0,0)) {
        Serial.println("❌ Invalid IP address - network initialization failed!");
        // Blink LED rapidly to indicate error
        while(1) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
            delay(100);
        }
    }

    // STEP 4: Initialize UDP and broadcast socket
    Serial.println("Step 4: Initializing UDP...");
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

    Serial.println("✅ Talker ready with UIPEthernet!");

    // Final startup indication
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("Setup completed - Ready for JSON communication!");
}


void loop() {
    Ethernet.maintain();
    
    json_talkie.listen();

    static unsigned long lastSend = 0;
    if (millis() - lastSend > 39000) {
        #if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument message_doc;
        if (!message_doc.overflowed()) {
            JsonObject message = message_doc.to<JsonObject>();
            message["m"] = 0;
            json_talkie.talk(message);
        }
        #else
        StaticJsonDocument<BROADCAST_SOCKET_BUFFER_SIZE> message_doc;
        if (message_doc.capacity() >= BROADCAST_SOCKET_BUFFER_SIZE) {
            JsonObject message = message_doc.to<JsonObject>();
            message["m"] = 0;
            json_talkie.talk(message);
        }
        #endif
        
        lastSend = millis();
    }
}

long total_runs = 0;
long _duration = 5;

bool buzz(JsonObject json_message) {
    (void)json_message;
    #ifndef BROADCASTSOCKET_SERIAL
    digitalWrite(buzzer_pin, HIGH);
    delay(_duration); 
    digitalWrite(buzzer_pin, LOW);
    #endif
    total_runs++;
    return true;
}

bool is_led_on = false;

bool led_on(JsonObject json_message) {
    (void)json_message;
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
    (void)json_message;
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
    (void)json_message;
    _duration = duration;
    return true;
}

long get_duration(JsonObject json_message) {
    (void)json_message;
    return _duration;
}

long get_total_runs(JsonObject json_message) {
    (void)json_message;
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

