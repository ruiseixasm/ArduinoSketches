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
//      0 - Arduino Library
//      1 - Project Library
//      2 - Arduino Copy Library


#if SOURCE_LIBRARY_MODE == 0
#include <JsonTalkie.hpp>
#include <sockets/BroadcastSocket_Ethernet.hpp>

#elif SOURCE_LIBRARY_MODE == 1
#include "src/JsonTalkie.hpp"
#include "src/sockets/BroadcastSocket_Ethernet.hpp"

#elif SOURCE_LIBRARY_MODE == 2
#include <Copy_JsonTalkie.hpp>
#include <sockets/BroadcastSocket_Ethernet.hpp>
#endif


// To upload a sketch to an ESP32, when the "......." appears press the button BOOT for a while


// The liberally bellow uses the libraries:
// #include <Ethernet.h>
// #include <EthernetUdp.h>
auto& broadcast_socket = BroadcastSocket_Ethernet::instance();

EthernetUDP udp;
uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

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

void setup() {
    // Serial is a singleton class (can be began multiple times)
    Serial.begin(115200);
    while (!Serial);
    
    delay(2000);    // Just to give some time to Serial

    // Saving string in PROGMEM (flash) to save RAM memory
    Serial.println("\n\nOpening the Socket...");
    

    // INITIATING THE ETHERNET CONNECTION

    Ethernet.begin(mac);
    
    // Check if ENC28J60 is connected
    Serial.print("Checking ENC28J60 connection...");
    delay(1000); // Give time for initialization
    
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        Serial.println(" FAILED - ENC28J60 not found!");
        while (true) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
            delay(100);
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
            delay(700); // Blink pattern to indicate hardware error
        }
    } else if (Ethernet.hardwareStatus() == EthernetW5100) {
        Serial.println(" WARNING - Found W5100 instead of ENC28J60");
    } else if (Ethernet.hardwareStatus() == EthernetW5500) {
        Serial.println(" WARNING - Found W5500 instead of ENC28J60");
    } else {
        Serial.println(" SUCCESS - ENC28J60 detected");
    }
    
    // Check link status
    Serial.print("Checking link status...");
    if (Ethernet.linkStatus() == LinkOFF) {
        Serial.println(" NO LINK - Check Ethernet cable!");
    } else {
        Serial.println(" LINK OK");
    }
        
    Serial.print("IP: ");
    Serial.println(Ethernet.localIP());
    
    // Calculate broadcast address manually
    IPAddress localIP = Ethernet.localIP();
    IPAddress subnetMask = Ethernet.subnetMask();
    IPAddress broadcastIP;
    
    for (int i = 0; i < 4; i++) {
        broadcastIP[i] = localIP[i] | (~subnetMask[i] & 0xFF);
    }
    
    Serial.print("Broadcast: ");
    Serial.println(broadcastIP);

    // ETHERNET CONNECTION MADE
    
    

    udp.begin(PORT);

    // Saving string in PROGMEM (flash) to save RAM memory
    Serial.println("\n\nOpening the Socket...");
    broadcast_socket.set_port(5005);    // By default is already 5005
    broadcast_socket.set_udp(&udp);

    Serial.println("Setting JsonTalkie...");
    json_talkie.set_manifesto(&manifesto);
    json_talkie.plug_socket(&broadcast_socket);


    Serial.println("Talker ready");

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("Sending JSON...");
}

void loop() {
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
