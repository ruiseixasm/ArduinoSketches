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


#define send_delay_us 10
#define receive_delay_us 10 // Receive needs more time to be processed


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
    JsonObject* _talkers_ss_pins;

protected:
    // Needed for the compiler, the base class is the one being called though
    // ADD THIS CONSTRUCTOR - it calls the base class constructor
    BroadcastSocket_SPI_Master(JsonTalker** json_talkers, uint8_t talker_count)
        : BroadcastSocket(json_talkers, talker_count) {
            
            _max_delay_ms = 0;  // SPI is sequencial, no need to control out of order packages
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

    
    size_t receiveString(int ss_pin) {
        size_t length = 0;	// No interrupts, so, not volatile
        uint8_t c; // Avoid using 'char' while using values above 127

        for (uint8_t r = 0; length == 0 && r < 3; r++) {
    
            digitalWrite(ss_pin, LOW);
            delayMicroseconds(5);

            // Asks the receiver to start receiving
            c = SPI.transfer(SEND);
            delayMicroseconds(send_delay_us);
                
            // Starts to receive all chars here
            for (uint8_t i = 0; i < BROADCAST_SOCKET_BUFFER_SIZE; i++) {	// First char is a control byte
                delayMicroseconds(receive_delay_us);
                if (i > 0) {    // The first response is discarded
                    c = SPI.transfer(_receiving_buffer[i - 1]);
                    if (c == END) {
                        _receiving_buffer[i] = '\0'; // Implicit char
                        length = i;
                        break;
                    } else if (c == ERROR) {
                        _receiving_buffer[0] = '\0'; // Implicit char
                        length = 0;
                        break;
                    } else {
                        _receiving_buffer[i] = c;
                    }
                } else {
                    c = SPI.transfer('\0');   // Dummy char, not intended to be processed
                    if (c == NONE) {
                        _receiving_buffer[0] = '\0'; // Implicit char
                        length = 1;
                        break;
                    }
                    _receiving_buffer[0] = c;   // First char received
                }
            }

            delayMicroseconds(5);
            digitalWrite(ss_pin, HIGH);
        }

        return length;
    }


    size_t sendString(int ss_pin) {
        size_t length = 0;	// No interrupts, so, not volatile
        uint8_t c; // Avoid using 'char' while using values above 127

        for (size_t s = 0; length == 0 && s < 3; s++) {
    
            digitalWrite(ss_pin, LOW);
            delayMicroseconds(5);

            // Asks the receiver to start receiving
            SPI.transfer(RECEIVE);
            delayMicroseconds(send_delay_us);
            
            // RECEIVE message code
            for (uint8_t i = 0; i < BROADCAST_SOCKET_BUFFER_SIZE; i++) {
                if (i > 0) {
                    if (SPI.transfer(_sending_buffer[i]) != _sending_buffer[i - 1])
                        length = 0;
                } else {
                    SPI.transfer(_sending_buffer[0]);
                }
                delayMicroseconds(send_delay_us);
                // Don't make '\0' implicit in order to not have to change the SPDR on the slave side!!
                if (_sending_buffer[i] == '\0') {
                    length = i + 1;
                    break;
                }
            }

            if (SPI.transfer(END) != '\0')  // Because the last char is always '\0'
                length = 0;

            delayMicroseconds(5);
            digitalWrite(ss_pin, HIGH);
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
        size_t length = BroadcastSocket::receive(); // Very important to do or else it may stop receiving !!
        bool received_once = false;

        for (auto key_value : *_talkers_ss_pins) {
            // const char* key = key_value.key().c_str();
            int ss_pin = key_value.value();

            if(receiveString(ss_pin) > 0)
                received_once = true;
        }


        return length;   // nothing received
    }


    void setup(JsonObject* talkers_ss_pins) {
        // Initialize SPI
        SPI.begin();
        SPI.setClockDivider(SPI_CLOCK_DIV4);    // Only affects the char transmission
        SPI.setDataMode(SPI_MODE0);
        SPI.setBitOrder(MSBFIRST);  // EXPLICITLY SET MSB FIRST! (OTHERWISE is LSB)
        _talkers_ss_pins = talkers_ss_pins;
    }
};

#endif // BROADCAST_SOCKET_SPI_MASTER_HPP
