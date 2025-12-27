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



// COMPILE WITH ARDUINO BOARD
#include "src/manifestos/BuzzerManifesto.hpp"
#include "src/JsonTalker.h"
#include "src/sockets/SPI_ESP_Arduino_Slave.h"
#include "src/MessageRepeater.hpp"


const char talker_name[] = "buzzer";
const char talker_desc[] = "I'm a buzzer that buzzes";
BuzzerManifesto talker_manifesto;
JsonTalker talker = JsonTalker(talker_name, talker_desc, &talker_manifesto);

// Singleton requires the & (to get a reference variable)
auto& spi_socket = SPI_ESP_Arduino_Slave::instance();

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
    Serial.println("\n\n=== Arduino with SPI STARTING ===");

    Serial.println("Step 1: Starting SPI...");
    Serial.println("SPI started successfully");
    delay(1000);

    Serial.println("Setup completed - Ready for JSON communication!");
}



void loop() {
    message_repeater.loop();
}

