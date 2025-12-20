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

// Guaranteed memory safety, constrained / schema-driven JSON protocol
// Advisable maximum sizes:
// 		f (from / name) → 16 bytes (15 + '\0')
// 		d (description) → 64 bytes (63 + '\0')


#ifndef BROADCAST_SOCKET_BUFFER_SIZE
#define BROADCAST_SOCKET_BUFFER_SIZE 128
#endif



using MessageKey = TalkieCodes::MessageKey;
using MessageValue = TalkieCodes::MessageValue;


class JsonMessage {
public:
	
	enum ValueType : uint8_t {
		STRING, INTEGER, OTHER, VOID
	};


protected:

	char _json_payload[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};
	uint8_t _json_length = 0;	// BROADCAST_SOCKET_BUFFER_SIZE <= uint8_t


public:

    virtual const char* class_name() const { return "JsonMessage"; }

	void reset() {
		// Only static guarantees it won't live on the stack!
		static const char default_payload[] = "{\"m\":0,\"i\":0,\"c\":0,\"f\":\"\"}";
		size_t default_length = sizeof(default_payload) - 1;
		if (default_length <= BROADCAST_SOCKET_BUFFER_SIZE) {
			for (size_t char_j = 0; char_j < default_length; char_j++) {
				_json_payload[char_j] = default_payload[char_j];
			}
			_json_length = default_length;
		}
	}

	JsonMessage() {
		reset();	// Initiate with the minimum
	}

	~JsonMessage() {
		// Does nothing
	}


	size_t get_length() const {
		return _json_length;
	}

    uint16_t getChecksum() {	// 16-bit word and XORing
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
			for (size_t char_j = 0; char_j < length; ++char_j) {
				_json_payload[char_j] = buffer[char_j];
			}
			_json_length = length;
			return true;
		}
		return false;
	}

	bool deserialize_string(const char* in_string) {
		size_t char_j = 0;
		while (char_j < BROADCAST_SOCKET_BUFFER_SIZE) {
			_json_payload[char_j] = in_string[char_j];
			char_j++;
		}
		if (in_string[char_j] == '\0') {
			_json_length = char_j;
		} else {
			reset();	// sets the length too
			return false;
		}
		return true;
	}

	size_t serialize(char* buffer, size_t size) const {
		if (size >= _json_length) {
			for (size_t json_i = 0; json_i < _json_length; ++json_i) {
				buffer[json_i] = _json_payload[json_i];
			}
			return _json_length;
		}
		return 0;
	}

	bool compare(const char* in_string, size_t size) const {
		if (size == _json_length) {
			for (size_t char_j = 0; char_j < size; ++char_j) {
				if (in_string[char_j] != _json_payload[char_j]) {
					return false;
				}
			}
			return true;
		}
		return false;
	}

	bool compare_string(const char* in_string) const {
		size_t char_j = 0;
		while (char_j < _json_length) {
			if (in_string[char_j] != _json_payload[char_j]) {
				return false;
			}
			char_j++;
		}
		return in_string[char_j] == '\0';
	}

	bool has_key(char key, size_t json_i = 4) const {
		if (_json_length > 6) {	// 6 because {"k":x} meaning 7 of length minumum
			for (; json_i < _json_length; ++json_i) {	// 4 because it's the shortest position possible for ':'
				if (_json_payload[json_i] == ':' && _json_payload[json_i - 2] == key && _json_payload[json_i - 3] == '"' && _json_payload[json_i - 1] == '"') {
					return true;
				}
			}
		}
		return false;
	}


	size_t value_position(char key, size_t json_i = 4) const {
		if (_json_length > 6) {	// 6 because {"k":x} meaning 7 of length minumum (> 6)
			for (; json_i < _json_length; ++json_i) {	// 4 because it's the shortest position possible for ':'
				if (_json_payload[json_i] == ':' && _json_payload[json_i - 2] == key && _json_payload[json_i - 3] == '"' && _json_payload[json_i - 1] == '"') {
					return json_i + 1;	// Moves 1 after the ':' char (avoids extra thinking)
				}
			}
		}
		return 0;
	}


	size_t key_position(char key, size_t json_i = 4) const {
		json_i = value_position(key, json_i);
		if (json_i) {			//   3210
			return json_i - 3;	// {"k":x}
		}
		return 0;
	}


	size_t get_field_length(char key, size_t json_i = 4) const {
		size_t field_length = 0;
		json_i = value_position(key, json_i);
		if (json_i) {
			field_length = 4;	// All keys occupy 4 '"k":' chars
			ValueType value_type = get_value_type(key, json_i - 1);
			switch (value_type) {

				case ValueType::STRING:
					field_length += 2;	// Adds the two '"' associated to the string
					for (json_i++; json_i < _json_length && _json_payload[json_i] != '"'; json_i++) {
						field_length++;
					}
					break;
				
				case ValueType::INTEGER:
					for (; json_i < _json_length && !(_json_payload[json_i] > '9' || _json_payload[json_i] < '0'); json_i++) {
						field_length++;
					}
					break;
				
				default: break;
			}
		}
		return field_length;
	}


	ValueType get_value_type(char key, size_t json_i = 4) const {
		json_i = value_position(key, json_i);
		if (json_i) {
			if (_json_payload[json_i] == '"') {
				for (json_i++; json_i < _json_length && _json_payload[json_i] != '"'; json_i++) {}
				if (json_i == _json_length) {
					return VOID;
				}
				return STRING;
			} else {
				while (json_i < _json_length && _json_payload[json_i] != ',' && _json_payload[json_i] != '}') {
					if (_json_payload[json_i] > '9' || _json_payload[json_i] < '0') {
						return OTHER;
					}
					json_i++;
				}
				if (json_i == _json_length) {
					return VOID;
				}
				return INTEGER;
			}
		}
		return VOID;
	}

	bool validate_fields() const {
		// Minimum length: '{"m":0,"i":0,"c":0,"f":"n"}' = 27
		if (_json_length < 27) return false;
		if (_json_payload[0] != '{' || _json_payload[_json_length - 1] != '}') return false;	// Note that literals add the '\0'!
		if (get_value_type('m') != INTEGER) return false;
		if (get_number('m') > 9) return false;
		if (get_value_type('i') != INTEGER) return false;
		if (get_value_type('c') != INTEGER) return false;
		if (get_value_type('f') != STRING) return false;
		return true;
	}

	// GETTERS

	uint32_t get_number(char key, size_t json_i = 4) const {
		uint32_t json_number = 0;
		json_i = value_position(key, json_i);
		if (json_i) {
			while (json_i < _json_length && !(_json_payload[json_i] > '9' || _json_payload[json_i] < '0')) {
				json_number *= 10;
				json_number += _json_payload[json_i++] - '0';
			}
		}
		return json_number;
	}

	bool get_string(char key, char* out_string, size_t size, size_t json_i = 4) const {
		json_i = value_position(key, json_i);
		if (json_i && _json_payload[json_i++] == '"' && out_string && size) {	// Safe code
			size_t char_j = 0;
			while (_json_payload[json_i] != '"' && json_i < _json_length && char_j < size) {
				out_string[char_j++] = _json_payload[json_i++];
			}
			if (char_j < size) {
				out_string[char_j] = '\0';	// Makes sure the termination char is added
				return true;
			}
			out_string[0] = '\0';	// Clears all noisy fill if it fails
			return false;
		}
		return false;
	}

	// REMOVERS

	void remove_field(char key, size_t json_i = 4) {
		json_i = key_position(key, json_i);
		if (json_i) {
			

		}
	}

	// SETTERS

	bool swap_key(char old_key, char new_key, size_t json_i = 4) {
		json_i = value_position(old_key, json_i);
		if (json_i) {
			_json_payload[json_i] = new_key;
			return true;
		}
		return false;
	}

	bool set_field(char key) {

		return false;
	}

	

};



#endif // JSON_MESSAGE_HPP
