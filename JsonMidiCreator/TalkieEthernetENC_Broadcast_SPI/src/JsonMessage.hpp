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


/**
 * @file JsonMessage.hpp
 * @brief JSON message handling for Talkie communication protocol
 * 
 * This class provides efficient, memory-safe JSON message manipulation 
 * for embedded systems with constrained resources. It implements a 
 * schema-driven JSON protocol optimized for Arduino environments.
 * 
 * @warning This class does not use dynamic memory allocation.
 *          All operations are performed on fixed-size buffers.
 * 
 * @section constraints Memory Constraints
 * - Maximum buffer size: TALKIE_BUFFER_SIZE (default: 128 bytes)
 * - Maximum name length: TALKIE_NAME_LEN (default: 16 bytes including null terminator)
 * - Maximum string length: TALKIE_MAX_LEN (default: 64 bytes including null terminator)
 * 
 * @author Rui Seixas Monteiro
 * @date Created: 2026-01-01
 * @version 1.0.0
 */

#ifndef JSON_MESSAGE_HPP
#define JSON_MESSAGE_HPP

#include <Arduino.h>        // Needed for Serial given that Arduino IDE only includes Serial in .ino files!
#include "TalkieCodes.hpp"

// Guaranteed memory safety, constrained / schema-driven JSON protocol
// Advisable maximum sizes:
// 		f (from / name) → 16 bytes (15 + '\0')
// 		d (description) → 64 bytes (63 + '\0')


// #define MESSAGE_DEBUG_TIMING

#define TALKIE_BUFFER_SIZE 128	    ///< Default buffer size for JSON message
#define TALKIE_NAME_LEN 16			///< Default maximum length for name fields
#define TALKIE_MAX_LEN 64			///< Default maximum length for string fields


using LinkType			= TalkieCodes::LinkType;
using TalkerMatch 		= TalkieCodes::TalkerMatch;
using BroadcastValue 	= TalkieCodes::BroadcastValue;
using MessageValue 		= TalkieCodes::MessageValue;
using SystemValue 		= TalkieCodes::SystemValue;
using RogerValue 		= TalkieCodes::RogerValue;
using ErrorValue 		= TalkieCodes::ErrorValue;
using ValueType 		= TalkieCodes::ValueType;

// Forward declaration
class BroadcastSocket;

/**
 * @class JsonMessage
 * @brief JSON message container and manipulator for Talkie protocol
 * 
 * This class manages JSON-formatted messages with a fixed schema:
 * - Mandatory fields: m (message), b (broadcast), i (identity), f (from)
 * - Optional fields: t (to), r (roger), s (system), a (action), 0-9 (values)
 * 
 * @note All string operations are bounds-checked to prevent buffer overflows.
 */
class JsonMessage {
public:

	struct Original {
		uint16_t identity;
		MessageValue message_value;
	};


	#ifdef MESSAGE_DEBUG_TIMING
	unsigned long _reference_time = millis();
	#endif

private:

	char _json_payload[TALKIE_BUFFER_SIZE];			///< Internal JSON buffer
	size_t _json_length = 0;						///< Current length of JSON string
    mutable char _temp_string[TALKIE_MAX_LEN];		///< Temporary buffer for string operations


    // ============================================
    // GENERIC METHODS (Parsing utilities)
    // ============================================

    /**
     * @brief Calculate number of digits in an unsigned integer
     * @param number The number to analyze
     * @return Number of decimal digits (1-10)
     * 
     * @note Handles numbers from 0 to 4,294,967,295
     */
	static size_t _number_of_digits(uint32_t number) {
		size_t length = 1;	// 0 has 1 digit
		while (number > 9) {
			number /= 10;
			length++;
		}
		return length;
	}


	/**
     * @brief This helper method generates the checksum of a given buffer content
     */
    uint16_t _generateChecksum() const {	// 16-bit word and XORing
        uint16_t checksum = 0;
		if (_json_length <= TALKIE_BUFFER_SIZE) {
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


    /**
     * @brief Find the position of the colon for a given key
     * @param key Single character key to search for
     * @param colon_position Starting position for search (default: 4)
     * @return Position of colon, or 0 if not found
     * 
     * @note Searches for pattern: `"key":`
     */
	size_t _get_colon_position(char key, size_t colon_position = 4) const {
		for (size_t json_i = colon_position; json_i < _json_length; ++json_i) {	// 4 because it's the shortest position possible for ':'
			if (_json_payload[json_i] == ':' && _json_payload[json_i - 2] == key && _json_payload[json_i - 3] == '"' && _json_payload[json_i - 1] == '"') {
				return json_i;
			}
		}
		return 0;
	}


    /**
     * @brief Get position of value for a given key
     * @param key Single character key
     * @param colon_position Optional hint for colon position
     * @return Position of first character after colon, or 0 if not found
     */
	size_t _get_value_position(char key, size_t colon_position = 4) const {
		colon_position = _get_colon_position(key, colon_position);
		if (colon_position) {			//     01
			return colon_position + 1;	// {"k":x}
		}
		return 0;
	}


    /**
     * @brief Get position of key character
     * @param key Single character key
     * @param colon_position Optional hint for colon position
     * @return Position of key character, or 0 if not found
     */
	size_t _get_key_position(char key, size_t colon_position = 4) const {
		colon_position = _get_colon_position(key, colon_position);
		if (colon_position) {			//   210
			return colon_position - 2;	// {"k":x}
		}
		return 0;
	}


    /**
     * @brief Calculate total field length (key + value)
     * @param key Single character key
     * @param colon_position Optional hint for colon position
     * @return Total characters occupied by field (including quotes, colon, commas)
     */
	size_t _get_field_length(char key, size_t colon_position = 4) const {
		size_t field_length = 0;
		size_t json_i = _get_value_position(key, colon_position);
		if (json_i) {
			field_length = 4;	// All keys occupy 4 '"k":' chars
			ValueType value_type = _get_value_type(key, json_i - 1);
			switch (value_type) {

				case ValueType::TALKIE_VT_STRING:
					field_length += 2;	// Adds the two '"' associated to the string
					for (json_i++; json_i < _json_length && _json_payload[json_i] != '"'; json_i++) {
						field_length++;
					}
					break;
				
				case ValueType::TALKIE_VT_INTEGER:
					for (; json_i < _json_length && !(_json_payload[json_i] > '9' || _json_payload[json_i] < '0'); json_i++) {
						field_length++;
					}
					break;
				
				default: break;
			}
		}
		return field_length;
	}


	/**
     * @brief Determine value type for a key
     * @param key Single character key
     * @param colon_position Optional hint for colon position
     * @return ValueType enum indicating the type of value
     */
	ValueType _get_value_type(char key, size_t colon_position = 4) const {
		size_t json_i = _get_value_position(key, colon_position);
		if (json_i) {
			if (_json_payload[json_i] == '"') {
				for (json_i++; json_i < _json_length && _json_payload[json_i] != '"'; json_i++) {}
				if (json_i == _json_length) {
					return ValueType::TALKIE_VT_VOID;
				}
				return ValueType::TALKIE_VT_STRING;
			} else {
				while (json_i < _json_length && _json_payload[json_i] != ',' && _json_payload[json_i] != '}') {
					if (_json_payload[json_i] > '9' || _json_payload[json_i] < '0') {
						return ValueType::TALKIE_VT_OTHER;
					}
					json_i++;
				}
				if (json_i == _json_length) {
					return ValueType::TALKIE_VT_VOID;
				}
				return ValueType::TALKIE_VT_INTEGER;
			}
		}
		return ValueType::TALKIE_VT_VOID;
	}


    /**
     * @brief Extract string value for a key
     * @param key Single character key
     * @param[out] buffer Output buffer for string
     * @param size Size of output buffer (including null terminator)
     * @param colon_position Optional hint for colon position
     * @return true if successful, false if key not found or buffer too small
     * 
     * @warning Buffer size must include space for null terminator
     */
	bool _get_value_string(char key, char* buffer, size_t size, size_t colon_position = 4) const {
		if (buffer && size) {
			size_t json_i = _get_value_position(key, colon_position);
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


	/**
     * @brief Extract numeric value for a key
     * @param key Single character key
     * @param colon_position Optional hint for colon position
     * @return Extracted number, or 0 if key not found or not a number
     */
	uint32_t _get_value_number(char key, size_t colon_position = 4) const {
		uint32_t json_number = 0;
		size_t json_i = _get_value_position(key, colon_position);
		if (json_i) {
			while (json_i < _json_length && !(_json_payload[json_i] > '9' || _json_payload[json_i] < '0')) {
				json_number *= 10;
				json_number += _json_payload[json_i++] - '0';
			}
		}
		return json_number;
	}


    // ============================================
    // MEMBER METHODS (Modification utilities)
    // ============================================

    /**
     * @brief Reset JSON payload to the bare minimum
     * 
     * Default bare minimum message: `{}`
     */
	void _reset() {
		_json_payload[0] = '{';
		_json_payload[1] = '}';
		_json_length = 2;
	}


    /**
     * @brief Remove a key-value pair from JSON
     * @param key Key to remove
     * @param colon_position Optional hint for colon position
     * 
     * @note Also removes leading or trailing commas as needed
     */
	void _remove(char key, size_t colon_position = 4) {
		colon_position = _get_colon_position(key, colon_position);
		if (colon_position) {
			size_t field_position = colon_position - 3;	// All keys occupy 3 '"k":' chars to the left of the colon
			size_t field_length = _get_field_length(key, colon_position);	// Excludes possible heading ',' separation comma
			if (_json_payload[field_position - 1] == ',') {	// the heading ',' has to be removed too
				field_position--;
				field_length++;
			} else if (_json_payload[field_position + field_length] == ',') {
				field_length++;	// Changes the length only, to pick up the tailing ','
			}
			for (size_t json_i = field_position; json_i < _json_length - field_length; json_i++) {
                _json_payload[json_i] = _json_payload[json_i + field_length];
            }
			_json_length -= field_length;	// Finally updates the _json_payload full length
		}
	}


    /**
     * @brief Set numeric value for a key
     * @param key Key to set
     * @param number Numeric value
     * @param colon_position Optional hint for colon position
     * @return true if successful, false if buffer too small
     * 
     * @note If key exists, it's replaced. Otherwise, it's added before closing brace.
     */
	bool _set_number(char key, uint32_t number, size_t colon_position = 4) {
		colon_position = _get_colon_position(key, colon_position);
		if (colon_position) _remove(key, colon_position);
		// At this time there is no field key for sure, so, one can just add it right before the '}'
		size_t number_size = _number_of_digits(number);
		// the usual key 4 plus the + 1 due to the ',' needed to be added to the beginning
		size_t new_length = _json_length + 1 + 4 + number_size;
		if (new_length > TALKIE_BUFFER_SIZE) {
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
			_reset();	// Something very wrong, needs to be reset
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


    /**
     * @brief Set numeric value for a single digit field value
     * @param key Key to set
     * @param number Numeric value
     * @param colon_position Optional hint for colon position
     * @return true if successful, false if buffer too small
     * 
     * @note If key exists, the value is replaced. Otherwise, it's added before closing brace.
     */
	bool _set_single_digit_number(char key, uint32_t number, size_t colon_position = 4) {
		if (number < 10) {
			colon_position = _get_colon_position(key, colon_position);
			if (colon_position) {
				size_t value_position = _get_value_position(key, colon_position);
				_json_payload[value_position] = '0' + number;
			} else {
				return _set_number(key, number);
			}
		}
		return false;
	}


    /**
     * @brief Set string value for a key
     * @param key Key to set
     * @param in_string String value (null-terminated)
     * @param colon_position Optional hint for colon position
     * @return true if successful, false if buffer too small or string empty
     */
	bool _set_string(char key, const char* in_string, size_t colon_position = 4) {
		if (in_string) {
			size_t length = 0;
			for (size_t char_j = 0; in_string[char_j] != '\0' && char_j < TALKIE_BUFFER_SIZE; char_j++) {
				length++;
			}
			// It can have empty strings too, so, a length can be 0!
			colon_position = _get_colon_position(key, colon_position);
			if (colon_position) _remove(key, colon_position);
			// the usual key + 4 plus + 2 for both '"' and the + 1 due to the heading ',' needed to be added
			size_t new_length = _json_length + length + 1 + 4 + 2;
			if (new_length > TALKIE_BUFFER_SIZE) {
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
				_reset();	// Something very wrong, needs to be reset
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
		return false;
	}

public:

    // ============================================
    // CONSTRUCTORS AND DESTRUCTOR
    // ============================================

    /**
     * @brief Default constructor
     * 
     * Initializes with the bare minimum: `{}`
     */
	JsonMessage() {
		_reset();	// Initiate with the bare minimum
	}


    /**
     * @brief Constructor from buffer
     * @param buffer Source buffer containing JSON
     * @param length Length of buffer
     * 
     * @note If deserialization fails, resets to default message
     */
	JsonMessage(const char* buffer, size_t length) {
		if (!deserialize_buffer(buffer, length)) {
			_reset();
		}
	}


    /**
     * @brief Copy constructor
     * @param other JsonMessage to copy from
     */
	JsonMessage(const JsonMessage& other) {
		_json_length = other._json_length;
		for (size_t json_i = 0; json_i < _json_length; ++json_i) {
			_json_payload[json_i] = other._json_payload[json_i];
		}
	}


    /**
     * @brief Destructor
     */
	~JsonMessage() {
		// Does nothing
	}


    // ============================================
    // OPERATORS
    // ============================================

    /**
     * @brief Equality operator
     * @param other JsonMessage to compare with
     * @return true if JSON content is identical
     */
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


    /**
     * @brief Inequality operator
     * @param other JsonMessage to compare with
     * @return true if JSON content differs
     */
	bool operator!=(const JsonMessage& other) const {
		return !(*this == other);
	}


    /**
     * @brief Assignment operator
     * @param other JsonMessage to copy from
     * @return Reference to this object
     */
    JsonMessage& operator=(const JsonMessage& other) {
        if (this == &other) return *this;

        _json_length = other._json_length;
        for (size_t i = 0; i < _json_length; ++i) {
            _json_payload[i] = other._json_payload[i];
        }
        return *this;
    }


    // ============================================
    // BASIC OPERATIONS
    // ============================================

    /**
     * @brief Get current JSON length
     * @return Length of JSON string (not including null terminator like '\0')
     */
	size_t _get_length() const {
		return _json_length;
	}


    /**
     * @brief Set current JSON length
     * @return Length of JSON string (not including null terminator like '\0')
     */
	void _set_length(size_t length) {
        _json_length = length;
    }


    /**
     * @brief Handy method that allows to add single chars one by one
     * @return true if it has space for the added char
     */
	bool _append(char c) {
		if (_json_length < TALKIE_BUFFER_SIZE) {
			_json_payload[_json_length++] = c;
			return true;
		}
		return false;
	}


    /**
     * @brief Allows a read only access to the message buffer
     * @return A constant pointer to the message buffer
     */
	const char* _read_buffer() const {
		return _json_payload;
	}


    /**
     * @brief Allows a read and write access to the message buffer
     * @param length The length of the amount of data intended to be written
     * @return A pointer to the message buffer to write on, or nullptr, if `length` is
	 *         greater than `TALKIE_BUFFER_SIZE`
     */
	char* _write_buffer(size_t length = 0) {
		if (length > TALKIE_BUFFER_SIZE) return nullptr;
        return _json_payload;
    }

	
    /**
     * @brief Reset to a bare minimum message
     * 
     * Resets to: `{}`
     */
	void reset() {
		_reset();
	}


    /**
     * @brief Deserialize from buffer
     * @param buffer Source buffer
     * @param length Length of buffer
     * @return true if successful, false if buffer is null or too large
     * 
     * @warning Does not validate JSON structure
     */
	bool deserialize_buffer(const char* buffer, size_t length) {
		if (buffer && length && length <= TALKIE_BUFFER_SIZE) {
			for (size_t char_j = 0; char_j < length; ++char_j) {
				_json_payload[char_j] = buffer[char_j];
			}
			_json_length = length;
			return true;
		}
		return false;
	}


    /**
     * @brief Serialize to buffer
     * @param[out] buffer Destination buffer
     * @param size Size of destination buffer
     * @return Number of bytes written, or 0 if buffer too small
     */
	size_t serialize_json(char* buffer, size_t size) const {
		if (buffer && size >= _json_length) {
			for (size_t json_i = 0; json_i < _json_length; ++json_i) {
				buffer[json_i] = _json_payload[json_i];
			}
			return _json_length;
		}
		return 0;
	}


    /**
     * @brief Write JSON to Print interface
     * @param out Print interface (Serial, File, etc.)
     * @return true if all bytes written successfully
     */
	bool write_to(Print& out) const {
		if (_json_length) {
			return out.write(reinterpret_cast<const uint8_t*>(_json_payload), _json_length) == _json_length;
		}
		return false;
	}


    /**
     * @brief Validates the delimiters while adjusting the last `}` one if necessary
     * @return true if the message is correctly delimited inside `{}`
     */
	bool _validate_json() {
		
		// Trim trailing newline and carriage return characters or any other that isn't '}'
		while (_json_length > 26 
			&& (_json_payload[_json_length - 1] != '}' || _json_payload[_json_length - 2] == '\\')) {
			_json_length--;	// Note that literals add the '\0'!
		}

		// Minimum valid length: '{"m":0,"b":0,"i":0,"f":"n"}' = 27
		if (_json_length < 27) {
			_reset();
			return false;
		}

		if (_json_payload[0] != '{') {
			_reset();
			return false;	
		}
		return true;
	}


    /**
     * @brief Checks if the checksum of the message matches the on in the respective field,
	 *        if not, sets the message value as `NOISE`, so, it still shall be transmitted in order
	 *        to be processed by the Talker and be returned as error to the original sender
     * @return true if it has a valid checksum
     */
	bool _process_checksum() {
		size_t c_colon_position = _get_colon_position('c');
		uint16_t received_checksum = _get_value_number('c', c_colon_position);
		_remove('c', c_colon_position);
		uint16_t checksum = _generateChecksum();
		if (checksum != received_checksum) {
			// Mark error message as noise and dispatch it to be processed by the respective Talker
			set_message_value(MessageValue::TALKIE_MSG_NOISE);
			set_error_value(ErrorValue::TALKIE_ERR_CHECKSUM);
			return false;
		}
		return true;
	}


    /**
     * @brief Generates a new message checksum and inserts it in the message
     * @return true if it had space to insert the checksum field
     */
	bool _insert_checksum() {
		// Starts by clearing any pre existent checksum (NO surprises or miss receives)
		_remove('c');
		uint16_t checksum = _generateChecksum();
		return _set_number('c', checksum);
	}


    // ============================================
    // MESSAGE TARGETING
    // ============================================

    /**
     * @brief Check if message is intended for this recipient
     * @param name Recipient name
     * @param channel Recipient channel
     * @return true if message targets this name/channel or is broadcast
     * 
     * Rules:
     * - If 't' field is string: match against name
     * - If 't' field is number: match against channel
     * - No 't' field: broadcast message (true for all)
     */
	bool for_me(const char* name, uint8_t channel) const {
		size_t colon_position = _get_colon_position('t');
		if (colon_position) {
			ValueType value_type = _get_value_type('t', colon_position);
			switch (value_type) {

				case ValueType::TALKIE_VT_STRING:
					{
						char message_to[TALKIE_NAME_LEN] = {'\0'};
						_get_value_string('t', message_to, colon_position);
						return strcmp(message_to, name) == 0;
					}
				break;
				
				case ValueType::TALKIE_VT_INTEGER:
					{
						uint32_t number = _get_value_number('t', colon_position);
						return number == channel;
					}
				break;
				
				default: break;
			}
		}
		return true;	// Non target messages, without to(t), are considered broadcasted messages an for everyone
	}


    /**
     * @brief Compare with buffer content
     * @param buffer Buffer to compare with
     * @param length Length of buffer
     * @return true if content matches exactly
     */
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


    // ============================================
    // FIELD EXISTENCE CHECKS
    // ============================================

    /**
     * @brief Check if key exists
     * @param key Key to check
     * @param colon_position Optional hint for search
     * @return true if key exists in JSON
     */
	bool has_key(char key, size_t colon_position = 4) const {
		size_t json_i = _get_colon_position(key, colon_position);
		return json_i > 0;
	}


	/** @brief Check if checksum field exists */ 
	bool has_checksum() const {
		return _get_colon_position('c') > 0;
	}


	/** @brief Check if identity field exists */ 
	bool has_identity() const {
		return _get_colon_position('i') > 0;
	}


	/** @brief Check if broadcast value field exists */ 
	bool has_broadcast_value() const {
		return _get_colon_position('b') > 0;
	}


	/** @brief Check if 'from' field exists */
	bool has_from() const {
		return _get_colon_position('f') > 0;
	}


	/** @brief Check if 'to' field exists */
	bool has_to() const {
		return _get_colon_position('t') > 0;
	}


	/** @brief Check if 'to' field is a string (name) */
	bool has_to_name() const {
		size_t colon_position = _get_colon_position('t');
		return colon_position 
			&& _get_value_type('t', colon_position) == ValueType::TALKIE_VT_STRING;
	}


	/** @brief Check if 'to' field is a number (channel) */
	bool has_to_channel() const {
		size_t colon_position = _get_colon_position('t');
		return colon_position 
			&& _get_value_type('t', colon_position) == ValueType::TALKIE_VT_INTEGER;
	}


	/** @brief Check if system field exists */
	bool has_system() const {
		return _get_colon_position('s') > 0;
	}


	/** @brief Check if error field exists */
	bool has_error() const {
		return _get_colon_position('e') > 0;
	}


    /**
     * @brief Check if nth value field exists (0-9)
     * @param nth Index 0-9
     * @return true if field exists
     */
	bool has_nth_value(uint8_t nth) const {
		if (nth < 10) {
			char value_key = '0' + nth;
			return _get_colon_position(value_key) > 0;
		}
		return false;
	}


    /**
     * @brief Check if nth value is a string
     * @param nth Index 0-9
     * @return true if field exists and is string
     */
	bool has_nth_value_string(uint8_t nth) const {
		if (nth < 10) {
			char value_key = '0' + nth;
			size_t colon_position = _get_colon_position(value_key);
			if (colon_position) {
				return _get_value_type(value_key, colon_position) == ValueType::TALKIE_VT_STRING;
			}
		}
		return false;
	}


    /**
     * @brief Check if nth value is a number
     * @param nth Index 0-9
     * @return true if field exists and is number
     */
	bool has_nth_value_number(uint8_t nth) const {
		if (nth < 10) {
			char value_key = '0' + nth;
			size_t colon_position = _get_colon_position(value_key);
			if (colon_position) {
				return _get_value_type(value_key, colon_position) == ValueType::TALKIE_VT_INTEGER;
			}
		}
		return false;
	}


    /**
     * @brief Get if it's not to be replied
     * @return true if it's not to be replied with echo
     */
	bool is_no_reply() const {
		return _get_colon_position('n') > 0;
	}


    // ============================================
    // FIELD VALUE CHECKS
    // ============================================

    /**
     * @brief Check if 'from' field matches name
     * @param name Name to compare with
     * @return true if 'from' field exists and matches
     */
	bool is_from(const char* name) const {
		const char* from_name = get_from_name();
		if (from_name) {
			return strcmp(name, from_name) == 0;
		}
		return false;
	}


    /**
     * @brief Check if 'to' field matches name
     * @param name Name to compare with
     * @return true if 'to' field is string and matches
     */
	bool is_to_name(const char* name) const {
		size_t colon_position = _get_colon_position('t');
		if (colon_position) {
			ValueType value_type = _get_value_type('t', colon_position);
			if (value_type == ValueType::TALKIE_VT_STRING && _get_value_string('t', _temp_string, TALKIE_NAME_LEN, colon_position)) {
				return strcmp(_temp_string, name) == 0;
			}
		}
		return false;
	}


    /**
     * @brief Check if 'to' field matches channel
     * @param channel Channel number (0-254)
     * @return true if 'to' field is number and matches
     * 
     * @note Returns false for channel 255 (reserved)
     */
	bool is_to_channel(uint8_t channel) const {
		size_t colon_position = _get_colon_position('t');
		return colon_position 
			&& _get_value_type('t', colon_position) == ValueType::TALKIE_VT_INTEGER
			&& _get_value_number('t', colon_position) == channel;
	}


    // ============================================
    // GETTERS - FIELD VALUES
    // ============================================

    /**
     * @brief Get the key value type
     * @param nth A single char like 'm'
     * @return ValueType enum, or TALKIE_VT_VOID if invalid index
     */
	ValueType get_key_value_type(char key) {
		return _get_value_type(key);
	}


    /**
     * @brief Get message type
     * @return MessageValue enum, or TALKIE_MSG_NOISE if invalid
     */
	MessageValue get_message_value() const {
		return static_cast<MessageValue>( _get_value_number('m') );
	}


    /**
     * @brief Get identity number
     * @return Identity value (0-65535)
     */
	uint16_t get_identity() {
		return static_cast<uint16_t>(_get_value_number('i'));
	}


    /**
     * @brief Get timestamp (alias for identity)
     * @return Timestamp value in milliseconds (0-65535)
     */
	uint16_t get_timestamp() {
		return get_identity();
	}


    /**
     * @brief Get broadcast type
     * @return BroadcastValue enum, or TALKIE_BC_NONE if invalid
     */
	BroadcastValue get_broadcast_value() const {
		return static_cast<BroadcastValue>( _get_value_number('b') );
	}


    /**
     * @brief Get roger/acknowledgment type
     * @return RogerValue enum, or TALKIE_RGR_NIL if invalid
     */
	RogerValue get_roger_value() const {
		return static_cast<RogerValue>( _get_value_number('r') );
	}


    /**
     * @brief Get system information type
     * @return SystemValue enum, or TALKIE_SYS_UNDEFINED if invalid
     */
	SystemValue get_system_value() const {
		return static_cast<SystemValue>( _get_value_number('s') );
	}


    /**
     * @brief Get error type
     * @return ErrorValue enum, or TALKIE_ERR_UNDEFINED if invalid
     */
	ErrorValue get_error_value() const {
		return static_cast<ErrorValue>( _get_value_number('e') );
	}
	

    /**
     * @brief Get sender name
     * @return Pointer to sender name string, or nullptr if not found
     * 
     * @warning Returned pointer is to internal buffer. Copy if needed.
     */
    char* get_from_name() const {
        if (_get_value_string('f', _temp_string, TALKIE_NAME_LEN)) {
            return _temp_string;  // safe C string
        }
        return nullptr;  // failed
    }


    /**
     * @brief Get target type
     * @return ValueType of 't' field
     */
	ValueType get_to_type() const {
		return _get_value_type('t');
	}


    /**
     * @brief Get target name
     * @return Pointer to target name, or nullptr if not a string
     */
    const char* get_to_name() const {
		size_t colon_position = _get_colon_position('t');
		if (colon_position && _get_value_type('t', colon_position) == ValueType::TALKIE_VT_STRING) {
			if (_get_value_string('t', _temp_string, TALKIE_NAME_LEN, colon_position)) {
				return _temp_string;
			}
		}
        return nullptr;  // failed
    }
	

    /**
     * @brief Get target channel
     * @return Channel number (0-254)
     */
	uint8_t get_to_channel() const {
		size_t colon_position = _get_colon_position('t');
		if (colon_position && _get_value_type('t', colon_position) == ValueType::TALKIE_VT_INTEGER) {
			return (uint8_t)_get_value_number('t', colon_position);
		}
		return 255;	// Means, no chanel
	}


    /**
     * @brief Get targeting method
     * @return TalkerMatch enum indicating how message is targeted
     * 
     * Determines if message is for specific name, channel, broadcast, or invalid.
     */
	TalkerMatch get_talker_match() const {
		// Has to have a valid `from`, anonymous messages aren't acceptable
		size_t from_position = _get_colon_position('f');
		if (!from_position || _get_value_type('f', from_position) != ValueType::TALKIE_VT_STRING) {
			return TalkerMatch::TALKIE_MATCH_FAIL;
		}
		size_t to_position = _get_colon_position('t');
		if (to_position) {
			ValueType value_type = _get_value_type('t', to_position);
			switch (value_type) {
				case ValueType::TALKIE_VT_INTEGER:
				{
					uint8_t channel = _get_value_number('t', to_position);
					if (channel < 255) {
						return TalkerMatch::TALKIE_MATCH_BY_CHANNEL;
					} else {	// 255 is a NO response channel
						return TalkerMatch::TALKIE_MATCH_FAIL;
					}
				}
				case ValueType::TALKIE_VT_STRING: return TalkerMatch::TALKIE_MATCH_BY_NAME;
				default: break;
			}
		} else {
			MessageValue message_value = get_message_value();
			if (message_value > MessageValue::TALKIE_MSG_PING || has_nth_value_number(0)) {
				// Only TALK, CHANNEL and PING can be for ANY
				// AVOIDS DANGEROUS ALL AT ONCE TRIGGERING (USE CHANNEL INSTEAD)
				// AVOIDS DANGEROUS SETTING OF ALL CHANNELS AT ONCE
				return TalkerMatch::TALKIE_MATCH_FAIL;
			} else {
				return TalkerMatch::TALKIE_MATCH_ANY;
			}
		}
		return TalkerMatch::TALKIE_MATCH_NONE;
	}

	
    /**
     * @brief Get nth value type
     * @param nth Index 0-9
     * @return ValueType enum, or TALKIE_VT_VOID if invalid index
     */
	ValueType get_nth_value_type(uint8_t nth) {
		if (nth < 10) {
			return _get_value_type('0' + nth);
		}
		return ValueType::TALKIE_VT_VOID;
	}


    /**
     * @brief Get nth value as string
     * @param nth Index 0-9
     * @return Pointer to string value, or nullptr if not string/invalid
     */
	char* get_nth_value_string(uint8_t nth) const {
		if (nth < 10 && _get_value_string('0' + nth, _temp_string, TALKIE_MAX_LEN)) {
			return _temp_string;  // safe C string
		}
		return nullptr;  // failed
	}


    /**
     * @brief Get nth value as number
     * @param nth Index 0-9
     * @return Numeric value, or 0 if not number/invalid
     */
	uint32_t get_nth_value_number(uint8_t nth) const {
		if (nth < 10) {
			return _get_value_number('0' + nth);
		}
		return 0;
	}


    /**
     * @brief Get action field type
     * @return ValueType of 'a' field
     */
	ValueType get_action_type() const {
		return _get_value_type('a');
	}


    /**
     * @brief Get action as a string
     * @return Pointer to action string, or nullptr if not string
     */
	char* get_action_string() const {
		if (_get_value_string('a', _temp_string, TALKIE_NAME_LEN)) {
			return _temp_string;  // safe C string
		}
		return nullptr;  // failed
	}


    /**
     * @brief Get action as a number
     * @return The action index
     */
	uint32_t get_action_index() const {
		return _get_value_number('a');
	}


    // ============================================
    // REMOVERS - FIELD DELETION
    // ============================================

    /** @brief Remove checksum field */
	void remove_checksum() {
		_remove('c');
	}


    /** @brief Remove message field */
	void remove_message() {
		_remove('m');
	}


	/** @brief Remove from field */
	void remove_from() {
		_remove('f');
	}


	/** @brief Remove to field */
	void remove_to() {
		_remove('t');
	}


	/** @brief Remove identity field */
	void remove_identity() {
		_remove('i');
	}


	/** @brief Remove timestamp field */
	void remove_timestamp() {
		_remove('i');
	}


	/** @brief Remove broadcast field */
	void remove_broadcast_value() {
		_remove('b');
	}


	/** @brief Remove roger field */
	void remove_roger_value() {
		_remove('r');
	}


	/** @brief Remove system field */
	void remove_system_value() {
		_remove('s');
	}


    /**
     * @brief Remove nth value field
     * @param nth Index 0-9
     */
	void remove_nth_value(uint8_t nth) {
		if (nth < 10) _remove('0' + nth);
	}


    /**
     * @brief Remove all the nth values
     * @return true if removed at least one value
     */
	void remove_all_nth_values() {
		for (uint8_t nth = 0; nth < 10; ++nth) {
			remove_nth_value(nth);
		}
	}


	/** @brief Remove no reply field */
	void remove_no_reply() {
		_remove('n');
	}


    // ============================================
    // SETTERS - FIELD MODIFICATION
    // ============================================

    /**
     * @brief Set message type
     * @param message_value Message type enum
     * @return true if field exists and was updated
     */
	bool set_message_value(MessageValue message_value) {
		return _set_single_digit_number('m', static_cast<uint32_t>(message_value));
	}


	/**
     * @brief Set identity number
     * @param identity Identity value (0-65535)
     * @return true if successful
     */
	bool set_identity(uint16_t identity) {
		return _set_number('i', identity);
	}


    /**
     * @brief Set identity to current millis()
     * @return true if successful
     */
	bool set_identity() {
		uint16_t identity = (uint16_t)millis();
		return _set_number('i', identity);
	}


    /**
     * @brief Set timestamp (alias for identity)
     * @param timestamp Timestamp value in milliseconds
     * @return true if successful
     */
	bool set_timestamp(uint16_t timestamp) {
		return _set_number('i', timestamp);
	}


    /**
     * @brief Set timestamp to current millis()
     * @return true if successful
     */
	bool set_timestamp() {
		return set_identity();
	}


    /**
     * @brief Set sender name
     * @param name Sender name string
     * @return true if successful
     */
	bool set_from_name(const char* name) {
		return _set_string('f', name);
	}


    /**
     * @brief Set target name
     * @param name Target name string
     * @return true if successful
     */
	bool set_to_name(const char* name) {
		return _set_string('t', name);
	}


    /**
     * @brief Set target channel
     * @param channel Channel number (0-254)
     * @return true if successful
     */
	bool set_to_channel(uint8_t channel) {
		return _set_number('t', channel);
	}


    /**
     * @brief Set action name
     * @param name Action name string
     * @return true if successful
     */
	bool set_action_name(const char* name) {
		return _set_string('a', name);
	}


    /**
     * @brief Set action index
     * @param index Action index
     * @return true if successful
     */
	bool set_action_index(uint8_t index) {
		return _set_number('a', index);
	}


    /**
     * @brief Set broadcast type
     * @param broadcast_value Broadcast type enum
     * @return true if successful
     */
	bool set_broadcast_value(BroadcastValue broadcast_value) {
		return _set_single_digit_number('b', static_cast<uint32_t>(broadcast_value));
	}


    /**
     * @brief Set roger/acknowledgment type
     * @param roger_value Roger type enum
     * @return true if successful
     */
	bool set_roger_value(RogerValue roger_value) {
		return _set_single_digit_number('r', static_cast<uint32_t>(roger_value));
	}


    /**
     * @brief Set system information type
     * @param system_value System type enum
     * @return true if successful
     */
	bool set_system_value(SystemValue system_value) {
		return _set_single_digit_number('s', static_cast<uint32_t>(system_value));
	}


    /**
     * @brief Set error type
     * @param error_value Error type
     * @return true if successful
     */
	bool set_error_value(ErrorValue error_value) {
		return _set_single_digit_number('e', static_cast<uint32_t>(error_value));
	}


    /**
     * @brief Set nth value as number
     * @param nth Index 0-9
     * @param number Numeric value
     * @return true if successful
     */
	bool set_nth_value_number(uint8_t nth, uint32_t number) {
		if (nth < 10) {
			return _set_number('0' + nth, number);
		}
		return false;
	}


    /**
     * @brief Set nth value as string
     * @param nth Index 0-9
     * @param in_string String value
     * @return true if successful
     */
	bool set_nth_value_string(uint8_t nth, const char* in_string) {
		if (nth < 10) {
			return _set_string('0' + nth, in_string);
		}
		return false;
	}


    /**
     * @brief Set as a No Reply for `call` messages
	 * 
	 * There will be no echo for this message if it's a `call` one
	 * 
     * @return true if successful
     */
	bool set_no_reply() {
		return _set_number('n', 1);
	}


    /**
     * @brief Swap 'from' and 'to' fields
     * @return true if 'from' field exists
     * 
     * @note Useful for creating replies. If 'to' doesn't exist,
     *       'from' becomes 'to' and 'from' is thus removed.
     */
	bool swap_from_with_to() {
		size_t key_from_position = _get_key_position('f');
		size_t key_to_position = _get_key_position('t');
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
