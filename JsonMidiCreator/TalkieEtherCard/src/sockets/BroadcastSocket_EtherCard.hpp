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

    uint16_t _port = 5005;
    static uint8_t _source_ip[4];
    static size_t _data_length;

    // ===== [SELF IP] cache our own IP =====
    static uint8_t _local_ip[4];

    static void staticCallback(uint16_t src_port, uint8_t* src_ip, uint16_t dst_port, 
                          const char* data, uint16_t length) {

        (void)src_port; // Silence unused parameter warning
        (void)dst_port; // Silence unused parameter warning

        // ===== [SELF IP] DROP self-sent packets =====
        if (memcmp(src_ip, _local_ip, 4) == 0) {
            return;   // silently discard
        }

        #ifdef BROADCAST_ETHERCARD_DEBUG
        Serial.print(F("R: "));
        Serial.write(data, length);
        Serial.println();
        #endif

        if (length < BROADCAST_SOCKET_BUFFER_SIZE) {
            memcpy(_receiving_buffer, data, length);
            memcpy(_source_ip, src_ip, 4);
            _data_length = length;  // Where is marked as received (> 0)
        }
    }


protected:
    // Constructor
    BroadcastSocket_EtherCard() : BroadcastSocket() {}

public:

    // Move ONLY the singleton instance method to subclass
    static BroadcastSocket_EtherCard& instance() {
        static BroadcastSocket_EtherCard instance;
        return instance;
    }

    const char* class_name() const override { return "BroadcastSocket_EtherCard"; }


    void set_port(uint16_t port) {

        // ===== [SELF IP] store local IP for self-filtering =====
        memcpy(_local_ip, ether.myip, 4);
        _port = port;
        ether.udpServerListenOnPort(staticCallback, port);
    }

    
    size_t send(size_t length, bool as_reply = false) override {
        
        uint8_t broadcastIp[4] = {255, 255, 255, 255};
        
        // Need to call homologous method in super class first
        length = BroadcastSocket::send(length, as_reply); // Very important pre processing !!

        #ifdef BROADCAST_ETHERCARD_DEBUG
        Serial.print(F("S: "));
        Serial.write(_sending_buffer, length);
        Serial.println();
        #endif

        #ifdef ENABLE_DIRECT_ADDRESSING
        ether.sendUdp(_sending_buffer, length, _port, as_reply ? _source_ip : broadcastIp, _port);
        #else
        (void)as_reply; // Silence unused parameter warning
        ether.sendUdp(_sending_buffer, length, _port, broadcastIp, _port);
        #endif

        return length;
    }


    size_t receive() override {
        _data_length = BroadcastSocket::receive();  // Makes sure it's the Ethernet reading that sets it! (always returns 0)
        ether.packetLoop(ether.packetReceive());	// Updates the _data_length variable
        return BroadcastSocket::triggerTalkers(_data_length);
    }

};

uint8_t BroadcastSocket_EtherCard::_source_ip[4] = {0};
size_t BroadcastSocket_EtherCard::_data_length = 0;
// ===== [SELF IP] =====
uint8_t BroadcastSocket_EtherCard::_local_ip[4] = {0};


#endif // BROADCAST_SOCKET_ETHERCARD_HPP
