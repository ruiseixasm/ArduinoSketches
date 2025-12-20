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

	JsonMessage() {
		// Does nothing
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

	bool deserialize(const char* in_string, size_t length) {
		if (length <= BROADCAST_SOCKET_BUFFER_SIZE) {
			for (size_t char_j = 0; char_j < length; ++char_j) {
				_json_payload[char_j] = in_string[char_j];
			}
			_json_length = length;
			return true;
		}
		return false;
	}

	size_t serialize(char* in_string, size_t size) const {
		if (size >= _json_length) {
			for (size_t json_i = 0; json_i < _json_length; ++json_i) {
				in_string[json_i] = _json_payload[json_i];
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

	bool has_key(char key) const {
		if (_json_length > 6) {	// 6 because {"k":x} meaning 7 of length minumum
			for (size_t json_i = 4; json_i < _json_length; ++json_i) {	// 4 because it's the shortest position possible for ':'
				if (_json_payload[json_i] == ':' && _json_payload[json_i - 2] == key && _json_payload[json_i - 3] == '"' && _json_payload[json_i - 1] == '"') {
					return true;
				}
			}
		}
		return false;
	}

	size_t key_position(char key) const {
		if (_json_length > 6) {	// 6 because {"k":x} meaning 7 of length minumum (> 6)
			for (size_t json_i = 4; json_i < _json_length; ++json_i) {	// 4 because it's the shortest position possible for ':'
				if (_json_payload[json_i] == ':' && _json_payload[json_i - 2] == key && _json_payload[json_i - 3] == '"' && _json_payload[json_i - 1] == '"') {
					return json_i + 1;	// Moves 1 after the ':' char (avoids extra thinking)
				}
			}
		}
		return 0;
	}

	ValueType value_type(char key) const {
		size_t json_i = key_position(key);
		if (json_i) {
			if (_json_payload[json_i] == '"') {
				return STRING;
			} else {
				while (_json_payload[json_i] != ',' && _json_payload[json_i] != '}') {
					if (_json_payload[json_i] > '9' || _json_payload[json_i] < '0') {
						return OTHER;
					}
					json_i++;
				}
				if (_json_payload[json_i - 1] == ':') {
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
		if (value_type('m') != INTEGER) return false;
		if (value_type('i') != INTEGER) return false;
		if (value_type('c') != INTEGER) return false;
		if (value_type('f') != STRING) return false;
		return true;
	}

	// EXTRACTORS

	MessageValue extract_message_value() const {
		size_t json_i = key_position(MessageKey::MESSAGE);
		if (json_i) {
			char number_char = _json_payload[json_i];
			if (number_char > '9' || number_char < '0') return MessageValue::NOISE;
			if (_json_payload[json_i + 1] != ',' && _json_payload[json_i + 1] != '}') return MessageValue::NOISE;
			return static_cast<MessageValue>(number_char - '0');
		}
		return MessageValue::NOISE;
	}

	uint16_t extract_identity() const {
		uint16_t identity = 0;
		size_t json_i = key_position(MessageKey::IDENTITY);
		if (json_i) {
			while (json_i < _json_length && !(_json_payload[json_i] > '9' || _json_payload[json_i] < '0')) {
				identity *= 10;
				identity += _json_payload[json_i++] - '0';
			}
		}
		return identity;
	}

	uint16_t extract_checksum() const {
		uint16_t checksum = 0;
		size_t json_i = key_position(MessageKey::CHECKSUM);
		if (json_i) {
			while (json_i < _json_length && !(_json_payload[json_i] > '9' || _json_payload[json_i] < '0')) {
				checksum *= 10;
				checksum += _json_payload[json_i++] - '0';
			}
		}
		return checksum;
	}

	uint32_t extract_number(char key) const {
		uint32_t json_number = 0;
		size_t json_i = key_position(key);
		if (json_i) {
			while (json_i < _json_length && !(_json_payload[json_i] > '9' || _json_payload[json_i] < '0')) {
				json_number *= 10;
				json_number += _json_payload[json_i++] - '0';
			}
		}
		return json_number;
	}

	bool extract_string(char key, char* out_string, size_t size) const {
		size_t json_i = key_position(key);
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

	// REMOVES

	void remove_field(char key) {
		size_t json_i = key_position(key);
		if (json_i) {
			

		}
	}

	// RESETS

	bool reset_field(char key) {

		return false;
	}

	// INSERTERS

	bool insert_message_value(MessageValue message_value) {
		size_t json_i = key_position('m');
		

		return false;
	}

	

};



#endif // JSON_MESSAGE_HPP
