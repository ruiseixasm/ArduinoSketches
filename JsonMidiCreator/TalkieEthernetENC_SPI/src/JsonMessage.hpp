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
using BroadcastValue = TalkieCodes::BroadcastValue;
using MessageValue = TalkieCodes::MessageValue;
using RogerValue = TalkieCodes::RogerValue;
using InfoValue = TalkieCodes::InfoValue;
using TalkerMatch = TalkieCodes::TalkerMatch;

class BroadcastSocket;

class JsonMessage {
public:
	
	enum ValueType : uint8_t {
		STRING, INTEGER, OTHER, VOID
	};



	// READ ONLY METHODS

	static size_t number_of_digits(uint32_t number) {
		size_t length = 1;	// 0 has 1 digit
		while (number > 9) {
			number /= 10;
			length++;
		}
		return length;
	}


	static size_t get_colon_position(char key, const char* json_payload, size_t json_length, size_t colon_position = 4) {
		for (size_t json_i = colon_position; json_i < json_length; ++json_i) {	// 4 because it's the shortest position possible for ':'
			if (json_payload[json_i] == ':' && json_payload[json_i - 2] == key && json_payload[json_i - 3] == '"' && json_payload[json_i - 1] == '"') {
				return json_i;
			}
		}
		return 0;
	}


	static size_t get_value_position(char key, const char* json_payload, size_t json_length, size_t colon_position = 4) {
		colon_position = get_colon_position(key, json_payload, json_length, colon_position);
		if (colon_position) {			//     01
			return colon_position + 1;	// {"k":x}
		}
		return 0;
	}



	static size_t get_key_position(char key, const char* json_payload, size_t json_length, size_t colon_position = 4) {
		colon_position = get_colon_position(key, json_payload, json_length, colon_position);
		if (colon_position) {			//   210
			return colon_position - 2;	// {"k":x}
		}
		return 0;
	}


	static size_t get_field_length(char key, const char* json_payload, size_t json_length, size_t colon_position = 4) {
		size_t field_length = 0;
		size_t json_i = get_value_position(key, json_payload, json_length, colon_position);
		if (json_i) {
			field_length = 4;	// All keys occupy 4 '"k":' chars
			ValueType value_type = get_value_type(key, json_payload, json_length, json_i - 1);
			switch (value_type) {

				case ValueType::STRING:
					field_length += 2;	// Adds the two '"' associated to the string
					for (json_i++; json_i < json_length && json_payload[json_i] != '"'; json_i++) {
						field_length++;
					}
					break;
				
				case ValueType::INTEGER:
					for (; json_i < json_length && !(json_payload[json_i] > '9' || json_payload[json_i] < '0'); json_i++) {
						field_length++;
					}
					break;
				
				default: break;
			}
		}
		return field_length;
	}


	static ValueType get_value_type(char key, const char* json_payload, size_t json_length, size_t colon_position = 4) {
		size_t json_i = get_value_position(key, json_payload, json_length, colon_position);
		if (json_i) {
			if (json_payload[json_i] == '"') {
				for (json_i++; json_i < json_length && json_payload[json_i] != '"'; json_i++) {}
				if (json_i == json_length) {
					return VOID;
				}
				return STRING;
			} else {
				while (json_i < json_length && json_payload[json_i] != ',' && json_payload[json_i] != '}') {
					if (json_payload[json_i] > '9' || json_payload[json_i] < '0') {
						return OTHER;
					}
					json_i++;
				}
				if (json_i == json_length) {
					return VOID;
				}
				return INTEGER;
			}
		}
		return VOID;
	}


	// When a function receives a buffer and its size, the size must include space for the '\0'
	static bool get_value_string(char key, char* buffer, size_t size, const char* json_payload, size_t json_length, size_t colon_position = 4) {
		if (buffer && size) {
			size_t json_i = get_value_position(key, json_payload, json_length, colon_position);
			if (json_i && json_payload[json_i++] == '"' && buffer && size) {	// Safe code
				size_t char_j = 0;
				while (json_payload[json_i] != '"' && json_i < json_length && char_j < size) {
					buffer[char_j++] = json_payload[json_i++];
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


	static uint32_t get_value_number(char key, const char* json_payload, size_t json_length, size_t colon_position = 4) {
		uint32_t json_number = 0;
		size_t json_i = get_value_position(key, json_payload, json_length, colon_position);
		if (json_i) {
			while (json_i < json_length && !(json_payload[json_i] > '9' || json_payload[json_i] < '0')) {
				json_number *= 10;
				json_number += json_payload[json_i++] - '0';
			}
		}
		return json_number;
	}


	// WRITE METHODS

	static void reset(char* json_payload, size_t *json_length) {
		// Only static guarantees it won't live on the stack!
		static const char default_payload[] = "{\"m\":0,\"b\":0,\"i\":0,\"f\":\"\"}";
		size_t default_length = sizeof(default_payload) - 1;
		if (default_length <= BROADCAST_SOCKET_BUFFER_SIZE) {
			for (size_t char_j = 0; char_j < default_length; char_j++) {
				json_payload[char_j] = default_payload[char_j];
			}
			*json_length = default_length;
		}
	}


	static bool remove(char key, char* json_payload, size_t *json_length, size_t colon_position = 4) {
		colon_position = get_colon_position(key, json_payload, *json_length, colon_position);
		if (colon_position) {
			size_t field_position = colon_position - 3;	// All keys occupy 3 '"k":' chars to the left of the colon
			size_t field_length = get_field_length(key, json_payload, *json_length, colon_position);	// Excludes possible heading ',' separation comma
			if (json_payload[field_position - 1] == ',') {	// the heading ',' has to be removed too
				field_position--;
				field_length++;
			} else if (json_payload[field_position + field_length] == ',') {
				field_length++;	// Changes the length only, to pick up the tailing ','
			}
			for (size_t json_i = field_position; json_i < *json_length - field_length; json_i++) {
                json_payload[json_i] = json_payload[json_i + field_length];
            }
			*json_length -= field_length;	// Finally updates the json_payload full length
			return true;
		}
		return false;
	}


	static bool set_number(char key, uint32_t number, char* json_payload, size_t *json_length, size_t colon_position = 4) {
		colon_position = get_colon_position(key, json_payload, *json_length, colon_position);
		if (colon_position) {
			if (!remove(key, json_payload, json_length, colon_position)) return false;
		}
		// At this time there is no field key for sure, so, one can just add it right before the '}'
		size_t number_size = number_of_digits(number);
		size_t new_length = *json_length + number_size + 4 + 1;	// the usual key 4 plus the + 1 due to the ',' needed to be added
		if (new_length > BROADCAST_SOCKET_BUFFER_SIZE) {
			return false;
		}
		// Sets the key json data
		char json_key[] = ",\"k\":";
		json_key[2] = key;
		if (*json_length > 2) {
			for (size_t char_j = 0; char_j < 5; char_j++) {
				json_payload[*json_length - 1 + char_j] = json_key[char_j];
			}
		} else if (*json_length == 2) {	// Edge case of '{}'
			new_length--;	// Has to remove the extra ',' considered above
			for (size_t char_j = 1; char_j < 5; char_j++) {
				json_payload[*json_length - 1 + char_j - 1] = json_key[char_j];
			}
		} else {
			reset(json_payload, json_length);	// Something very wrong, needs to be reset
			return false;
		}
		if (number) {
			// To be added, it has to be from right to left
			for (size_t json_i = new_length - 2; number; json_i--) {
				json_payload[json_i] = '0' + number % 10;
				number /= 10; // Truncates the number (does a floor)
			}
		} else {	// Regardless being 0, it also has to be added
			json_payload[new_length - 2] = '0';
		}
		// Finally writes the last char '}'
		json_payload[new_length - 1] = '}';
		*json_length = new_length;
		return true;
	}


	static bool set_string(char key, const char* in_string, char* json_payload, size_t *json_length, size_t colon_position = 4) {
		if (in_string) {
			size_t length = 0;
			for (size_t char_j = 0; in_string[char_j] != '\0' && char_j < BROADCAST_SOCKET_BUFFER_SIZE; char_j++) {
				length++;
			}
			if (length) {
				colon_position = get_colon_position(key, json_payload, *json_length, colon_position);
				if (colon_position) {
					if (!remove(key, json_payload, json_length, colon_position)) return false;
				}
				// the usual key + 4 plus + 2 for both '"' and the + 1 due to the heading ',' needed to be added
				size_t new_length = *json_length + length + 4 + 2 + 1;
				if (new_length > BROADCAST_SOCKET_BUFFER_SIZE) {
					return false;
				}
				// Sets the key json data
				char json_key[] = ",\"k\":";
				json_key[2] = key;
				// length to position requires - 1 and + 5 for the key (at '}' position + 5)
				size_t setting_position = *json_length - 1 + 5;
				if (*json_length > 2) {
					for (size_t char_j = 0; char_j < 5; char_j++) {
						json_payload[*json_length - 1 + char_j] = json_key[char_j];
					}
				} else if (*json_length == 2) {	// Edge case of '{}'
					new_length--;	// Has to remove the extra ',' considered above
					setting_position--;
					for (size_t char_j = 1; char_j < 5; char_j++) {
						json_payload[*json_length - 1 + char_j - 1] = json_key[char_j];
					}
				} else {
					reset(json_payload, json_length);	// Something very wrong, needs to be reset
					return false;
				}
				// Adds the first char '"'
				json_payload[setting_position++] = '"';
				// To be added, it has to be from right to left
				for (size_t char_j = 0; char_j < length; char_j++) {
					json_payload[setting_position++] = in_string[char_j];
				}
				// Adds the second char '"'
				json_payload[setting_position++] = '"';
				// Finally writes the last char '}'
				json_payload[setting_position++] = '}';
				*json_length = new_length;
				return true;
			}
		}
		return false;
	}


protected:

	char _json_payload[BROADCAST_SOCKET_BUFFER_SIZE];	// Length already managed, no need to zero it
	size_t _json_length = 0;
	// If multiple messages may be read at once (or in ISR context, multi-core ESP32, etc.), keep it per-instance to avoid overwriting / race conditions.
    mutable char _temp_string[MAX_LEN];  // mutable allows const methods to modify it (non static for the reasons above)


public:

    virtual const char* class_name() const { return "JsonMessage"; }

	JsonMessage() {
		reset(_json_payload, &_json_length);	// Initiate with the minimum
	}

	JsonMessage(const char* buffer, size_t length) {
		if (!deserialize_buffer(buffer, length)) {
			reset(_json_payload, &_json_length);
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

    JsonMessage& operator=(const JsonMessage& other) {
        if (this == &other) return *this;

        _json_length = other._json_length;
        for (size_t i = 0; i < _json_length; ++i) {
            _json_payload[i] = other._json_payload[i];
        }
        return *this;
    }


	size_t get_length() const {
		return _json_length;
	}


	void reset() {
		reset(_json_payload, &_json_length);
	}

	bool validate_fields() const {
		// Minimum length: '{"m":0,"b":0,"i":0,"f":"n"}' = 27
		if (_json_length < 27) return false;
		if (_json_payload[0] != '{' || _json_payload[_json_length - 1] != '}') return false;	// Note that literals add the '\0'!

		bool found_m = false;
		bool found_b = false;
		bool found_i = false;
		bool found_f = false;
		for (size_t json_i = 4; json_i < _json_length; ++json_i) {	// 4 because it's the shortest position possible for ':'
			if (_json_payload[json_i] == ':' && _json_payload[json_i - 3] == '"' && _json_payload[json_i - 1] == '"') {

				switch (_json_payload[json_i - 2]) {

					case 'm':
					{
						ValueType value_type = get_value_type('m', _json_payload, _json_length, json_i);
						if (value_type == INTEGER) {
							if (_json_payload[json_i + 2] == ',' || _json_payload[json_i + 2] == '}') {
								if (found_b && found_i && found_f) return true;
								found_m = true;
							} else {
								return false;
							}
						} else {
							return false;
						}
					}
					break;
					
					case 'b':
					{
						ValueType value_type = get_value_type('b', _json_payload, _json_length, json_i);
						if (value_type == INTEGER) {
							if (_json_payload[json_i + 2] == ',' || _json_payload[json_i + 2] == '}') {
								if (found_m && found_i && found_f) return true;
								found_b = true;
							} else {
								return false;
							}
						} else {
							return false;
						}
					}
					break;
					
					case 'i':
					{
						ValueType value_type = get_value_type('i', _json_payload, _json_length, json_i);
						if (value_type == INTEGER) {
							if (found_m && found_b && found_f) return true;
							found_i = true;
						} else {
							return false;
						}
					}
					break;
					
					case 'f':
					{
						ValueType value_type = get_value_type('f', _json_payload, _json_length, json_i);
						if (value_type == STRING) {
							if (found_m && found_b && found_i) return true;
							found_f = true;
						} else {
							return false;
						}
					}
					break;
					
					default: break;
				}
			}
		}

		return found_m && found_b && found_i && found_f;
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

	size_t serialize_json(char* buffer, size_t size) const {
		if (buffer && size >= _json_length) {
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
		size_t colon_position = get_colon_position('t', _json_payload, _json_length);
		if (colon_position) {
			ValueType value_type = get_value_type('t', _json_payload, _json_length, colon_position);
			switch (value_type) {

				case STRING:
					{
						char message_to[NAME_LEN] = {'\0'};
						get_value_string('t', message_to, colon_position, _json_payload, _json_length);
						return strcmp(message_to, name) == 0;
					}
				break;
				
				case INTEGER:
					{
						uint32_t number = get_value_number('t', _json_payload, _json_length, colon_position);
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
		size_t json_i = get_colon_position(key, _json_payload, _json_length, colon_position);
		return json_i > 0;
	}

	bool has_identity() const {
		return get_colon_position('i', _json_payload, _json_length) > 0;
	}

	bool has_from() const {
		return get_colon_position('f', _json_payload, _json_length) > 0;
	}

	bool has_to() const {
		return get_colon_position('t', _json_payload, _json_length) > 0;
	}

	bool has_to_name() const {
		size_t colon_position = get_colon_position('t', _json_payload, _json_length);
		return colon_position 
			&& get_value_type('t', _json_payload, _json_length, colon_position) == ValueType::STRING;
	}

	bool has_to_channel() const {
		size_t colon_position = get_colon_position('t', _json_payload, _json_length);
		return colon_position 
			&& get_value_type('t', _json_payload, _json_length, colon_position) == ValueType::INTEGER;
	}

	bool has_info() const {
		return get_colon_position('s', _json_payload, _json_length) > 0;
	}

	bool has_nth_value(uint8_t nth) const {
		if (nth < 10) {
			char value_key = '0' + nth;
			return get_colon_position(value_key, _json_payload, _json_length) > 0;
		}
		return false;
	}

	bool has_nth_value_string(uint8_t nth) const {
		if (nth < 10) {
			char value_key = '0' + nth;
			size_t colon_position = get_colon_position(value_key, _json_payload, _json_length);
			if (colon_position) {
				return get_value_type(value_key, _json_payload, _json_length, colon_position) == ValueType::STRING;
			}
		}
		return false;
	}

	bool has_nth_value_number(uint8_t nth) const {
		if (nth < 10) {
			char value_key = '0' + nth;
			size_t colon_position = get_colon_position(value_key, _json_payload, _json_length);
			if (colon_position) {
				return get_value_type(value_key, _json_payload, _json_length, colon_position) == ValueType::INTEGER;
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
		size_t colon_position = get_colon_position('t', _json_payload, _json_length);
		if (colon_position) {
			ValueType value_type = get_value_type('t', _json_payload, _json_length, colon_position);
			if (value_type == ValueType::STRING && get_value_string('t', _temp_string, NAME_LEN, _json_payload, _json_length, colon_position)) {
				return strcmp(_temp_string, name) == 0;
			}
		}
		return false;
	}

	bool is_to_channel(uint8_t channel) const {
		size_t colon_position = get_colon_position('t', _json_payload, _json_length);
		return colon_position 
			&& get_value_type('t', _json_payload, _json_length, colon_position) == ValueType::INTEGER
			&& get_value_number('t', _json_payload, _json_length, colon_position) == channel;
	}


	// GETTERS

	
	ValueType get_value_type(char key) const {
		return get_value_type(key, _json_payload, _json_length);
	}

	uint32_t get_value_number(char key) const {
		return get_value_number(key, _json_payload, _json_length);
	}

	MessageValue get_message_value() const {
		size_t colon_position = get_colon_position('m', _json_payload, _json_length);
		if (colon_position) {
			uint8_t message_number = get_value_number('m', _json_payload, _json_length, colon_position);
			if (message_number < static_cast<uint8_t>( MessageValue::NOISE )) {
				return static_cast<MessageValue>( message_number );
			}
		}
		return MessageValue::NOISE;
	}

	uint16_t get_identity() {
		return static_cast<uint16_t>(get_value_number('i', _json_payload, _json_length));
	}

	uint16_t get_timestamp() {
		return get_identity();
	}

	BroadcastValue get_broadcast_value() const {
		size_t colon_position = get_colon_position('b', _json_payload, _json_length);
		if (colon_position) {
			BroadcastValue broadcast_value = static_cast<BroadcastValue>( get_value_number('b', _json_payload, _json_length, colon_position) );
			if (broadcast_value < BroadcastValue::NONE) {
				return broadcast_value;
			}
		}
		return BroadcastValue::NONE;
	}

	RogerValue get_roger_value() const {
		size_t colon_position = get_colon_position('r', _json_payload, _json_length);
		if (colon_position) {
			uint8_t roger_number = (uint8_t)get_value_number('r', _json_payload, _json_length, colon_position);
			if (roger_number < static_cast<uint8_t>( RogerValue::NIL )) {
				return static_cast<RogerValue>( roger_number );
			}
		}
		return RogerValue::NIL;
	}

	InfoValue get_info_value() const {
		size_t colon_position = get_colon_position('s', _json_payload, _json_length);
		if (colon_position) {
			uint8_t info_number = (uint8_t)get_value_number('s', _json_payload, _json_length, colon_position);
			if (info_number < static_cast<uint8_t>( InfoValue::UNDEFINED )) {
				return static_cast<InfoValue>( info_number );
			}
		}
		return InfoValue::UNDEFINED;
	}

    // New method using internal temporary buffer (_temp_string)
    char* get_from_name() const {
        if (get_value_string('f', _temp_string, NAME_LEN, _json_payload, _json_length)) {
            return _temp_string;  // safe C string
        }
        return nullptr;  // failed
    }

	ValueType get_to_type() const {
		return get_value_type('t', _json_payload, _json_length);
	}

    // New method using internal temporary buffer (_temp_string)
    char* get_to_name() const {
		size_t colon_position = get_colon_position('t', _json_payload, _json_length);
		if (colon_position && get_value_type('t', _json_payload, _json_length, colon_position) == ValueType::STRING) {
			if (get_value_string('t', _temp_string, NAME_LEN, _json_payload, _json_length, colon_position)) {
				return _temp_string;
			}
		}
        return nullptr;  // failed
    }
	
	uint8_t get_to_channel() const {
		size_t colon_position = get_colon_position('t', _json_payload, _json_length);
		if (colon_position && get_value_type('t', _json_payload, _json_length, colon_position) == ValueType::INTEGER) {
			return (uint8_t)get_value_number('t', _json_payload, _json_length, colon_position);
		}
		return 255;	// Means, no chanel
	}


	TalkerMatch get_talker_match() const {
		if (has_to()) {
			ValueType value_type = get_to_type();
			switch (value_type) {
				case ValueType::INTEGER: return TalkerMatch::BY_CHANNEL;
				case ValueType::STRING: return TalkerMatch::BY_NAME;
				default: break;
			}
		} else {
			MessageValue message_value = get_message_value();
			if (message_value > MessageValue::PING || has_nth_value_number(0)) {
				// Only TALK, CHANNEL and PING can be for ANY
				// AVOIDS DANGEROUS ALL AT ONCE TRIGGERING (USE CHANNEL INSTEAD)
				// AVOIDS DANGEROUS SETTING OF ALL CHANNELS AT ONCE
				return TalkerMatch::FAIL;
			} else {
				return TalkerMatch::ANY;
			}
		}
		return TalkerMatch::NONE;
	}

	
	ValueType get_nth_value_type(uint8_t nth) {
		if (nth < 10) {
			char value_key = '0' + nth;
			return get_value_type(value_key, _json_payload, _json_length);
		}
		return ValueType::VOID;
	}

	char* get_nth_value_string(uint8_t nth) const {
		if (nth < 10 && get_value_string('0' + nth, _temp_string, MAX_LEN, _json_payload, _json_length)) {
			return _temp_string;  // safe C string
		}
		return nullptr;  // failed
	}

	uint32_t get_nth_value_number(uint8_t nth) const {
		if (nth < 10) {
			char value_key = '0' + nth;
			return get_value_number(value_key, _json_payload, _json_length);
		}
		return false;
	}

	char* get_action_string() const {
		if (get_value_string('a', _temp_string, NAME_LEN, _json_payload, _json_length)) {
			return _temp_string;  // safe C string
		}
		return nullptr;  // failed
	}

	uint32_t get_action_number() const {
		return get_value_number('a', _json_payload, _json_length);
	}


	// REMOVERS

	bool remove_message() {
		return remove('m', _json_payload, &_json_length);
	}

	bool remove_from() {
		return remove('f', _json_payload, &_json_length);
	}

	bool remove_to() {
		return remove('t', _json_payload, &_json_length);
	}

	bool remove_identity() {
		return remove('i', _json_payload, &_json_length);
	}

	bool remove_timestamp() {
		return remove('i', _json_payload, &_json_length);
	}

	bool remove_broadcast_value() {
		return remove('b', _json_payload, &_json_length);
	}

	bool remove_roger_value() {
		return remove('r', _json_payload, &_json_length);
	}

	bool remove_info_value() {
		return remove('s', _json_payload, &_json_length);
	}

	bool remove_nth_value(uint8_t nth) {
		if (nth < 10) {
			char value_key = '0' + nth;
			return remove(value_key, _json_payload, &_json_length);
		}
		return false;
	}

	// SETTERS


	bool set_message(MessageValue message_value) {
		size_t value_position = get_value_position('m', _json_payload, _json_length);
		if (value_position) {
			_json_payload[value_position] = '0' + static_cast<uint8_t>(message_value);
			return true;
		}
		return false;
	}

	bool set_identity(uint16_t identity) {
		return set_number('i', identity, _json_payload, &_json_length);
	}

	bool set_identity() {
		uint16_t identity = (uint16_t)millis();
		return set_number('i', identity, _json_payload, &_json_length);
	}

	bool set_timestamp(uint16_t timestamp) {
		return set_number('i', timestamp, _json_payload, &_json_length);
	}

	bool set_timestamp() {
		return set_identity();
	}

	bool set_from_name(const char* name) {
		return set_string('f', name, _json_payload, &_json_length);
	}

	bool set_to_name(const char* name) {
		return set_string('t', name, _json_payload, &_json_length);
	}

	bool set_to_channel(uint8_t channel) {
		return set_number('t', channel, _json_payload, &_json_length);
	}

	bool set_broadcast_value(BroadcastValue broadcast_value) {
		size_t value_position = get_value_position('b', _json_payload, _json_length);
		if (value_position) {
			_json_payload[value_position] = '0' + static_cast<uint8_t>(broadcast_value);
			return true;
		}
		return set_number('b', static_cast<uint8_t>(broadcast_value), _json_payload, &_json_length);
	}

	bool set_roger_value(RogerValue roger_value) {
		size_t value_position = get_value_position('r', _json_payload, _json_length);
		if (value_position) {
			_json_payload[value_position] = '0' + static_cast<uint8_t>(roger_value);
			return true;
		}
		return set_number('r', static_cast<uint8_t>(roger_value), _json_payload, &_json_length);
	}

	bool set_info_value(InfoValue info_value) {
		size_t value_position = get_value_position('s', _json_payload, _json_length);
		if (value_position) {
			_json_payload[value_position] = '0' + static_cast<uint8_t>(info_value);
			return true;
		}
		return set_number('s', static_cast<uint8_t>(info_value), _json_payload, &_json_length);
	}

	bool set_nth_value_number(uint8_t nth, uint32_t number) {
		if (nth < 10) {
			return set_number('0' + nth, number, _json_payload, &_json_length);
		}
		return false;
	}

	bool set_nth_value_string(uint8_t nth, const char* in_string) {
		if (nth < 10) {
			return set_string('0' + nth, in_string, _json_payload, &_json_length);
		}
		return false;
	}

	bool swap_from_with_to() {
		size_t key_from_position = get_key_position('f', _json_payload, _json_length);
		size_t key_to_position = get_key_position('t', _json_payload, _json_length);
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
