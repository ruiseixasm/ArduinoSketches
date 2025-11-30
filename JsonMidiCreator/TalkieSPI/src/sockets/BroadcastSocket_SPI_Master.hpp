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
#ifndef BROADCAST_SOCKET_SPI_MASTER_HPP
#define BROADCAST_SOCKET_SPI_MASTER_HPP


#include <SPI.h>
#include <ArduinoJson.h>    // Include ArduinoJson Library to be used as a dictionary
#include "../BroadcastSocket.h"

// #define BROADCAST_SPI_MASTER_DEBUG


#define ENABLE_DIRECT_ADDRESSING


class BroadcastSocket_SPI_Master : public BroadcastSocket {
public:

    enum MessageCode : uint8_t {
        START   = 0xF0, // Start of transmission
        END     = 0xF1, // End of transmission
        ACK     = 0xF2, // Acknowledge
        NACK    = 0xF3, // Not acknowledged
        READY   = 0xF4, // Slave has response ready
        ERROR   = 0xF5, // Error frame
        RECEIVE = 0xF6, // Asks the receiver to start receiving
        SEND    = 0xF7, // Asks the receiver to start sending
        NONE    = 0xF8  // Means nothing to send
    };


private:
    uint8_t _receiving_index = 0;   // No interrupts, so, not volatile
    bool _receiving_state = true;
    JsonObject* _devices_ss_pins;

protected:
    // Needed for the compiler, the base class is the one being called though
    // ADD THIS CONSTRUCTOR - it calls the base class constructor
    BroadcastSocket_SPI_Master(JsonTalker** json_talkers, uint8_t talker_count)
        : BroadcastSocket(json_talkers, talker_count) {
            
            // // Initialize devices control object (optional initial setup)
            // devices_ss_pins["initialized"] = true;
        }


    bool sendJsonMessage(JsonObject json_message, bool as_reply = false) override {

        
        return BroadcastSocket::sendJsonMessage(json_message, as_reply);
    }


    size_t send(size_t length, bool as_reply = false) override {

        // Need to call homologous method in super class first
        length = BroadcastSocket::send(length, as_reply); // Very important pre processing !!

        if (length > 0) {

            


        }

        return length;
    }


public:

    // Move ONLY the singleton instance method to subclass
    static BroadcastSocket_SPI_Master& instance(JsonTalker** json_talkers, uint8_t talker_count) {
        static BroadcastSocket_SPI_Master instance(json_talkers, talker_count);
        return instance;
    }


    size_t receive() override {

        // Need to call homologous method in super class first
        BroadcastSocket::receive(); // Very important to do or else it may stop receiving !!

        return 0;   // nothing received
    }


    void setup(JsonObject* receivers_ss_pins) {
        // Initialize SPI
        SPI.begin();
        SPI.setClockDivider(SPI_CLOCK_DIV4);    // Only affects the char transmission
        SPI.setDataMode(SPI_MODE0);
        SPI.setBitOrder(MSBFIRST);  // EXPLICITLY SET MSB FIRST! (OTHERWISE is LSB)
        _devices_ss_pins = receivers_ss_pins;
    }
};

#endif // BROADCAST_SOCKET_SPI_MASTER_HPP
