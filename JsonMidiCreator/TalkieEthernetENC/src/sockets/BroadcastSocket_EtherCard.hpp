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
#ifndef BROADCAST_SOCKET_ETHERCARD_HPP
#define BROADCAST_SOCKET_ETHERCARD_HPP

#include "../BroadcastSocket.h"
#include <EtherCard.h>


// #define BROADCAST_ETHERCARD_DEBUG

// #define ENABLE_DIRECT_ADDRESSING


class BroadcastSocket_EtherCard : public BroadcastSocket {
private:

    static BroadcastSocket_EtherCard* _self_instance;
    static uint8_t _source_ip[4];
    static size_t _data_length;

    static void staticCallback(uint16_t src_port, uint8_t* src_ip, uint16_t dst_port, 
                          const char* data, uint16_t length) {

        (void)src_port; // Silence unused parameter warning
        (void)dst_port; // Silence unused parameter warning

        #ifdef BROADCAST_ETHERCARD_DEBUG
        Serial.print(F("R: "));
        Serial.write(data, length);
        Serial.println();
        #endif

        if (length <= BROADCAST_SOCKET_BUFFER_SIZE) {
            memcpy(_received_data, data, length);
            memcpy(_source_ip, src_ip, 4);
            if (_self_instance) {
                _data_length = _self_instance->triggerTalkers(_received_data, length);
            } else {
                #ifdef BROADCAST_ETHERCARD_DEBUG
                Serial.println(F("Instance is NULL!"));
                #endif
            }
        }
    }


protected:
    // Needed for the compiler, the base class is the one being called though
    // ADD THIS CONSTRUCTOR - it calls the base class constructor
    BroadcastSocket_EtherCard(JsonTalker* json_talkers, size_t talker_count)
        : BroadcastSocket(json_talkers, talker_count) {
            _self_instance = this;
        }

public:

    // Move ONLY the singleton instance method to subclass
    static BroadcastSocket_EtherCard& instance(JsonTalker* json_talkers, size_t talker_count) {
        static BroadcastSocket_EtherCard instance(json_talkers, talker_count);
        return instance;
    }

    
    bool send(const char* data, size_t size, bool as_reply = false) override {
        uint8_t broadcastIp[4] = {255, 255, 255, 255};
        
        #ifdef BROADCAST_ETHERCARD_DEBUG
        Serial.print(F("S: "));
        Serial.write(data, size);
        Serial.println();
        #endif

        #ifdef ENABLE_DIRECT_ADDRESSING
        ether.sendUdp(data, size, _port, as_reply ? _source_ip : broadcastIp, _port);
        #else
        (void)as_reply; // Silence unused parameter warning
        ether.sendUdp(data, size, _port, broadcastIp, _port);
        #endif

        return true;
    }


    size_t receive() override {
        _data_length = 0;   // Makes sure it's the Ethernet reading that sets it!
        ether.packetLoop(ether.packetReceive());
        return _data_length;
    }


    // Modified methods to work with singleton
    void set_port(uint16_t port) override {
        _port = port;
        ether.udpServerListenOnPort(staticCallback, _port);
    }
};

BroadcastSocket_EtherCard* BroadcastSocket_EtherCard::_self_instance = nullptr;
uint8_t BroadcastSocket_EtherCard::_source_ip[4] = {0};
size_t BroadcastSocket_EtherCard::_data_length = 0;


#endif // BROADCAST_SOCKET_ETHERCARD_HPP
