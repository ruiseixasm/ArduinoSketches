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
#include "S_Broadcast_SPI_Arduino_Slave.h"


const char talker_name[] = "buzzer";
const char talker_desc[] = "I'm a buzzer that buzzes";
M_BuzzerManifesto talker_manifesto;
JsonTalker talker = JsonTalker(talker_name, talker_desc, &talker_manifesto);

// Singleton requires the & (to get a reference variable)
auto& spi_socket = S_Broadcast_SPI_Arduino_Slave::instance();

// SETTING THE REPEATER
BroadcastSocket* uplinked_sockets[] = { &spi_socket };
JsonTalker* downlinked_talkers[] = { &talker };
MessageRepeater message_repeater(
		uplinked_sockets, sizeof(uplinked_sockets)/sizeof(BroadcastSocket*),
		downlinked_talkers, sizeof(downlinked_talkers)/sizeof(JsonTalker*)
	);



void setup() {
    // Then start Serial
    Serial.begin(115200);
    delay(100);
    Serial.println(F("\n\n=== Arduino with SPI STARTING ==="));

    Serial.println(F("Step 1: Starting SPI..."));
	spi_socket.bridgeSocket();	// Makes sure it accepts LOCAL messages too
    Serial.println(F("SPI started successfully"));
    delay(5000);	// Waits 5 seconds for all other SPIs to start (it's a slave, so, it depends on a master)

    Serial.println(F("Setup completed - Ready for JSON communication!"));
}



void loop() {
    message_repeater.loop();
}

