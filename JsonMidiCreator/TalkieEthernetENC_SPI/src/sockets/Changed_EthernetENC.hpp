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
#ifndef CHANGED_ETHERNETENC_HPP
#define CHANGED_ETHERNETENC_HPP

    // writeReg(ERXFCON, ERXFCON_UCEN | ERXFCON_CRCEN | ERXFCON_BCEN);

    // // Pattern matching disabled – not needed for broadcast

    // // writeRegPair(EPMM0, 0x303f);

    // // writeRegPair(EPMCSL, 0xf7f9);

#include "../Changed_EthernetENC/src/EthernetENC.h"
// #include <EthernetUdp.h>  // DON'T INCLUDE THIS ONE BECAUSE ARDUINO COMPILER CAN PICH THE WRONG ONE
#include "../Changed_EthernetENC/src/EthernetUdp.h"    // Forces the correct usage of EthernetUdp.h


#include "../BroadcastSocket.hpp"


// #define BROADCAST_ETHERNETENC_DEBUG
// #define BROADCAST_ETHERNETENC_DEBUG_NEW

#define ENABLE_DIRECT_ADDRESSING


class Changed_EthernetENC : public BroadcastSocket {
protected:

    uint16_t _port = 5005;
    IPAddress _source_ip = IPAddress(255, 255, 255, 255);   // By default it's used the broadcast IP
    EthernetUDP* _udp = nullptr;
	char _new_from_name[NAME_LEN] = {'\0'};
	String _from_name = "";

    // ===== [SELF IP] cache our own IP =====
    IPAddress _local_ip;

	
    // Constructor
    Changed_EthernetENC(JsonTalker* const* const json_talkers, uint8_t talker_count, SourceValue source_value = SourceValue::REMOTE)
        : BroadcastSocket(json_talkers, talker_count, source_value) {}



    uint8_t receive() override {
        if (_udp == nullptr) return 0;

        // Need to call homologous method in super class first
        BroadcastSocket::receive(); // Very important to do or else it may stop receiving !!

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
				
                return 0;
            } else {
				
				#ifdef BROADCAST_ETHERNETENC_DEBUG
				Serial.println(F("\treceive1: Packet NOT sent from this socket"));
				Serial.print(F("\t\tRemote IP: "));
            	Serial.println(_udp->remoteIP());
				Serial.print(F("\t\tLocal IP:  "));
            	Serial.println(_local_ip);
				#endif
				
			}

            // Avoids overflow
            if (packetSize > BROADCAST_SOCKET_BUFFER_SIZE) return 0;

            int length = _udp->read(_receiving_buffer, static_cast<size_t>(packetSize));
            if (length <= 0) return 0;  // Your requested check - handles all error cases
            
            #ifdef BROADCAST_ETHERNETENC_DEBUG
            Serial.print(F("\treceive1: "));
            Serial.print(packetSize);
            Serial.print(F("B from "));
            Serial.print(_udp->remoteIP());
            Serial.print(F(" to "));
            Serial.print(_local_ip);
            Serial.print(F(" -->      "));
            Serial.println(_receiving_buffer);
            #endif
            
            _source_ip = _udp->remoteIP();
			// Makes sure the _received_length is set
			_received_length = length;
			triggerTalkers();
			// Makes sure the _receiving_buffer is deleted with 0
			_received_length = 0;
			return length;
        }
        return 0;   // nothing received
    }


	// Allows the overriding class to peek at the received JSON message
	bool receivedJsonMessage(JsonMessage& new_json_message) override {

		if (BroadcastSocket::receivedJsonMessage(new_json_message)) {
			// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
			strcpy(_new_from_name, new_json_message.get_from_name());
			
			#ifdef BROADCAST_ETHERNETENC_DEBUG_NEW
			Serial.print(F("receivedJsonMessage1: "));
			new_json_message.write_to(Serial);
			Serial.print(" | ");
			Serial.println(_new_from_name);
			#endif

			return true;
		}
		return false;
	}


    bool send(const JsonMessage& new_json_message) override {
		
        if (_udp && BroadcastSocket::send(new_json_message)) {	// Very important pre processing !!
			
            IPAddress broadcastIP(255, 255, 255, 255);

            #ifdef ENABLE_DIRECT_ADDRESSING

			// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
			bool as_reply = new_json_message.is_to_name(_new_from_name);

			#ifdef BROADCAST_ETHERNETENC_DEBUG_NEW
			Serial.print(F("\t\t\t\t\tsend orgn: "));
			new_json_message.write_to(Serial);
			Serial.println();
			Serial.print(F("\t\t\t\t\tsend json: "));
			new_json_message.write_to(Serial);
			Serial.print(" | ");
			Serial.println(as_reply);
			Serial.print(F("\t\t\t\t\tsend buff: "));
			Serial.write(_sending_buffer, _sending_length);
			Serial.print(" | ");
			Serial.println(_sending_length);
			#endif

            if (!_udp->beginPacket(as_reply ? _source_ip : broadcastIP, _port)) {
                #ifdef BROADCAST_ETHERNETENC_DEBUG
                Serial.println(F("\tFailed to begin packet"));
                #endif
                return false;
            } else {
				
				#ifdef BROADCAST_ETHERNETENC_DEBUG
				if (as_reply && _source_ip != broadcastIP) {

					Serial.print(F("\tsend1: --> Directly sent to the  "));
					Serial.print(_source_ip);
					Serial.print(F(" address --> "));
					
				} else {
					
					Serial.print(F("\tsend1: --> Broadcast sent to the 255.255.255.255 address --> "));
					
				}
				#endif
			}
            #else
            if (!_udp->beginPacket(broadcastIP, _port)) {
                #ifdef BROADCAST_ETHERNETENC_DEBUG
                Serial.println(F("\tFailed to begin packet"));
                #endif
                return false;
            } else {
									
				#ifdef BROADCAST_ETHERNETENC_DEBUG
				Serial.print(F("\tsend1: --> Broadcast sent to the 255.255.255.255 address --> "));
				#endif

			}
            #endif

            size_t bytesSent = _udp->write(reinterpret_cast<const uint8_t*>(_sending_buffer), _sending_length);
            (void)bytesSent; // Silence unused variable warning

            if (!_udp->endPacket()) {
                #ifdef BROADCAST_ETHERNETENC_DEBUG
                Serial.println(F("\n\t\tERROR: Failed to end packet"));
                #endif
                return false;
            }

            #ifdef BROADCAST_ETHERNETENC_DEBUG
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
    static Changed_EthernetENC& instance(JsonTalker* const* json_talkers, uint8_t talker_count, SourceValue source_value = SourceValue::REMOTE) {
        static Changed_EthernetENC instance(json_talkers, talker_count, source_value);
        return instance;
    }

    const char* class_name() const override { return "Changed_EthernetENC"; }


    void set_port(uint16_t port) { _port = port; }
    void set_udp(EthernetUDP* udp) {
        
        // ===== [SELF IP] store local IP for self-filtering =====
        _local_ip = Ethernet.localIP();
        _udp = udp;
    }

};

#endif // CHANGED_ETHERNETENC_HPP
