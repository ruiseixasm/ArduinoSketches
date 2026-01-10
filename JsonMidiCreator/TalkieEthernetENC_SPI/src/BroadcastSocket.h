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
 * @file BroadcastSocket.h
 * @brief Broadcast Socket interface for Talkie communication protocol
 * 
 * This class provides efficient, memory-safe input and output for the
 * JSONmessages which processing is started and finished by it.
 * 
 * @warning This class does not use dynamic memory allocation.
 *          All operations are performed on fixed-size buffers.
 * 
 * @section constraints Memory Constraints
 * - Maximum buffer size: TALKIE_BUFFER_SIZE (default: 128 bytes)
 * 
 * @author Rui Seixas Monteiro
 * @date Created: 2026-01-03
 * @version 1.0.0
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


/**
 * @class BroadcastSocket
 * @brief An Interface to be implemented as a Socket to receive and send its buffer
 * 
 * The implementation of this class requires de definition of the methods, _receive,
 * _send and class_name. After receiving data, the method _startTransmission
 * shall be called.
 * 
 * @note This class has a received and a sending buffer already.
 */
class BroadcastSocket {
protected:

	MessageRepeater* _message_repeater = nullptr;
	LinkType _link_type = LinkType::TALKIE_LT_NONE;

    char _sending_buffer[TALKIE_BUFFER_SIZE];
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


    /**
     * @brief Allows the overriding Socket class to peek at the
	 *        received JSON message
	 * 
	 * This method gives the overriding class the opportunity to
	 * get direct information from the Json Message.
     */
	virtual void _showReceivedMessage(const JsonMessage& json_message) {}

	
    /**
     * @brief Sends the generated message by _startTransmission
	 *        to the Repeater
     */
	void _transmitToRepeater(JsonMessage& json_message);


    /**
     * @brief Starts the transmission of the data received
	 * 
	 * This method creates a new message that is shown to the socket
	 * implementation via the _showReceivedMessage method and then sends
	 * it to the Repeater via the method _transmitToRepeater.
     * 
     * @note This method resets the _received_length to 0.
     */
    void _startTransmission(JsonMessage& json_message) {

		if (json_message._validate_checksum()) {	// It also removes the 'c' field from the json_message

			#ifdef MESSAGE_DEBUG_TIMING
			Serial.print("\n\t");
			Serial.print(class_name());
			Serial.print(": ");
			#endif
				
			#ifdef BROADCASTSOCKET_DEBUG_NEW
			Serial.print(F("\thandleTransmission1.1: "));
			json_message.write_to(Serial);
			Serial.print(" | ");
			Serial.println(json_message._get_length());
			#endif

			
			#ifdef BROADCASTSOCKET_DEBUG
			Serial.print(F("handleTransmission4: Validated Checksum of "));
			Serial.println(checksum);
			#endif
			
			if (_max_delay_ms > 0) {

				MessageValue message_code = json_message.get_message_value();
				if (message_code == MessageValue::TALKIE_MSG_CALL) {

					uint16_t message_timestamp = json_message.get_timestamp();

					#ifdef BROADCASTSOCKET_DEBUG
					Serial.print(F("handleTransmission6: Message code requires delay check: "));
					Serial.println((int)message_code);
					#endif

					#ifdef BROADCASTSOCKET_DEBUG
					Serial.print(F("handleTransmission3: Remote time: "));
					Serial.println(message_timestamp);
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
								return;  // Out of time package (too late)
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
			
			_showReceivedMessage(json_message); // Gives a chance to show it before transmitting
			_transmitToRepeater(json_message);
			
			#ifdef MESSAGE_DEBUG_TIMING
			Serial.print(" | ");
			Serial.print(millis() - json_message._reference_time);
			#endif

		// Has to report an error
		} else {
			// Mark error message as noise and dispatch it to be processed by the respective Talker
			json_message.set_message_value(MessageValue::TALKIE_MSG_NOISE);
			json_message.set_error_value(ErrorValue::TALKIE_ERR_CHECKSUM);
			_showReceivedMessage(json_message); // Gives a chance to show it before transmitting
			_transmitToRepeater(json_message);
		}
    }


    /**
     * @brief Lets the overriding class to green light the access to the sending buffer
	 * 
	 * This method is intended to be used by slave devices that works with the sending
	 * buffer asynchronously, like the case of the Arduino SPI in slave mode.
     * 
     * @note This method returns true whenever the sending buffer is available.
     */
	virtual bool _unlockSendingBuffer() { return true; }


    /**
     * @brief Pure abstract method that triggers the receiving data by the socket
	 * 
     * @note This method shall also trigger the method _startTransmission.
     */
    virtual void _receive() = 0;


	/**
     * @brief Pure abstract method that sends via socket any received json message
     * @param json_message A json message able to be accessed by the subclass socket
	 * 
     * @note This method marks the end of the message cycle with _finishTransmission.
     */
    virtual bool _send(const JsonMessage& json_message) = 0;


public:
    // Delete copy/move operations
    BroadcastSocket(const BroadcastSocket&) = delete;
    BroadcastSocket& operator=(const BroadcastSocket&) = delete;
    BroadcastSocket(BroadcastSocket&&) = delete;
    BroadcastSocket& operator=(BroadcastSocket&&) = delete;

	
	/** @brief A getter for the class name to be returned for the `system` command */
    virtual const char* class_name() const = 0;

	
	/**
     * @brief Method intended to be called by the Repeater class by its public loop method
	 * 
     * @note This method being underscored means to be called internally only.
     */
    virtual void _loop() {
        // In theory, a UDP packet on a local area network (LAN) could survive
        // for about 4.25 minutes (255 seconds).
        // BUT in practice it won't more that 256 milliseconds given that is a Ethernet LAN
        if (_control_timing && (uint16_t)millis() - _last_local_time > MAX_NETWORK_PACKET_LIFETIME_MS) {
            _control_timing = false;
        }
        _receive();
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
	void _setLink(MessageRepeater* message_repeater, LinkType link_type);


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
	

	/**
     * @brief The final step in a cycle of processing a json message in which the
	 *        json message content is loaded into the sending buffer of the socket
	 *        to be sent
     * @param json_message A json message to be serialized into the sending buffer
	 * 
     * @note This method marks the end of the message cycle.
     */
    bool _finishTransmission(JsonMessage& json_message) {

		bool message_sent = false;

		#ifdef BROADCASTSOCKET_DEBUG_NEW
		Serial.print(F("socketSend1: "));
		json_message.write_to(Serial);
		Serial.println();  // optional: just to add a newline after the JSON
		#endif

		// Before writing on the _sending_buffer it needs the final processing and then waits for buffer availability
		if (_unlockSendingBuffer()) {

			#ifdef BROADCASTSOCKET_DEBUG_NEW
			Serial.print(F("socketSend2: "));
			json_message.write_to(Serial);
			Serial.println();  // optional: just to add a newline after the JSON
			#endif

			_sending_length = json_message.serialize_json(_sending_buffer, TALKIE_BUFFER_SIZE);

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
				
			if (json_message._get_length() > 0) {
				
				uint16_t checksum = JsonMessage::_generateChecksum(_sending_buffer, _sending_length);
				JsonMessage::_set_number('c', checksum, _sending_buffer, &_sending_length);

				json_message._insert_checksum();
				message_sent = _send(json_message);

				#ifdef MESSAGE_DEBUG_TIMING
				Serial.print(" | ");
				Serial.print(millis() - json_message._reference_time);
				#endif
			}
		}
		_sending_length = 0;	// Marks sending buffer available even if the send fails
		return message_sent;
    }

};

#endif // BROADCAST_SOCKET_H
