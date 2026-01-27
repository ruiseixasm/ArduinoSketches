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


#include <JsonTalkie.hpp>
#include "M_BuzzerManifesto.hpp"
#include "S_SocketSerial.hpp"
#include "S_Basic_SPI_2xArduino_Master_Single.hpp"


const char talker_name[] = "single";
const char talker_desc[] = "I'm a single talker";
M_BuzzerManifesto buzzer_manifesto;
JsonTalker talker = JsonTalker(talker_name, talker_desc, &buzzer_manifesto);

// Singleton requires the & (to get a reference variable)
auto& serial_socket = S_SocketSerial::instance();
auto& spi_socket = S_Basic_SPI_2xArduino_Master_Single::instance(SS);

// SETTING THE REPEATER
BroadcastSocket* uplinked_sockets[] = { &serial_socket };
JsonTalker* downlinked_talkers[] = { &talker };
BroadcastSocket* downlinked_sockets[] = { &spi_socket };
const MessageRepeater message_repeater(
		uplinked_sockets, sizeof(uplinked_sockets)/sizeof(BroadcastSocket*),
		downlinked_talkers, sizeof(downlinked_talkers)/sizeof(JsonTalker*),
		downlinked_sockets, sizeof(downlinked_sockets)/sizeof(BroadcastSocket*)
	);


void setup() {
    // Initialize pins FIRST before anything else
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); // Start with LED off

    // Then start Serial
    Serial.begin(115200);
    delay(250); // Important: Give time for single to initialize
    Serial.println("\n\n=== Arduino with SERIAL ===");

    // Add a small LED blink to confirm code is running
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    

    // Final startup indication
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("Setup completed - Ready for JSON communication!");
}


void loop() {
    message_repeater.loop();
}

