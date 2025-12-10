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
#define SPI_MOSI 13    // GPIO13 for HSPI MOSI
#define SPI_MISO 12    // GPIO12 for HSPI MISO
#define SPI_SCK  14    // GPIO14 for HSPI SCK
#define SPI_SS   15    // GPIO15 for HSPI SCK
// SS pin can be any GPIO - kept as parameter
// ==========================================================



class BroadcastSocket_SPI_ESP_Arduino_Master_HSPI : public BroadcastSocket_SPI_ESP_Arduino_Master_VSPI {

protected:
    // Needed for the compiler, the base class is the one being called though
    // ADD THIS CONSTRUCTOR - it calls the base class constructor
    BroadcastSocket_SPI_ESP_Arduino_Master_HSPI(JsonTalker** json_talkers, uint8_t talker_count)
        : BroadcastSocket_SPI_ESP_Arduino_Master_VSPI(json_talkers, talker_count) {
            
        }

public:

    // Move ONLY the singleton instance method to subclass
    static BroadcastSocket_SPI_ESP_Arduino_Master_HSPI& instance(JsonTalker** json_talkers, uint8_t talker_count) {
        static BroadcastSocket_SPI_ESP_Arduino_Master_HSPI instance(json_talkers, talker_count);
        return instance;
    }

	// Override setup to use HSPI pins
    void setup(int* talkers_ss_pins, uint8_t ss_pins_count) override {
        // First, reinitialize SPI with HSPI pins
        SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
        SPI.setClockDivider(SPI_CLOCK_DIV4);
        SPI.setDataMode(SPI_MODE0);
        SPI.setBitOrder(MSBFIRST);
        
        // Then call parent setup
        BroadcastSocket_SPI_ESP_Arduino_Master_VSPI::setup(talkers_ss_pins, ss_pins_count);
    }
};

#endif // BROADCAST_SOCKET_SPI_ESP_ARDUINO_MASTER_HSPI_HPP
