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
#ifndef BROADCAST_SOCKET_ETHERNET_HPP
#define BROADCAST_SOCKET_ETHERNET_HPP

#include <Ethernet.h>
#include <EthernetUdp.h>
#include "../BroadcastSocket.h"

// #define BROADCAST_ETHERNETENC_DEBUG
#define ENABLE_DIRECT_ADDRESSING


class BroadcastSocket_Ethernet : public BroadcastSocket {
private:
    uint16_t _port = 5005;
    IPAddress _source_ip = IPAddress(255, 255, 255, 255);   // By default it's used the broadcast IP
    EthernetUDP* _udp = nullptr;

protected:
    // Constructor
    BroadcastSocket_Ethernet() : BroadcastSocket() {}


    size_t receive() override {
        if (_udp == nullptr) return 0;

        // Need to call homologous method in super class first
        BroadcastSocket::receive(); // Very important to do or else it may stop receiving !!

        // Receive packets
        int packetSize = _udp->parsePacket();
        if (packetSize > 0) {

            // Avoids overflow
            if (packetSize > BROADCAST_SOCKET_BUFFER_SIZE) return 0;

            int length = _udp->read(_receiving_buffer, static_cast<size_t>(packetSize));
			if (length > 0) {

				_received_length = (size_t)length;
				
				#ifdef BROADCAST_ETHERNETENC_DEBUG
				Serial.print(packetSize);
				Serial.print(F("B from "));
				Serial.print(_udp->remoteIP());
				Serial.print(F(":"));
				Serial.print(_udp->remotePort());
				Serial.print(F(" -> "));
				Serial.write(_receiving_buffer, _received_length);
				Serial.println();
				#endif
				
				_source_ip = _udp->remoteIP();
				triggerTalkers();
				_received_length = 0;
				return (size_t)length;
			}
        }
        return 0;   // nothing received
    }


    bool send(const JsonMessage& json_message) override {
        if (_udp == nullptr) return false;

        if (BroadcastSocket::send(json_message)) {	// Very important pre processing !!

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

			size_t bytesSent = _udp->write(reinterpret_cast<const uint8_t*>(_sending_buffer), _sending_length);
			(void)bytesSent; // Silence unused variable warning

			if (!_udp->endPacket()) {
				#ifdef BROADCAST_ETHERNETENC_DEBUG
				Serial.println(F("Failed to end packet"));
				#endif
				return false;
			}

			#ifdef BROADCAST_ETHERNETENC_DEBUG
			Serial.print(F("S: "));
			Serial.write(_sending_buffer, _sending_length);
			Serial.println();
			#endif

			_sending_length = 0;	// Marks sending buffer available
			return true;
		}

        return false;
    }


public:

    // Move ONLY the singleton instance method to subclass
    static BroadcastSocket_Ethernet& instance() {
        static BroadcastSocket_Ethernet instance;
        return instance;
    }


    void set_port(uint16_t port) { _port = port; }
    void set_udp(EthernetUDP* udp) { _udp = udp; }
};

#endif // BROADCAST_SOCKET_ETHERNET_HPP
