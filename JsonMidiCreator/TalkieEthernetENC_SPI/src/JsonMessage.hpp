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
#ifndef JSON_MESSAGE_HPP
#define JSON_MESSAGE_HPP

#include <Arduino.h>        // Needed for Serial given that Arduino IDE only includes Serial in .ino files!
#include "TalkieCodes.hpp"

#ifndef BROADCAST_SOCKET_BUFFER_SIZE
#define BROADCAST_SOCKET_BUFFER_SIZE 128
#endif



class JsonMessage {
protected:

	char _json_buffer[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};
	uint8_t _length = 0;


public:

    virtual const char* class_name() const { return "JsonMessage"; }

	JsonMessage() {
		// Does nothing
	}

	~JsonMessage() {
		// Does nothing
	}


    uint16_t getChecksum() {
        // 16-bit word and XORing
        uint16_t checksum = 0;
		if (_length <= BROADCAST_SOCKET_BUFFER_SIZE) {
			for (size_t i = 0; i < _length; i += 2) {
				uint16_t chunk = _json_buffer[i] << 8;
				if (i + 1 < _length) {
					chunk |= _json_buffer[i + 1];
				}
				checksum ^= chunk;
			}
		}
        return checksum;
    }

	
	bool hasKey(char key) const {
		if (_length > 6) {	// 6 because {"k":x} meaning 7 of length minumum
			for (uint8_t char_i = 4; char_i < _length; ++char_i) {	// 4 because it's the shortest position possible for ':'
				if (_json_buffer[char_i] == ':') {
					if (_json_buffer[char_i - 2] == key && _json_buffer[char_i - 3] == '"' && _json_buffer[char_i - 1] == '"') {
						return true;
					}
				}
			}
		}
		return false;
	}

	
};



#endif // JSON_MESSAGE_HPP
