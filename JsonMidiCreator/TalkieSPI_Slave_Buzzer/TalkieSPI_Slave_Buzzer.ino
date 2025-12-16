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
#include "src/sockets/BroadcastSocket_SPI_ESP_Arduino_Slave.h"


const char talker_name[] = "buzzer";
const char talker_desc[] = "I'm a buzzer that buzzes";
BuzzerManifesto talker_manifesto;
JsonTalker talker = JsonTalker(talker_name, talker_desc, &talker_manifesto);
JsonTalker* talkers[] = { &talker };   // It's an array of pointers of JsonTalker (keep it as JsonTalker!)
// Singleton requires the & (to get a reference variable)

auto& spi_socket = BroadcastSocket_SPI_ESP_Arduino_Slave::instance(talkers, sizeof(talkers)/sizeof(JsonTalker*));



void setup() {
    // Then start Serial
    Serial.begin(115200);
    Serial.println("\n\n=== Arduino with SPI STARTING ===");

    Serial.println("Step 1: Starting SPI...");
    Serial.println("SPI started successfully");
    delay(1000);

    // Connect the talkers with each other (static variable)
    Serial.println("Connecting Talkers with each other");
    JsonTalker::connectTalkers(talkers, sizeof(talkers)/sizeof(JsonTalker*));

    Serial.println("Setup completed - Ready for JSON communication!");
}



void loop() {
    spi_socket.loop();
}

