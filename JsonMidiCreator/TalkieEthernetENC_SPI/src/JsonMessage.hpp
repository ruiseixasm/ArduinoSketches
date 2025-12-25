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
#ifndef NAME_LEN
#define NAME_LEN 16
#endif
#ifndef MAX_LEN
#define MAX_LEN 64
#endif



using MessageKey = TalkieCodes::MessageKey;
using SourceValue = TalkieCodes::SourceValue;
using MessageValue = TalkieCodes::MessageValue;
using RogerValue = TalkieCodes::RogerValue;
using InfoValue = TalkieCodes::InfoValue;


class JsonMessage {
public:
	
	enum ValueType : uint8_t {
		STRING, INTEGER, OTHER, VOID
	};


protected:

	char _json_payload[BROADCAST_SOCKET_BUFFER_SIZE];	// Length already managed, no need to zero it
	size_t _json_length = 0;
	// If multiple messages may be read at once (or in ISR context, multi-core ESP32, etc.), keep it per-instance to avoid overwriting / race conditions.
    mutable char _temp_string[MAX_LEN];  // mutable allows const methods to modify it (non static for the reasons above)


	static size_t number_of_digits(uint32_t number) {
		size_t length = 1;	// 0 has 1 digit
		while (number > 9) {
			number /= 10;
			length++;
		}
		return length;
	}


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

	
	size_t get_colon_position(char key, size_t colon_position = 4) const {
		if (_json_length > 6) {	// 6 because {"k":x} meaning 7 of length minumum (> 6)
			for (size_t json_i = colon_position; json_i < _json_length; ++json_i) {	// 4 because it's the shortest position possible for ':'
				if (_json_payload[json_i] == ':' && _json_payload[json_i - 2] == key && _json_payload[json_i - 3] == '"' && _json_payload[json_i - 1] == '"') {
					return json_i;
				}
			}
		}
		return 0;
	}


	size_t get_value_position(char key, size_t colon_position = 4) const {
		colon_position = get_colon_position(key, colon_position);
		if (colon_position) {			//     01
			return colon_position + 1;	// {"k":x}
		}
		return 0;
	}


	size_t get_key_position(char key, size_t colon_position = 4) const {
		colon_position = get_colon_position(key, colon_position);
		if (colon_position) {			//   210
			return colon_position - 2;	// {"k":x}
		}
		return 0;
	}


	size_t get_field_length(char key, size_t colon_position = 4) const {
		size_t field_length = 0;
		size_t json_i = get_value_position(key, colon_position);
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


	// When a function receives a buffer and its size, the size must include space for the '\0'
	bool get_string(char key, char* buffer, size_t size, size_t colon_position = 4) const {
		if (buffer && size) {
			size_t json_i = get_value_position(key, colon_position);
			if (json_i && _json_payload[json_i++] == '"' && buffer && size) {	// Safe code
				size_t char_j = 0;
				while (_json_payload[json_i] != '"' && json_i < _json_length && char_j < size) {
					buffer[char_j++] = _json_payload[json_i++];
				}
				if (char_j < size) {
					buffer[char_j] = '\0';	// Makes sure the termination char is added
					return true;
				}
				buffer[0] = '\0';	// Clears all noisy fill if it fails
				return false;
			}
		}
		return false;
	}


	bool set_number(char key, uint32_t number, size_t colon_position = 4) {
		colon_position = get_colon_position(key, colon_position);
		if (colon_position) {
			if (!remove(key, colon_position)) return false;
		}
		// At this time there is no field key for sure, so, one can just add it right before the '}'
		size_t number_size = number_of_digits(number);
		size_t new_length = _json_length + number_size + 4 + 1;	// the usual key 4 plus the + 1 due to the ',' needed to be added
		if (new_length > BROADCAST_SOCKET_BUFFER_SIZE) {
			return false;
		}
		// Sets the key json data
		char json_key[] = ",\"k\":";
		json_key[2] = key;
		if (_json_length > 2) {
			for (size_t char_j = 0; char_j < 5; char_j++) {
				_json_payload[_json_length - 1 + char_j] = json_key[char_j];
			}
		} else if (_json_length == 2) {	// Edge case of '{}'
			new_length--;	// Has to remove the extra ',' considered above
			for (size_t char_j = 1; char_j < 5; char_j++) {
				_json_payload[_json_length - 1 + char_j - 1] = json_key[char_j];
			}
		} else {
			reset();	// Something very wrong, needs to be reset
			return false;
		}
		if (number) {
			// To be added, it has to be from right to left
			for (size_t json_i = new_length - 2; number; json_i--) {
				_json_payload[json_i] = '0' + number % 10;
				number /= 10; // Truncates the number (does a floor)
			}
		} else {	// Regardless being 0, it also has to be added
			_json_payload[new_length - 2] = '0';
		}
		// Finally writes the last char '}'
		_json_payload[new_length - 1] = '}';
		_json_length = new_length;
		return true;
	}

	bool set_string(char key, const char* in_string, size_t colon_position = 4) {
		if (in_string) {
			size_t length = 0;
			for (size_t char_j = 0; in_string[char_j] != '\0' && char_j < BROADCAST_SOCKET_BUFFER_SIZE; char_j++) {
				length++;
			}
			if (length) {
				colon_position = get_colon_position(key, colon_position);
				if (colon_position) {
					if (!remove(key, colon_position)) return false;
				}
				// the usual key + 4 plus + 2 for both '"' and the + 1 due to the heading ',' needed to be added
				size_t new_length = _json_length + length + 4 + 2 + 1;
				if (new_length > BROADCAST_SOCKET_BUFFER_SIZE) {
					return false;
				}
				// Sets the key json data
				char json_key[] = ",\"k\":";
				json_key[2] = key;
				// length to position requires - 1 and + 5 for the key (at '}' position + 5)
				size_t setting_position = _json_length - 1 + 5;
				if (_json_length > 2) {
					for (size_t char_j = 0; char_j < 5; char_j++) {
						_json_payload[_json_length - 1 + char_j] = json_key[char_j];
					}
				} else if (_json_length == 2) {	// Edge case of '{}'
					new_length--;	// Has to remove the extra ',' considered above
					setting_position--;
					for (size_t char_j = 1; char_j < 5; char_j++) {
						_json_payload[_json_length - 1 + char_j - 1] = json_key[char_j];
					}
				} else {
					reset();	// Something very wrong, needs to be reset
					return false;
				}
				// Adds the first char '"'
				_json_payload[setting_position++] = '"';
				// To be added, it has to be from right to left
				for (size_t char_j = 0; char_j < length; char_j++) {
					_json_payload[setting_position++] = in_string[char_j];
				}
				// Adds the second char '"'
				_json_payload[setting_position++] = '"';
				// Finally writes the last char '}'
				_json_payload[setting_position++] = '}';
				_json_length = new_length;
				return true;
			}
		}
		return false;
	}



public:

    virtual const char* class_name() const { return "JsonMessage"; }

	JsonMessage() {
		reset();	// Initiate with the minimum
	}

	JsonMessage(const char* buffer, size_t length) {
		if (!deserialize_buffer(buffer, length)) {
			reset();
		}
	}

	JsonMessage(const JsonMessage& other) {
		_json_length = other._json_length;
		for (size_t json_i = 0; json_i < _json_length; ++json_i) {
			_json_payload[json_i] = other._json_payload[json_i];
		}
	}

	~JsonMessage() {
		// Does nothing
	}

	bool operator==(const JsonMessage& other) const {
		if (_json_length == other._json_length) {
			for (size_t json_i = 0; json_i < _json_length; ++json_i) {
				if (_json_payload[json_i] != other._json_payload[json_i]) {
					return false;
				}
			}
			return true;
		}
		return false;
	}

	bool operator!=(const JsonMessage& other) const {
		return !(*this == other);
	}


	size_t get_length() const {
		return _json_length;
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

    uint16_t generate_checksum() {	// 16-bit word and XORing
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


    uint16_t extract_checksum() {
        uint16_t data_checksum = 0;
        bool at_c = false;
        size_t json_i = 4;	// Optimized {"c": ...
        for (size_t char_j = json_i; char_j < _json_length; ++char_j) {
            if (_json_payload[char_j] == ':') {
                if (_json_payload[char_j - 2] == 'c' && _json_payload[char_j - 3] == '"' && _json_payload[char_j - 1] == '"') {
                    at_c = true;
                }
            } else if (at_c) {
				if (_json_payload[char_j] < '0' || _json_payload[char_j] > '9') {
					at_c = false;
				} else if (_json_payload[char_j - 1] == ':') { // First number in the row
					data_checksum = _json_payload[char_j] - '0';
					_json_payload[char_j] = '0';
				} else {
					data_checksum *= 10;
					data_checksum += _json_payload[char_j] - '0';
					continue;   // Avoids the copy of the char
				}
            }
            _json_payload[json_i++] = _json_payload[char_j]; // Does a left offset
        }
        _json_length = json_i;
        return data_checksum;
    }


    bool insert_checksum() {
		if (!set_number('c', 0)) return false;
        uint16_t checksum = generate_checksum();
        if (checksum > 0) { // Isn't already 0

            size_t num_digits = number_of_digits(checksum);
            size_t char_j = _json_length - 1;
			size_t new_length = _json_length + num_digits - 1;	// Discount the digit '0' already placed
            
            if (new_length > BROADCAST_SOCKET_BUFFER_SIZE) return false;	// buffer overflow
			_json_length = new_length;

			bool at_c = false;
            for (size_t json_i = new_length - 1; char_j > 4; json_i--) {
                if (_json_payload[char_j - 2] == ':') {	// Must find it at 5 the least (> 4)
                    if (_json_payload[char_j - 4] == 'c' && _json_payload[char_j - 5] == '"' && _json_payload[char_j - 3] == '"') {
                        at_c = true;
                    }
                } else if (at_c) {
                    if (checksum == 0) {
                        return true;
                    } else {
                        _json_payload[json_i] = '0' + checksum % 10;
                        checksum /= 10; // Truncates the number (does a floor)
                        continue;       // Avoids the copy of the char
                    }
                }
                _json_payload[json_i] = _json_payload[char_j--]; // Does an offset (NOTE the continue above)
            }
        }
        return true;
    }


	bool validate_checksum() {
		const uint16_t message_checksum = extract_checksum();
		const uint16_t checksum = generate_checksum();
		return message_checksum == checksum;
	}

	bool deserialize_buffer(const char* buffer, size_t length) {
		if (buffer && length && length <= BROADCAST_SOCKET_BUFFER_SIZE) {
			for (size_t char_j = 0; char_j < length; ++char_j) {
				_json_payload[char_j] = buffer[char_j];
			}
			_json_length = length;
			return true;
		}
		return false;
	}

	size_t serialize_json(char* buffer, size_t length) const {
		if (buffer && length >= _json_length) {
			for (size_t json_i = 0; json_i < _json_length; ++json_i) {
				buffer[json_i] = _json_payload[json_i];
			}
			return _json_length;
		}
		return 0;
	}

	bool write_to(Print& out) const {
		if (_json_length) {
			return out.write(reinterpret_cast<const uint8_t*>(_json_payload), _json_length) == _json_length;
		}
		return false;
	}

	bool for_me(const char* name, uint8_t channel) const {
		size_t colon_position = get_colon_position('t');
		if (colon_position) {
			ValueType value_type = get_value_type('t', colon_position);
			switch (value_type) {

				case STRING:
					{
						char message_to[NAME_LEN] = {'\0'};
						get_string('t', message_to, colon_position);
						return strcmp(message_to, name) == 0;
					}
				break;
				
				case INTEGER:
					{
						uint32_t number = get_number('t', colon_position);
						return number == channel;
					}
				break;
				
				default: break;
			}
		}
		return true;	// Non target messages, without to(t), are considered broadcasted messages an for everyone
	}


	bool compare_buffer(const char* buffer, size_t length) const {
		if (length == _json_length) {
			for (size_t char_j = 0; char_j < length; ++char_j) {
				if (buffer[char_j] != _json_payload[char_j]) {
					return false;
				}
			}
			return true;
		}
		return false;
	}


	bool has_key(char key, size_t colon_position = 4) const {
		size_t json_i = get_colon_position(key, colon_position);
		return json_i > 0;
	}

	bool has_identity() const {
		return get_colon_position('i') > 0;
	}

	bool has_from() const {
		return get_colon_position('f') > 0;
	}

	bool has_to() const {
		return get_colon_position('t') > 0;
	}

	bool has_to_name() const {
		size_t colon_position = get_colon_position('t');
		return colon_position 
			&& get_value_type('t', colon_position) == ValueType::STRING;
	}

	bool has_to_channel() const {
		size_t colon_position = get_colon_position('t');
		return colon_position 
			&& get_value_type('t', colon_position) == ValueType::INTEGER;
	}

	bool has_info() const {
		return get_colon_position('s') > 0;
	}

	bool has_nth_value(uint8_t nth) const {
		if (nth < 10) {
			return get_colon_position('0' + nth) > 0;
		}
		return false;
	}

	bool has_nth_value_string(uint8_t nth) const {
		if (nth < 10) {
			char value_key = '0' + nth;
			size_t colon_position = get_colon_position(value_key);
			if (colon_position) {
				return get_value_type(value_key, colon_position) == ValueType::STRING;
			}
		}
		return false;
	}

	bool has_nth_value_number(uint8_t nth) const {
		if (nth < 10) {
			char value_key = '0' + nth;
			size_t colon_position = get_colon_position(value_key);
			if (colon_position) {
				return get_value_type(value_key, colon_position) == ValueType::INTEGER;
			}
		}
		return false;
	}


	bool is_from(const char* name) const {
		const char* from_name = get_from_name();
		if (from_name) {
			return strcmp(name, from_name) == 0;
		}
		return false;
	}

	bool is_to_name(const char* name) const {
		size_t colon_position = get_colon_position('t')
		if (colon_position) {
			ValueType value_type = get_value_type('t', colon_position);
			if (value_type == ValueType::STRING) {
				const char* message_to = get_string('t', _temp_string, NAME_LEN, colon_position);
				return strcmp(message_to, name) == 0;
			}
		}
		return false;
	}

	bool is_to_channel(uint8_t channel) const {
		size_t colon_position = get_colon_position('t');
		return colon_position 
			&& get_value_type('t', colon_position) == ValueType::INTEGER
			&& get_number('t', colon_position) == channel;
	}


	// GETTERS

	ValueType get_value_type(char key, size_t colon_position = 4) const {
		size_t json_i = get_value_position(key, colon_position);
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

	uint32_t get_number(char key, size_t colon_position = 4) const {
		uint32_t json_number = 0;
		size_t json_i = get_value_position(key, colon_position);
		if (json_i) {
			while (json_i < _json_length && !(_json_payload[json_i] > '9' || _json_payload[json_i] < '0')) {
				json_number *= 10;
				json_number += _json_payload[json_i++] - '0';
			}
		}
		return json_number;
	}

	MessageValue get_message_value() const {
		size_t colon_position = get_colon_position('m');
		if (colon_position) {
			uint8_t message_number = get_number('m', colon_position);
			if (message_number < static_cast<uint8_t>( MessageValue::NOISE )) {
				return static_cast<MessageValue>( message_number );
			}
		}
		return MessageValue::NOISE;
	}

	uint16_t get_checksum() {
		return static_cast<uint16_t>(get_number('c'));
	}

	uint16_t get_identity() {
		return static_cast<uint16_t>(get_number('i'));
	}

	uint16_t get_timestamp() {
		return get_identity();
	}

	SourceValue get_source_value() const {
		size_t colon_position = get_colon_position('c');
		if (colon_position) {
			uint8_t source_number = (uint8_t)get_number('c', colon_position);
			if (source_number < static_cast<uint8_t>( SourceValue::NONE )) {
				return static_cast<SourceValue>( source_number );
			}
		}
		return SourceValue::NONE;
	}

	RogerValue get_roger_value() const {
		size_t colon_position = get_colon_position('r');
		if (colon_position) {
			uint8_t roger_number = (uint8_t)get_number('r', colon_position);
			if (roger_number < static_cast<uint8_t>( RogerValue::NIL )) {
				return static_cast<RogerValue>( roger_number );
			}
		}
		return RogerValue::NIL;
	}

	InfoValue get_info_value() const {
		size_t colon_position = get_colon_position('s');
		if (colon_position) {
			uint8_t info_number = (uint8_t)get_number('s', colon_position);
			if (info_number < static_cast<uint8_t>( InfoValue::UNDEFINED )) {
				return static_cast<InfoValue>( info_number );
			}
		}
		return InfoValue::UNDEFINED;
	}

    // New method using internal temporary buffer (_temp_string)
    char* get_from_name() const {
        if (get_string('f', _temp_string, NAME_LEN)) {
            return _temp_string;  // safe C string
        }
        return nullptr;  // failed
    }

	ValueType get_to_type() const {
		return get_value_type('t');
	}

    // New method using internal temporary buffer (_temp_string)
    char* get_to_name() const {
		size_t colon_position = get_colon_position('t');
		if (colon_position && get_value_type('t', colon_position) == ValueType::STRING) {
			if (get_string('t', _temp_string, NAME_LEN, colon_position)) {
				return _temp_string;
			}
		}
        return nullptr;  // failed
    }
	
	uint8_t get_to_channel() const {
		size_t colon_position = get_colon_position('t');
		if (colon_position && get_value_type('t', colon_position) == ValueType::INTEGER) {
			return (uint8_t)get_number('t', colon_position);
		}
		return 255;	// Means, no chanel
	}


	ValueType get_nth_value_type(uint8_t nth) {
		if (nth < 10) {
			return get_value_type('0' + nth);
		}
		return ValueType::VOID;
	}

	char* get_nth_value_string(uint8_t nth) const {
		if (nth < 10 && get_string('0' + nth, _temp_string, MAX_LEN)) {
			return _temp_string;  // safe C string
		}
		return nullptr;  // failed
	}

	uint32_t get_nth_value_number(uint8_t nth) const {
		if (nth < 10) {
			return get_number('0' + nth);
		}
		return false;
	}

	char* get_action_string() const {
		if (get_string('a', _temp_string, NAME_LEN)) {
			return _temp_string;  // safe C string
		}
		return nullptr;  // failed
	}

	uint32_t get_action_number() const {
		return get_number('a');
	}


	// REMOVERS

	bool remove(char key, size_t colon_position = 4) {
		colon_position = get_colon_position(key, colon_position);
		if (colon_position) {
			size_t field_position = colon_position - 3;	// All keys occupy 3 '"k":' chars to the left of the colon
			size_t field_length = get_field_length(key, colon_position);	// Excludes possible heading ',' separation comma
			if (_json_payload[field_position - 1] == ',') {	// the heading ',' has to be removed too
				field_position--;
				field_length++;
			} else if (_json_payload[field_position + field_length] == ',') {
				field_length++;	// Changes the length only, to pick up the tailing ','
			}
			for (size_t json_i = field_position; json_i < _json_length - field_length; json_i++) {
                _json_payload[json_i] = _json_payload[json_i + field_length];
            }
			_json_length -= field_length;	// Finally updates the json_payload full length
			return true;
		}
		return false;
	}

	bool remove_message() {
		return remove('m');
	}

	bool remove_from() {
		return remove('f');
	}

	bool remove_to() {
		return remove('t');
	}

	bool remove_identity() {
		return remove('i');
	}

	bool remove_timestamp() {
		return remove('i');
	}

	bool remove_checksum() {
		return remove('c');
	}

	bool remove_source_value() {
		return remove('c');
	}

	bool remove_roger_value() {
		return remove('r');
	}

	bool remove_info_value() {
		return remove('s');
	}

	bool remove_nth_value(uint8_t nth) {
		if (nth < 10) {
			return remove('0' + nth);
		}
		return false;
	}

	// SETTERS


	bool set_message(MessageValue message_value) {
		size_t value_position = get_value_position('m');
		if (value_position) {
			_json_payload[value_position] = '0' + static_cast<uint8_t>(message_value);
			return true;
		}
		return false;
	}

	bool set_identity(uint16_t identity) {
		return set_number('i', identity);
	}

	bool set_identity() {
		uint16_t identity = (uint16_t)millis();
		return set_number('i', identity);
	}

	bool set_timestamp(uint16_t timestamp) {
		return set_number('i', timestamp);
	}

	bool set_timestamp() {
		return set_identity();
	}

	bool set_checksum() {
		if (!set_number('c', 0)) return false;	// Resets 'c' to 0
		const uint16_t checksum = generate_checksum();
		// Finally inserts the Checksum
		return set_number('c', checksum);
	}

	bool set_from_name(const char* name) {
		return set_string('f', name);
	}

	bool set_to_name(const char* name) {
		return set_string('t', name);
	}

	bool set_source_value(SourceValue source_value) {
		size_t value_position = get_value_position('c');
		if (value_position) {
			_json_payload[value_position] = '0' + static_cast<uint8_t>(source_value);
			return true;
		}
		return set_number('c', static_cast<uint8_t>(source_value));
	}

	bool set_roger_value(RogerValue roger_value) {
		size_t value_position = get_value_position('r');
		if (value_position) {
			_json_payload[value_position] = '0' + static_cast<uint8_t>(roger_value);
			return true;
		}
		return set_number('r', static_cast<uint8_t>(roger_value));
	}

	bool set_info_value(InfoValue info_value) {
		size_t value_position = get_value_position('s');
		if (value_position) {
			_json_payload[value_position] = '0' + static_cast<uint8_t>(info_value);
			return true;
		}
		return set_number('s', static_cast<uint8_t>(info_value));
	}

	bool set_nth_value_number(uint8_t nth, uint32_t number) {
		if (nth < 10) {
			return set_number('0' + nth, number);
		}
		return false;
	}

	bool set_nth_value_string(uint8_t nth, const char* in_string) {
		if (nth < 10) {
			return set_string('0' + nth, in_string);
		}
		return false;
	}

	bool swap_from_with_to() {
		size_t key_from_position = get_key_position('f');
		size_t key_to_position = get_key_position('t');
		if (key_from_position) {
			if (key_to_position) {
				_json_payload[key_from_position] = 't';
				_json_payload[key_to_position] = 'f';
			} else {
				_json_payload[key_from_position] = 't';
			}
			return true;
		}
		return false;
	}

};



#endif // JSON_MESSAGE_HPP
