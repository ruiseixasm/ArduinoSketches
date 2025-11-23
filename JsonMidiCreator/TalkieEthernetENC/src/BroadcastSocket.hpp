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
#ifndef BROADCAST_SOCKET_HPP
#define BROADCAST_SOCKET_HPP

#include <Arduino.h>    // Needed for Serial given that Arduino IDE only includes Serial in .ino files!
#include "JsonTalker.h"


// #define BROADCASTSOCKET_DEBUG

class BroadcastSocket {
private:

    JsonTalker** _device_talkers = nullptr;   // A list of Talkers (pointers)
    size_t _talker_count = 0;

protected:
    uint16_t _port = 5005;


    size_t triggerTalkers(char* buffer, size_t length) {

        uint16_t received_checksum = BroadcastSocket::readChecksum(buffer, &length);
        uint16_t checksum = BroadcastSocket::getChecksum(buffer, length);

        if (received_checksum == checksum) {
            // Triggers all Talkers to processes the received data
            for (size_t talker_i = 0; talker_i < _talker_count; ++talker_i) {
                JsonTalker* talker = _device_talkers[talker_i];
                talker->receiveData(this, buffer, length);
            }
        }
        return length;
    }

    // Private constructor
    BroadcastSocket() = default;
    virtual ~BroadcastSocket() = default;

    BroadcastSocket(JsonTalker** device_talkers, size_t talker_count)
        : _device_talkers(device_talkers), _talker_count(talker_count) {}


public:
    // Delete copy/move operations
    BroadcastSocket(const BroadcastSocket&) = delete;
    BroadcastSocket& operator=(const BroadcastSocket&) = delete;
    BroadcastSocket(BroadcastSocket&&) = delete;
    BroadcastSocket& operator=(BroadcastSocket&&) = delete;

    // Singleton accessor
    static BroadcastSocket& instance(JsonTalker** device_talkers, size_t talker_count) {
        static BroadcastSocket instance(device_talkers, talker_count);
        return instance;
    }


    // NOT Pure virtual methods anymores (= 0;)
    virtual bool send(const char* data, size_t len, bool as_reply = false) {
        (void)data; // Silence unused parameter warning
        (void)len; // Silence unused parameter warning
        (void)as_reply; // Silence unused parameter warning
        return false;
    }
    virtual size_t receive(char* buffer, size_t size) {
        (void)buffer; // Silence unused parameter warning
        (void)size; // Silence unused parameter warning
        return 0;
    }
    
    virtual void set_port(uint16_t port) { _port = port; }
    virtual uint16_t get_port() { return _port; }



    static uint16_t readChecksum(char* source_data, size_t* source_len) {
        
        // ASCII byte values:
        // 	'c' = 99
        // 	':' = 58
        // 	'"' = 34
        // 	'0' = 48
        // 	'9' = 57

        uint16_t data_checksum = 0;
        // Has to be pre processed (linearly)
        bool at_c0 = false;
        size_t data_i = 4;
        for (size_t i = data_i; i < *source_len; ++i) {
            if (!at_c0 && source_data[i - 3] == 'c' && source_data[i - 1] == ':' && source_data[i - 4] == '"' && source_data[i - 2] == '"') {
                at_c0 = true;
                data_checksum = source_data[data_i] - '0';
                source_data[data_i++] = '0';
                continue;
            } else if (at_c0) {
                if (source_data[i] < '0' || source_data[i] > '9') {
                    at_c0 = false;
                } else {
                    data_checksum *= 10;
                    data_checksum += source_data[i] - '0';
                    continue;
                }
            }
            source_data[data_i] = source_data[i]; // Does an offset
            data_i++;
        }
        *source_len = data_i;
        return data_checksum;
    }
    

    static uint16_t getChecksum(const char* net_data, const size_t len) {
        // 16-bit word and XORing
        uint16_t checksum = 0;
        for (size_t i = 0; i < len; i += 2) {
            uint16_t chunk = net_data[i] << 8;
            if (i + 1 < len) {
                chunk |= net_data[i + 1];
            }
            checksum ^= chunk;
        }
        return checksum;
    }




};


#endif // BROADCAST_SOCKET_HPP
