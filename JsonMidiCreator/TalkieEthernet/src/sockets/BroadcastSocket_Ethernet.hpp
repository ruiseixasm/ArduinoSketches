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
    EthernetUDP* _udp = nullptr;
	// Source Talker info
	char _from_name[TALKIE_NAME_LEN] = {'\0'};
    IPAddress _from_ip = IPAddress(255, 255, 255, 255);   // By default it's used the broadcast IP
    // ===== [SELF IP] cache our own IP =====
    IPAddress _local_ip;


protected:
    // Constructor
    BroadcastSocket_Ethernet() : BroadcastSocket() {}


    void _receive() override {
        if (_udp) {

			// Receive packets
			int packetSize = _udp->parsePacket();
			if (packetSize > 0) {

				// ===== [SELF IP] DROP self-sent packets =====
				if (_udp->remoteIP() == _local_ip) {
					_udp->flush();   // discard payload
					
					#ifdef BROADCAST_ETHERNETENC_DEBUG
					Serial.println(F("\treceive1: Dropped packet for being sent from this socket"));
					Serial.print(F("\t\tRemote IP: "));
					Serial.println(_udp->remoteIP());
					Serial.print(F("\t\tLocal IP:  "));
					Serial.println(_local_ip);
					#endif
					
					return;
				} else {
					
					#ifdef BROADCAST_ETHERNETENC_DEBUG
					Serial.println(F("\treceive1: Packet NOT sent from this socket"));
					Serial.print(F("\t\tRemote IP: "));
					Serial.println(_udp->remoteIP());
					Serial.print(F("\t\tLocal IP:  "));
					Serial.println(_local_ip);
					#endif
					
				}

				JsonMessage new_message;
				char* message_buffer = new_message._write_buffer(packetSize);
				if (!message_buffer) return;	// Avoids overflow

				int length = _udp->read(message_buffer, static_cast<size_t>(packetSize));
				if (length == packetSize) {
				
					if (new_message._validate_json()) {
				
						if (new_message._process_checksum()) {
							strcpy(_from_name, new_message.get_from_name());
							_from_ip = _udp->remoteIP();
						}
		
						#ifdef BROADCAST_ETHERNETENC_DEBUG
						Serial.print(F("\treceive1: "));
						Serial.print(packetSize);
						Serial.print(F("B from "));
						Serial.print(_udp->remoteIP());
						Serial.print(F(" to "));
						Serial.print(_local_ip);
						Serial.print(F(" -->      "));
						Serial.println(message_buffer);
						#endif
						
						_startTransmission(new_message);
					}
				}
			}
		}
    }


    bool _send(const JsonMessage& json_message) override {

		if (_udp) {

			IPAddress broadcastIP(255, 255, 255, 255);

			#ifdef ENABLE_DIRECT_ADDRESSING
			if (!_udp->beginPacket(_from_ip, _port)) {
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

			size_t bytesSent = _udp->write(
				reinterpret_cast<const uint8_t*>( json_message._read_buffer() ),
				json_message._get_length()
			);
			(void)bytesSent; // Silence unused variable warning

			if (!_udp->endPacket()) {
				#ifdef BROADCAST_ETHERNETENC_DEBUG
				Serial.println(F("Failed to end packet"));
				#endif
				return false;
			}

			#ifdef BROADCAST_ETHERNETENC_DEBUG
			Serial.print(F("S: "));
			Serial.write(
				json_message._read_buffer(),
				json_message._get_length()
			);
			Serial.println();
			#endif

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

    const char* class_name() const override { return "BroadcastSocket_Ethernet"; }


    void set_port(uint16_t port) { _port = port; }
    void set_udp(EthernetUDP* udp) {
		
        // ===== [SELF IP] store local IP for self-filtering =====
        _local_ip = Ethernet.localIP();
		_udp = udp;
	}
};

#endif // BROADCAST_SOCKET_ETHERNET_HPP
