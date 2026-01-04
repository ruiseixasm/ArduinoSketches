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

        if (length && length < TALKIE_BUFFER_SIZE) {
            memcpy(_ptr_received_buffer, data, length);
            memcpy(_source_ip, src_ip, 4);
            _data_length = length;	// Where is marked as received (> 0)
        } else {
			_data_length = 0;
		}
    }


protected:
    // Constructor
    BroadcastSocket_EtherCard() : BroadcastSocket() {
		
		// For static access to the buffers
		_ptr_received_buffer = _received_buffer;
		ether.udpServerListenOnPort(staticCallback, _port);
	}


    size_t receive() override {
		_data_length = 0;	// Makes sure this is only called once per message received (it's the Ethernet reading that sets it)
		ether.packetLoop(ether.packetReceive());	// Updates the _data_length variable
		if (_data_length) {
			_received_length = _data_length;
			
			#ifdef BROADCAST_ETHERCARD_DEBUG
			Serial.print(F("R: "));
			Serial.write(_received_buffer, _received_length);
			Serial.println();
			#endif

			BroadcastSocket::receive();  // Makes sure it's the Ethernet reading that sets it! (always returns 0)
			BroadcastSocket::startTransmission();
			_received_length = 0;
			return _data_length;
		}
		return 0;
    }


    bool send(const JsonMessage& json_message) override {
        
        if (BroadcastSocket::send(json_message)) {	// Very important pre processing !!
				
			uint8_t broadcastIp[4] = {255, 255, 255, 255};
			
			#ifdef BROADCAST_ETHERCARD_DEBUG
			Serial.print(F("S: "));
			Serial.write(_sending_buffer, _sending_length);
			Serial.println();
			#endif

			#ifdef ENABLE_DIRECT_ADDRESSING
			ether.sendUdp(_sending_buffer, _sending_length, _port, _source_ip, _port);
			#else
			ether.sendUdp(_sending_buffer, _sending_length, _port, broadcastIp, _port);
			#endif

			_sending_length = 0;	// Marks sending buffer available

			return true;
		}
        return false;
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
