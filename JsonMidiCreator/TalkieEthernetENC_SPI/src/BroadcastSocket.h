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
#include "JsonTalker.h"
#include "TalkieCodes.hpp"
#include "JsonMessage.hpp"

#ifndef BROADCAST_SOCKET_BUFFER_SIZE
#define BROADCAST_SOCKET_BUFFER_SIZE 128
#endif
#ifndef NAME_LEN
#define NAME_LEN 16
#endif


// #define BROADCASTSOCKET_DEBUG
// #define BROADCASTSOCKET_DEBUG_NEW

// Readjust if necessary
#define MAX_NETWORK_PACKET_LIFETIME_MS 256UL    // 256 milliseconds

using LinkType = TalkieCodes::LinkType;
using TalkerMatch = TalkieCodes::TalkerMatch;

class MessageRepeater;

class BroadcastSocket {
public:

    static uint16_t generateChecksum(const char* buffer, size_t length) {	// 16-bit word and XORing
        uint16_t checksum = 0;
		if (length <= BROADCAST_SOCKET_BUFFER_SIZE) {
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
	LinkType _link_type = LinkType::UP_LINKED;

    char _received_buffer[BROADCAST_SOCKET_BUFFER_SIZE];
    char _sending_buffer[BROADCAST_SOCKET_BUFFER_SIZE];
	size_t _received_length = 0;
	size_t _sending_length = 0;

    // Pointer PRESERVE the polymorphism while objects don't!
    uint8_t _max_delay_ms = 5;
    bool _control_timing = false;
    uint16_t _last_local_time = 0;
    uint16_t _last_message_timestamp = 0;
    uint16_t _drops_count = 0;


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

    
    bool triggerTalkers() {

		size_t colon_position = JsonMessage::get_colon_position('c', _received_buffer, _received_length);
		uint16_t received_checksum = JsonMessage::get_value_number('c', _received_buffer, _received_length, colon_position);

		#ifdef BROADCASTSOCKET_DEBUG_NEW
		Serial.print(F("\ttriggerTalkers0.1: "));
        Serial.write(_received_buffer, _received_length);
		Serial.print(" | ");
		Serial.println(received_checksum);
		#endif

		if (!JsonMessage::remove('c', _received_buffer, &_received_length, colon_position)) return false;
		uint16_t checksum = generateChecksum(_received_buffer, _received_length);

		#ifdef BROADCASTSOCKET_DEBUG_NEW
		Serial.print(F("\ttriggerTalkers0.2: "));
        Serial.write(_received_buffer, _received_length);
		Serial.print(" | ");
		Serial.println(checksum);
		#endif

		if (received_checksum == checksum) {

			// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
			JsonMessage json_message(_received_buffer, _received_length);

			#ifdef BROADCASTSOCKET_DEBUG_NEW
			Serial.print(F("\ttriggerTalkers1.1: "));
			json_message.write_to(Serial);
			Serial.print(" | ");
			Serial.println(json_message.validate_fields());
			#endif

			if (json_message.validate_fields()) {

				MessageValue message_code = json_message.get_message_value();
				uint16_t message_timestamp = json_message.get_timestamp();
				
				#ifdef BROADCASTSOCKET_DEBUG
				Serial.print(F("triggerTalkers3: Remote time: "));
				Serial.println(message_timestamp);
				#endif

				#ifdef BROADCASTSOCKET_DEBUG
				Serial.print(F("triggerTalkers4: Validated Checksum of "));
				Serial.println(checksum);
				#endif

				if (_max_delay_ms > 0) {

					if (message_code == MessageValue::CALL) {	// Only does time control on Calls (drops)

						#ifdef BROADCASTSOCKET_DEBUG
						Serial.print(F("triggerTalkers6: Message code requires delay check: "));
						Serial.println(message_code_int);
						#endif

						const uint16_t local_time = (uint16_t)millis();
						
						if (_control_timing) {
							
							const uint16_t remote_delay = _last_message_timestamp - message_timestamp;  // Package received after

							if (remote_delay > 0 && remote_delay < MAX_NETWORK_PACKET_LIFETIME_MS) {    // Out of order package
								const uint16_t allowed_delay = static_cast<uint16_t>(_max_delay_ms);
								const uint16_t local_delay = local_time - _last_local_time;
								#ifdef BROADCASTSOCKET_DEBUG
								Serial.print(F("triggerTalkers7: Local delay: "));
								Serial.println(local_delay);
								#endif
								if (remote_delay > allowed_delay || local_delay > allowed_delay) {
									#ifdef BROADCASTSOCKET_DEBUG
									Serial.print(F("triggerTalkers8: Out of time package (remote delay): "));
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

				// Gives a chance to show it one time

				if (!receivedJsonMessage(json_message)) {
					#ifdef JSON_TALKER_DEBUG
					Serial.println(4);
					#endif
					// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
					if (json_message.swap_from_with_to()) {
						json_message.set_message_value(MessageValue::ERROR);
						if (!json_message.has_identity()) {
							json_message.set_identity();
						}
						socketSend(json_message);	// Includes reply swap
					}
					return false;
				}

				transmitToRepeater(json_message);

			} else {
				#ifdef BROADCASTSOCKET_DEBUG
				Serial.print(F("triggerTalkers9: Validation of Checksum FAILED: "));
				Serial.println(checksum);
				#endif
			}
		}
		_received_length = 0;	// Enables new receiving
        return true;
    }

    // Constructor
    BroadcastSocket() {
		// Does nothing here
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


    virtual uint8_t receive() {
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

    virtual const char* class_name() const { return "BroadcastSocket"; }

    virtual void loop() {
        receive();
    }

	void setLink(MessageRepeater* message_repeater, LinkType link_type);

	void setLinkType(LinkType link_type) {
		_link_type = link_type;
	}

	LinkType getLinkType() const {
		return _link_type;
	}

	bool deserialize_buffer(JsonMessage& json_message) const {
		return json_message.deserialize_buffer(_received_buffer, _received_length);
	}


    bool socketSend(const JsonMessage& json_message) {

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

			// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
			_sending_length = json_message.serialize_json(_sending_buffer, BROADCAST_SOCKET_BUFFER_SIZE);
			uint16_t checksum = generateChecksum(_sending_buffer, _sending_length);
			JsonMessage::set_number('c', checksum, _sending_buffer, &_sending_length);

			#ifdef BROADCASTSOCKET_DEBUG_NEW
			Serial.print(F("socketSend3: "));
			Serial.write(_sending_buffer, _sending_length);
			Serial.print(" | ");
			Serial.println(_sending_length);
			#endif

			if (_sending_length) {
				return send(json_message);
			}
		}
		return false;
    }
    

    void set_max_delay(uint8_t max_delay_ms = 5) { _max_delay_ms = max_delay_ms; }
    uint8_t get_max_delay() { return _max_delay_ms; }
    uint16_t get_drops_count() { return _drops_count; }

};




#endif // BROADCAST_SOCKET_H
