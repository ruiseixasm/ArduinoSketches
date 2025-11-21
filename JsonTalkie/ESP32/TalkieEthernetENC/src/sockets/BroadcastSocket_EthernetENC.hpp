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

#include "../BroadcastSocket.hpp"
#include <EthernetENC.h>
#include <EthernetUdp.h>  // Using EthernetUdp instead of UIPUdp


#define BROADCAST_ETHERNETENC_DEBUG
// #define ENABLE_DIRECT_ADDRESSING


class BroadcastSocket_EthernetENC : public BroadcastSocket {
private:
    IPAddress _source_ip;   // By default it's used the broadcast IP
    EthernetUDP* _udp = nullptr;

    // Private constructor for singleton
    BroadcastSocket_EthernetENC() {
        _source_ip = IPAddress(255, 255, 255, 255);   // By default it's used the broadcast IP
    }

public:
    // Singleton accessor
    static BroadcastSocket_EthernetENC& instance() {
        static BroadcastSocket_EthernetENC instance;
        return instance;
    }

    
    bool send(const char* data, size_t size, bool as_reply = false) override {
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

        size_t bytesSent = _udp->write(reinterpret_cast<const uint8_t*>(data), size);
        (void)bytesSent; // Silence unused variable warning

        if (!_udp->endPacket()) {
            #ifdef BROADCAST_ETHERNETENC_DEBUG
            Serial.println(F("Failed to end packet"));
            #endif
            return false;
        }

        #ifdef BROADCAST_ETHERNETENC_DEBUG
        Serial.print(F("S: "));
        Serial.write(data, size);
        Serial.println();
        #endif

        return true;
    }


    size_t receive(char* buffer, size_t size) override {
        if (_udp == nullptr) return 0;
        
        // Receive packets
        int packetSize = _udp->parsePacket();
        if (packetSize > 0) {
            // Use std::min instead of min to avoid potential conflicts
            int length = _udp->read(buffer, std::min(static_cast<size_t>(packetSize), size));
            if (length <= 0) return 0;  // Your requested check - handles all error cases
            
            #ifdef BROADCAST_ETHERNETENC_DEBUG
            Serial.print(packetSize);
            Serial.print(F("B from "));
            Serial.print(_udp->remoteIP());
            Serial.print(F(":"));
            Serial.print(_udp->remotePort());
            Serial.print(F(" -> "));
            Serial.println(buffer);
            #endif
            
            _source_ip = _udp->remoteIP();
            return jsonStrip(buffer, static_cast<size_t>(length));
        }
        return 0;   // nothing received
    }

    void set_udp(EthernetUDP* udp) { _udp = udp; }
};

#endif // BROADCAST_SOCKET_ETHERNETENC_HPP
