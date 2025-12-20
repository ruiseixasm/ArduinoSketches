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
public:
	
	enum ValueType : int {
		STRING, INTEGER, OTHER, VOID
	};


protected:

	char _json_payload[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};
	size_t _json_length = 0;


public:

    virtual const char* class_name() const { return "JsonMessage"; }

	JsonMessage() {
		// Does nothing
	}

	~JsonMessage() {
		// Does nothing
	}


	size_t get_length() const {
		return _json_length;
	}

    uint16_t getChecksum() {
        // 16-bit word and XORing
        uint16_t checksum = 0;
		if (_json_length <= BROADCAST_SOCKET_BUFFER_SIZE) {
			for (size_t i = 0; i < _json_length; i += 2) {
				uint16_t chunk = _json_payload[i] << 8;
				if (i + 1 < _json_length) {
					chunk |= _json_payload[i + 1];
				}
				checksum ^= chunk;
			}
		}
        return checksum;
    }

	bool deserialize(const char* buffer, size_t length) {
		if (length <= BROADCAST_SOCKET_BUFFER_SIZE) {
			for (size_t char_i = 0; char_i < length; ++char_i) {
				_json_payload[char_i] = buffer[char_i];
			}
			_json_length = length;
			return true;
		}
		return false;
	}

	size_t serialize(char* buffer, size_t size) const {
		if (size <= BROADCAST_SOCKET_BUFFER_SIZE) {
			for (size_t char_i = 0; char_i < _json_length; ++char_i) {
				buffer[char_i] = _json_payload[char_i];
			}
			return _json_length;
		}
		return 0;
	}

	bool compare(const char* buffer, size_t size) const {
		if (size <= BROADCAST_SOCKET_BUFFER_SIZE) {
			for (size_t char_i = 0; char_i < _json_length; ++char_i) {
				if (buffer[char_i] != _json_payload[char_i]) {
					return false;
				}
			}
		}
		return true;
	}

	bool has_key(char key) const {
		if (_json_length > 6) {	// 6 because {"k":x} meaning 7 of length minumum
			for (size_t char_i = 4; char_i < _json_length; ++char_i) {	// 4 because it's the shortest position possible for ':'
				if (_json_payload[char_i] == ':' && _json_payload[char_i - 2] == key && _json_payload[char_i - 3] == '"' && _json_payload[char_i - 1] == '"') {
					return true;
				}
			}
		}
		return false;
	}

	size_t key_position(char key) const {
		if (_json_length > 6) {	// 6 because {"k":x} meaning 7 of length minumum
			for (size_t char_i = 4; char_i < _json_length; ++char_i) {	// 4 because it's the shortest position possible for ':'
				if (_json_payload[char_i] == ':' && _json_payload[char_i - 2] == key && _json_payload[char_i - 3] == '"' && _json_payload[char_i - 1] == '"') {
					return char_i;
				}
			}
		}
		return 0;
	}

	ValueType value_type(char key) const {
		size_t position = key_position(key);
		if (position) {
			if (_json_payload[++position] == '"') {
				return STRING;
			} else {
				while (_json_payload[position] != ',') {
					if (_json_payload[position] > '9' || _json_payload[position] < '0') {
						return OTHER;
					}
					position++;
				}
				return INTEGER;
			}
		}
		return VOID;
	}

	bool validate_fields() const {
		if (value_type('m') != INTEGER) return false;
		if (value_type('c') != INTEGER) return false;
		if (value_type('i') != INTEGER) return false;
		if (value_type('f') != STRING) return false;
		return true;
	}

};



#endif // JSON_MESSAGE_HPP
