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
#ifndef BROADCAST_SOCKET_ETHERNETENC_HPP
#define BROADCAST_SOCKET_ETHERNETENC_HPP

    // writeReg(ERXFCON, ERXFCON_UCEN | ERXFCON_CRCEN | ERXFCON_BCEN);

    // // Pattern matching disabled – not needed for broadcast

    // // writeRegPair(EPMM0, 0x303f);

    // // writeRegPair(EPMCSL, 0xf7f9);

#include "../Changed_EthernetENC/src/EthernetENC.h"
// #include <EthernetUdp.h>  // DON'T INCLUDE THIS ONE BECAUSE ARDUINO COMPILER CAN PICH THE WRONG ONE
#include "../Changed_EthernetENC/src/EthernetUdp.h"    // Forces the correct usage of EthernetUdp.h


#include "../BroadcastSocket.h"

// #define BROADCAST_ETHERNETENC_DEBUG


#define ENABLE_DIRECT_ADDRESSING


class BroadcastSocket_EthernetENC : public BroadcastSocket {
private:
    uint16_t _port = 5005;
    IPAddress _source_ip = IPAddress(255, 255, 255, 255);   // By default it's used the broadcast IP
    EthernetUDP* _udp = nullptr;

protected:
    // Needed for the compiler, the base class is the one being called though
    // ADD THIS CONSTRUCTOR - it calls the base class constructor
    BroadcastSocket_EthernetENC(JsonTalker** json_talkers, uint8_t talker_count)
        : BroadcastSocket(json_talkers, talker_count) {}


    bool send(size_t length, bool as_reply = false) override {
        if (_udp == nullptr) return false;

        IPAddress broadcastIP(255, 255, 255, 255);
        
        #ifdef ENABLE_DIRECT_ADDRESSING
        if (!_udp->beginPacket(as_reply ? _source_ip : broadcastIP, _port)) {
            #ifdef BROADCAST_ETHERNETENC_DEBUG
            Serial.println(F("Failed to begin packet"));
            #endif
            return false;
        }
        #else
        if (!_udp->beginPacket(broadcastIP, _port)) {
            #ifdef BROADCAST_ETHERNETENC_DEBUG
            Serial.println(F("Failed to begin packet"));
            #endif
            return false;
        }
        #endif

        size_t bytesSent = _udp->write(reinterpret_cast<const uint8_t*>(_sending_buffer), length);
        (void)bytesSent; // Silence unused variable warning

        if (!_udp->endPacket()) {
            #ifdef BROADCAST_ETHERNETENC_DEBUG
            Serial.println(F("Failed to end packet"));
            #endif
            return false;
        }

        #ifdef BROADCAST_ETHERNETENC_DEBUG
        Serial.print(F("S: "));
        Serial.write(_sending_buffer, length);
        Serial.println();
        #endif

        return true;
    }


public:

    // Move ONLY the singleton instance method to subclass
    static BroadcastSocket_EthernetENC& instance(JsonTalker** json_talkers, uint8_t talker_count) {
        static BroadcastSocket_EthernetENC instance(json_talkers, talker_count);
        return instance;
    }

    
    void set_port(uint16_t port) {
        _port = port;
    }
    

    size_t receive() override {
        if (_udp == nullptr) return 0;

        // Need to call homologous method in super class first
        BroadcastSocket::receive(); // Very im_portant to do or else it may stop receiving !!

        // Receive packets
        int packetSize = _udp->parsePacket();
        if (packetSize > 0) {

            // Avoids overflow
            if (packetSize > BROADCAST_SOCKET_BUFFER_SIZE) return 0;

            int length = _udp->read(_receiving_buffer, static_cast<size_t>(packetSize));
            if (length <= 0) return 0;  // Your requested check - handles all error cases
            
            #ifdef BROADCAST_ETHERNETENC_DEBUG
            Serial.print(packetSize);
            Serial.print(F("B from "));
            Serial.print(_udp->remoteIP());
            Serial.print(F(":"));
            Serial.print(_udp->remotePort());
            Serial.print(F(" -> "));
            Serial.println(_received_data);
            #endif
            
            _source_ip = _udp->remoteIP();
            return triggerTalkers(static_cast<size_t>(length));
        }
        return 0;   // nothing received
    }

    void set_udp(EthernetUDP* udp) { _udp = udp; }
};

#endif // BROADCAST_SOCKET_ETHERNETENC_HPP
