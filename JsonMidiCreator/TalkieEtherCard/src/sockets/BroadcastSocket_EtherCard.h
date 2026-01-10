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
#ifndef BROADCAST_SOCKET_ETHERCARD_H
#define BROADCAST_SOCKET_ETHERCARD_H

#include "../BroadcastSocket.h"
#include <EtherCard.h>


// #define BROADCAST_ETHERCARD_DEBUG
// #define ENABLE_DIRECT_ADDRESSING


class BroadcastSocket_EtherCard : public BroadcastSocket {
private:

	static JsonMessage _json_message;
    static char* _ptr_received_buffer;
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
        Serial.print(F("C: "));
        Serial.write(data, length);
        Serial.print(" | ");
        Serial.println(length);
        #endif

        if (length) {
			char* message_buffer = _json_message._write_buffer(length);
			if (message_buffer) {
				memcpy(message_buffer, data, length);
				memcpy(_source_ip, src_ip, 4);
				_data_length = length;	// Where is marked as received (> 0)
			}
		}
    }


protected:
    // Constructor
    BroadcastSocket_EtherCard() : BroadcastSocket() {
		
		ether.udpServerListenOnPort(staticCallback, _port);
	}


    void _receive() override {
		_data_length = 0;	// Makes sure this is only called once per message received (it's the Ethernet reading that sets it)
		ether.packetLoop(ether.packetReceive());	// Updates the _data_length variable
		if (_data_length) {
			
			#ifdef BROADCAST_ETHERCARD_DEBUG
			Serial.print(F("R: "));
			Serial.write(json_message._read_buffer(), json_message._get_length());
			Serial.println();
			#endif

			if (_json_message._validate_json()) {
				_json_message._process_checksum();	// Has to be done before transmission
				_startTransmission(_json_message);
			}
		}
    }


    bool _send(const JsonMessage& json_message) override {
        
		const char* message_buffer = json_message._read_buffer();
		size_t message_length = json_message._get_length();
		uint8_t broadcastIp[4] = {255, 255, 255, 255};
		
		#ifdef BROADCAST_ETHERCARD_DEBUG
		Serial.print(F("S: "));
		Serial.write(message_buffer, message_length);
		Serial.println();
		#endif

		#ifdef ENABLE_DIRECT_ADDRESSING
		ether.sendUdp(message_buffer, message_length, _port, _source_ip, _port);
		#else
		ether.sendUdp(message_buffer, message_length, _port, broadcastIp, _port);
		#endif

		return true;
    }


public:

    // Move ONLY the singleton instance method to subclass
    static BroadcastSocket_EtherCard& instance() {
        static BroadcastSocket_EtherCard instance;
        return instance;
    }

    const char* class_name() const override { return "BroadcastSocket_EtherCard"; }

};


#endif // BROADCAST_SOCKET_ETHERCARD_H
