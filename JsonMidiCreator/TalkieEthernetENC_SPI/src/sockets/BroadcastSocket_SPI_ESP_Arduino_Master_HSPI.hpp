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
#ifndef BROADCAST_SOCKET_SPI_ESP_ARDUINO_MASTER_HSPI_HPP
#define BROADCAST_SOCKET_SPI_ESP_ARDUINO_MASTER_HSPI_HPP

#include "BroadcastSocket_SPI_ESP_Arduino_Master_VSPI.hpp"


// ================== HSPI PIN DEFINITIONS ==================
// HSPI pins for ESP32 (alternative to default VSPI)
#define HSPI_MOSI 13 	// GPIO13 for HSPI MOSI
#define HSPI_MISO 12    // GPIO12 for HSPI MISO
#define HSPI_SCK  14    // GPIO14 for HSPI SCK
#define HSPI_SS   15    // GPIO15 for HSPI SCK
// SS pin can be any GPIO - kept as parameter
// ==========================================================



class BroadcastSocket_SPI_ESP_Arduino_Master_HSPI : public BroadcastSocket_SPI_ESP_Arduino_Master_VSPI {

protected:
    // Needed for the compiler, the base class is the one being called though
    // ADD THIS CONSTRUCTOR - it calls the base class constructor
    BroadcastSocket_SPI_ESP_Arduino_Master_HSPI(JsonTalker** json_talkers, int* talkers_ss_pins, uint8_t talker_count)
        : BroadcastSocket_SPI_ESP_Arduino_Master_VSPI(json_talkers, talkers_ss_pins, talker_count) {

            _actual_ss_pin = HSPI_SS;
        }

public:

    // Move ONLY the singleton instance method to subclass
    static BroadcastSocket_SPI_ESP_Arduino_Master_HSPI& instance(JsonTalker** json_talkers, int* talkers_ss_pins, uint8_t talker_count) {
        static BroadcastSocket_SPI_ESP_Arduino_Master_HSPI instance(json_talkers, talkers_ss_pins, talker_count);

        return instance;
    }

    const char* class_name() const override { return "BroadcastSocket_SPI_ESP_Arduino_Master_HSPI"; }


    void begin() override {
		
		#ifdef BROADCAST_SPI_DEBUG
		Serial.println("Pins set for HSPI:");
		Serial.print("\tHSPI_SCK: ");
		Serial.println(HSPI_SCK);
		Serial.print("\tHSPI_MISO: ");
		Serial.println(HSPI_MISO);
		Serial.print("\tHSPI_MOSI: ");
		Serial.println(HSPI_MOSI);
		Serial.print("\tHSPI_SS: ");
		Serial.println(HSPI_SS);
		#endif

		// ================== INITIALIZE HSPI ==================
		// Initialize SPI with HSPI pins: SCK=14, MISO=12, MOSI=13
		// This method signature is only available in ESP32 Arduino SPI library!
		SPI.begin(HSPI_SCK, HSPI_MISO, HSPI_MOSI);
		
		initiate();
    }
};

#endif // BROADCAST_SOCKET_SPI_ESP_ARDUINO_MASTER_HSPI_HPP
