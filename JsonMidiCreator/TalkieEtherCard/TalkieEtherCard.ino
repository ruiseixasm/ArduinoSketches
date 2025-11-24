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
// #include <avr/pgmspace.h>


// ONLY THE CHANGED LIBRARY ALLOWS THE RECEPTION OF BROADCASTED UDP PACKAGES TO 255.255.255.255
#include "src/sockets/BroadcastSocket_EtherCard.hpp"
#include "src/JsonTalker.h"


// Adjust the Ethercard buffer size to the absolutely minimum needed
// for the DHCP so that it works, but too much and the Json messages
// become corrupted due to lack of memory in the Uno and Nano.
#define ETHERNET_BUFFER_SIZE 340    // 256 or 300 and the DHCP won't work!
byte Ethernet::buffer[ETHERNET_BUFFER_SIZE];  // Ethernet buffer


// Network settings

// uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};   // DEFAULT

// uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01};
uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x02};
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

#define PORT 5005                                       // UDP port



const char nano_name[] = "nano";
const char nano_desc[] = "Arduino Nano";
JsonTalker nano = JsonTalker(nano_name, nano_desc);
const char uno_name[] = "uno";
const char uno_desc[] = "Arduino Uno";
JsonTalker uno = JsonTalker(uno_name, uno_desc);
JsonTalker talkers[] = { nano, uno };
// Singleton requires the & (to get a reference variable)
auto& broadcast_socket = BroadcastSocket_EtherCard::instance(talkers, sizeof(talkers)/sizeof(JsonTalker));



// Buzzer pin
#define buzzer_pin 3

void setup() {
    // Serial is a singleton class (can be began multiple times)
    Serial.begin(9600);
    while (!Serial);
    
    delay(2000);    // Just to give some time to Serial

    // Saving string in PROGMEM (flash) to save RAM memory
    Serial.println(F("\n\nOpening the Socket..."));
    
    // MAC and CS pin in constructor
    // SS is a macro variable normally equal to 10
    if (!ether.begin(ETHERNET_BUFFER_SIZE, mac, SS)) {
        Serial.println(F("Failed to access ENC28J60"));
        while (1);
    }
    // Set dynamic IP (via DHCP)
    if (!ether.dhcpSetup()) {
        Serial.println(F("DHCP failed"));
        while (1);
    }
    // Makes sure it allows broadcast
    ether.enableBroadcast();


    // By default is already 5005
    broadcast_socket.set_port(5005);


    Serial.println(F("Socket ready"));

    #ifndef BROADCAST_SOCKET_SERIAL_HPP
    pinMode(buzzer_pin, OUTPUT);
    digitalWrite(buzzer_pin, HIGH);
    delay(10); 
    digitalWrite(buzzer_pin, LOW);
    #endif
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println(F("Sending JSON..."));
}

void loop() {
    broadcast_socket.receive();


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
        Serial.println(F("Empty echo received!"));
    }
    return false;
}

