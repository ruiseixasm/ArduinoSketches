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
#include "src/manifestos/BlackManifesto.hpp"
#include "src/JsonTalker.h"
#include "src/MessageRepeater.hpp"


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
BlackManifesto black_manifesto;
JsonTalker nano = JsonTalker(nano_name, nano_desc, &black_manifesto);
const char uno_name[] = "uno";
const char uno_desc[] = "Arduino Uno";
JsonTalker uno = JsonTalker(uno_name, uno_desc);
JsonTalker* downlinked_talkers[] = { &nano, &uno };    // Only an array of pointers preserves polymorphism!!
// Singleton requires the & (to get a reference variable)
auto& uplinked_sockets = BroadcastSocket_EtherCard::instance();

MessageRepeater message_repeater(
		uplinked_sockets, sizeof(uplinked_sockets)/sizeof(BroadcastSocket*),
		downlinked_talkers, sizeof(downlinked_talkers)/sizeof(JsonTalker*)
	);



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
    message_repeater.loop();
}


