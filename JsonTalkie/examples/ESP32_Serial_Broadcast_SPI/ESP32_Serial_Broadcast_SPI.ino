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

// LED_BUILTIN is already defined by ESP32 platform
// Typically GPIO2 for most ESP32 boards
#ifndef LED_BUILTIN
  #define LED_BUILTIN 2  // Fallback definition if not already defined
#endif

#include <JsonTalkie.hpp>
// ONLY THE CHANGED LIBRARY ALLOWS THE RECEPTION OF BROADCASTED UDP PACKAGES TO 255.255.255.255
#include "S_SocketSerial.hpp"
#include "S_Broadcast_SPI_2xESP_Master.hpp"
#include "M_Spy.hpp"
#include "M_CyclerManifesto.hpp"


// TALKERS 

const char talker_name[] = "serial";
const char talker_desc[] = "I'm a serial talker";
M_SerialManifesto serial_manifesto;
JsonTalker talker = JsonTalker(talker_name, talker_desc, &serial_manifesto);

// M_Spy Talker
const char t_spy_name[] = "spy";
const char t_spy_desc[] = "I'm a M_Spy and I spy the talkers' pings";
M_Spy spy_manifesto;
JsonTalker t_spy = JsonTalker(t_spy_name, t_spy_desc, &spy_manifesto);

// Talker cycler
const char t_cycler_name[] = "cycler";
const char t_cycler_desc[] = "I cycle the blue led";
M_CyclerManifesto cycler_manifesto;
JsonTalker t_cycler = JsonTalker(t_cycler_name, t_cycler_desc, &cycler_manifesto);


// SOCKETS
// Singleton requires the & (to get a reference variable)
auto& serial_socket = S_SocketSerial::instance();
int spi_pins[] = {4, 16};
auto& spi_socket = S_Broadcast_SPI_2xESP_Master::instance(spi_pins, sizeof(spi_pins)/sizeof(int));


// SETTING THE REPEATER
BroadcastSocket* uplinked_sockets[] = { &serial_socket };
JsonTalker* downlinked_talkers[] = { &t_spy, &t_tester, &l_led };
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
    delay(2000); // Important: Give time for serial to initialize
    Serial.println("\n\n=== ESP32 with Broadcast SPI STARTING ===");

    // Add a small LED blink to confirm code is running
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    
    // Setting up broadcast sockets
    Serial.println("Setting up broadcast sockets...");
	SPIClass* hspi = new SPIClass(HSPI);  // heap variable!
	// ================== INITIALIZE HSPI ==================
	// Initialize SPI with HSPI pins: SCK=14, MISO=12, MOSI=13, SS=15
	hspi->begin(14, 12, 13, 15);  // SCK, MISO, MOSI, SS
    spi_socket.begin(hspi);

    // Final startup indication
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("Setup completed - Ready for JSON communication!");
}


void loop() {
	message_repeater.loop();	// Keep calling the Repeater
}

