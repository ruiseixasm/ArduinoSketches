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
#ifndef BROADCAST_SOCKET_H
#define BROADCAST_SOCKET_H

#include <Arduino.h>    // Needed for Serial given that Arduino IDE only includes Serial in .ino files!
#include "TalkieCodes.hpp"
#include "JsonMessage.hpp"


// #define BROADCASTSOCKET_DEBUG
// #define BROADCASTSOCKET_DEBUG_NEW

// Readjust if necessary
#define MAX_NETWORK_PACKET_LIFETIME_MS 256UL    // 256 milliseconds

using LinkType			= TalkieCodes::LinkType;
using TalkerMatch 		= TalkieCodes::TalkerMatch;
using BroadcastValue 	= TalkieCodes::BroadcastValue;
using MessageValue 		= TalkieCodes::MessageValue;
using SystemValue 		= TalkieCodes::SystemValue;
using RogerValue 		= TalkieCodes::RogerValue;
using ErrorValue 		= TalkieCodes::ErrorValue;
using ValueType 		= TalkieCodes::ValueType;
using Original 			= JsonMessage::Original;

class MessageRepeater;

class BroadcastSocket {
public:

    static uint16_t _generateChecksum(const char* buffer, size_t length) {	// 16-bit word and XORing
        uint16_t checksum = 0;
		if (length <= TALKIE_BUFFER_SIZE) {
			for (size_t i = 0; i < length; i += 2) {
				uint16_t chunk = buffer[i] << 8;
				if (i + 1 < length) {
					chunk |= buffer[i + 1];
				}
				checksum ^= chunk;
			}
		}
        return checksum;
    }


protected:

	MessageRepeater* _message_repeater = nullptr;
	LinkType _link_type = LinkType::TALKIE_LT_NONE;

    char _received_buffer[TALKIE_BUFFER_SIZE];
    char _sending_buffer[TALKIE_BUFFER_SIZE];
	size_t _received_length = 0;
	size_t _sending_length = 0;

    // Pointer PRESERVE the polymorphism while objects don't!
    uint8_t _max_delay_ms = 5;
    bool _control_timing = false;
    uint16_t _last_local_time = 0;
    uint16_t _last_message_timestamp = 0;
    uint16_t _drops_count = 0;

	
    // Constructor
    BroadcastSocket() {
		// Does nothing here
	}


	// Allows the overriding class to peek at the received JSON message
	virtual bool receivedJsonMessage(const JsonMessage& json_message) {
		
		return true;
	}

	// Allows the overriding class to peek after processing of the JSON message
	virtual bool processedJsonMessage(const JsonMessage& json_message) {
        (void)json_message;	// Silence unused parameter warning

		return true;
	}

	
	bool transmitToRepeater(JsonMessage& json_message);

    
    bool startTransmission() {

		// Trim trailing newline and carriage return characters or any other that isn't '}'
		while (_received_length > 26 
			&& (_received_buffer[_received_length - 1] != '}' || _received_buffer[_received_length - 2] == '\\')) {
			_received_length--;	// Note that literals add the '\0'!
		}

		// Minimum length: '{"m":0,"b":0,"i":0,"f":"n"}' = 27
		if (_received_length < 27) {
			_received_length = 0;
			return false;
		}

		if (_received_buffer[0] != '{') {
			_received_length = 0;
			return false;	
		}	

		size_t colon_position = JsonMessage::get_colon_position('c', _received_buffer, _received_length);
		uint16_t received_checksum = JsonMessage::get_value_number('c', _received_buffer, _received_length, colon_position);

		#ifdef BROADCASTSOCKET_DEBUG_NEW
		Serial.print(F("\thandleTransmission0.1: "));
        Serial.write(_received_buffer, _received_length);
		Serial.print(" | ");
		Serial.println(received_checksum);
		#endif

		if (!JsonMessage::_remove('c', _received_buffer, &_received_length, colon_position)) return false;
		uint16_t checksum = _generateChecksum(_received_buffer, _received_length);

		#ifdef BROADCASTSOCKET_DEBUG_NEW
		Serial.print(F("\thandleTransmission0.2: "));
        Serial.write(_received_buffer, _received_length);
		Serial.print(" | ");
		Serial.println(checksum);
		#endif

		if (received_checksum == checksum) {

			#ifdef MESSAGE_DEBUG_TIMING
			Serial.print("\n\t");
			Serial.print(class_name());
			Serial.print(": ");
			#endif
				
			JsonMessage json_message(_received_buffer, _received_length);

			#ifdef BROADCASTSOCKET_DEBUG_NEW
			Serial.print(F("\thandleTransmission1.1: "));
			json_message.write_to(Serial);
			Serial.print(" | ");
			Serial.println(json_message.validate_fields());
			#endif

			if (json_message.validate_fields()) {

				MessageValue message_code = json_message.get_message_value();
				uint16_t message_timestamp = json_message.get_timestamp();
				
				#ifdef BROADCASTSOCKET_DEBUG
				Serial.print(F("handleTransmission3: Remote time: "));
				Serial.println(message_timestamp);
				#endif

				#ifdef BROADCASTSOCKET_DEBUG
				Serial.print(F("handleTransmission4: Validated Checksum of "));
				Serial.println(checksum);
				#endif

				if (_max_delay_ms > 0) {

					if (message_code == MessageValue::TALKIE_MSG_CALL) {	// Only does time control on Calls (drops)

						#ifdef BROADCASTSOCKET_DEBUG
						Serial.print(F("handleTransmission6: Message code requires delay check: "));
						Serial.println(message_code_int);
						#endif

						const uint16_t local_time = (uint16_t)millis();
						
						if (_control_timing) {
							
							const uint16_t remote_delay = _last_message_timestamp - message_timestamp;  // Package received after

							if (remote_delay > 0 && remote_delay < MAX_NETWORK_PACKET_LIFETIME_MS) {    // Out of order package
								const uint16_t allowed_delay = static_cast<uint16_t>(_max_delay_ms);
								const uint16_t local_delay = local_time - _last_local_time;
								#ifdef BROADCASTSOCKET_DEBUG
								Serial.print(F("handleTransmission7: Local delay: "));
								Serial.println(local_delay);
								#endif
								if (remote_delay > allowed_delay || local_delay > allowed_delay) {
									#ifdef BROADCASTSOCKET_DEBUG
									Serial.print(F("handleTransmission8: Out of time package (remote delay): "));
									Serial.println(remote_delay);
									#endif
									_drops_count++;
									return false;  // Out of time package (too late)
								}
							}
						}
						_last_local_time = local_time;
						_last_message_timestamp = message_timestamp;
						_control_timing = true;
					}
				}

				
				#ifdef MESSAGE_DEBUG_TIMING
				Serial.print(millis() - json_message._reference_time);
				#endif
				
				// Gives a chance to show it one time
				if (!receivedJsonMessage(json_message)) {
					#ifdef JSON_TALKER_DEBUG
					Serial.println(4);
					#endif
					if (json_message.swap_from_with_to()) {
						json_message.set_message_value(MessageValue::TALKIE_MSG_ERROR);
						if (!json_message.has_identity()) {
							json_message.set_identity();
						}
						finishTransmission(json_message);	// Includes reply swap
					}
					return false;
				}

				transmitToRepeater(json_message);
				
				#ifdef MESSAGE_DEBUG_TIMING
				Serial.print(" | ");
				Serial.print(millis() - json_message._reference_time);
				#endif
				

			} else {
				#ifdef BROADCASTSOCKET_DEBUG
				Serial.print(F("handleTransmission9: Validation of Checksum FAILED: "));
				Serial.println(checksum);
				#endif
			}
		}
		_received_length = 0;	// Enables new receiving
        return true;
    }


	virtual bool availableReceivingBuffer(uint8_t wait_seconds = 3) {
		uint16_t start_waiting = (uint16_t)millis();
		while (_received_length) {
			if ((uint16_t)millis() - start_waiting > 1000 * wait_seconds) {
				return false;
			}
		}
		return true;
	}

	virtual bool availableSendingBuffer(uint8_t wait_seconds = 3) {
		uint16_t start_waiting = (uint16_t)millis();
		while (_sending_length) {
			if ((uint16_t)millis() - start_waiting > 1000 * wait_seconds) {

				#ifdef BROADCASTSOCKET_DEBUG
				Serial.println(F("\tavailableSendingBuffer: NOT available sending buffer"));
				#endif

				return false;
			}
		}
		
		#ifdef BROADCASTSOCKET_DEBUG
		Serial.println(F("\tavailableSendingBuffer: Available sending buffer"));
		#endif

		return true;
	}


    virtual bool send(const JsonMessage& json_message) {
        (void)json_message;	// Silence unused parameter warning
		
        return true;
    }


    virtual size_t receive() {
        // In theory, a UDP packet on a local area network (LAN) could survive
        // for about 4.25 minutes (255 seconds).
        // BUT in practice it won't more that 256 milliseconds given that is a Ethernet LAN
        if (_control_timing && (uint16_t)millis() - _last_local_time > MAX_NETWORK_PACKET_LIFETIME_MS) {
            _control_timing = false;
        }
        return 0;
    }


public:
    // Delete copy/move operations
    BroadcastSocket(const BroadcastSocket&) = delete;
    BroadcastSocket& operator=(const BroadcastSocket&) = delete;
    BroadcastSocket(BroadcastSocket&&) = delete;
    BroadcastSocket& operator=(BroadcastSocket&&) = delete;

	// The subclass must have the class name defined (pure virtual)
    virtual const char* class_name() const = 0;

    virtual void loop() {
        receive();
    }


    // ============================================
    // GETTERS - FIELD VALUES
    // ============================================
	
    /**
     * @brief Get the Link Type with the Message Repeater
     * @return Returns the Link Type (ex. UP_LINKED)
	 * 
     * @note Usefull if intended to be bridged (ex. UP_BRIDGED),
	 *       where the `LOCAL` messages are also broadcasted
     */
	LinkType getLinkType() const { return _link_type; }

	
    /**
     * @brief Get the maximum amount of delay a message can have before being dropped
     * @return Returns the delay in microseconds
     * 
     * @note A max delay of `0` means no message will be dropped,
	 *       this only applies to `CALL` messages value
     */
    uint8_t get_max_delay() const { return _max_delay_ms; }


    /**
     * @brief Get the total amount of call messages already dropped
     * @return Returns the number of dropped call messages
     */
    uint16_t get_drops_count() const { return _drops_count; }


    // ============================================
    // SETTERS - FIELD MODIFICATION
    // ============================================

    /**
     * @brief Intended to be used by the Message Repeater only
     * @param message_repeater The Message Repeater pointer
     * @param link_type The Link Type with the Message Repeater
     * 
     * @note This method is used by the Message Repeater to set up the Socket
     */
	void setLink(MessageRepeater* message_repeater, LinkType link_type);


    /**
     * @brief Sets the Link Type of the Talker directly
     * @param link_type The Link Type with the Message Repeater
     * 
     * @note Only usefull if intended to be bridged (ex. UP_BRIDGED),
	 *       where the `LOCAL` messages are also broadcasted
     */
	void setLinkType(LinkType link_type) { _link_type = link_type; }


    /**
     * @brief Sets the maximum amount of delay a message can have before being dropped
     * @param max_delay_ms The maximum amount of delay in milliseconds
     * 
     * @note A max delay of `0` means no message will be dropped,
	 *       this only applies to `CALL` messages value
     */
    void set_max_delay(uint8_t max_delay_ms = 5) { _max_delay_ms = max_delay_ms; }
	


	
	bool deserialize_buffer(JsonMessage& json_message) const {
		return json_message.deserialize_buffer(_received_buffer, _received_length);
	}


    bool finishTransmission(const JsonMessage& json_message) {

		#ifdef BROADCASTSOCKET_DEBUG_NEW
		Serial.print(F("socketSend1: "));
		json_message.write_to(Serial);
		Serial.println();  // optional: just to add a newline after the JSON
		#endif

		// Before writing on the _sending_buffer it needs the final processing and then waits for buffer availability
		if (json_message.validate_fields() && processedJsonMessage(json_message) && availableSendingBuffer()) {

			#ifdef BROADCASTSOCKET_DEBUG_NEW
			Serial.print(F("socketSend2: "));
			json_message.write_to(Serial);
			Serial.println();  // optional: just to add a newline after the JSON
			#endif

			_sending_length = json_message.serialize_json(_sending_buffer, TALKIE_BUFFER_SIZE);
			uint16_t checksum = _generateChecksum(_sending_buffer, _sending_length);
			JsonMessage::_set_number('c', checksum, _sending_buffer, &_sending_length);

			#ifdef BROADCASTSOCKET_DEBUG_NEW
			Serial.print(F("socketSend3: "));
			Serial.write(_sending_buffer, _sending_length);
			Serial.print(" | ");
			Serial.println(_sending_length);
			#endif

			#ifdef MESSAGE_DEBUG_TIMING
			Serial.print(" | ");
			Serial.print(millis() - json_message._reference_time);
			#endif
				
			if (_sending_length) {
				
				#ifdef MESSAGE_DEBUG_TIMING
				bool send_result = send(json_message);
				Serial.print(" | ");
				Serial.print(millis() - json_message._reference_time);
				return send_result;
				#else
				return send(json_message);
				#endif
				
			}
		}
		return false;
    }
    
};




#endif // BROADCAST_SOCKET_H
