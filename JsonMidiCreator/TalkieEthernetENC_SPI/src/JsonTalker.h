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
 * @file JsonTalker.h
 * @brief JSON message handler for Talkie communication protocol.
 *        This class acts on the received JsonMessage accordingly
 *        to its manifesto.
 * 
 * This class provides efficient, memory-safe JSON message manipulation 
 * for embedded systems with constrained resources. It implements a 
 * schema-driven JSON protocol optimized for Arduino environments.
 * 
 * @warning This class uses messages of the type JsonMessage.
 * 
 * @author Rui Seixas Monteiro
 * @date Created: 2026-01-03
 * @version 1.0.0
 */

#ifndef JSON_TALKER_H
#define JSON_TALKER_H

#include <Arduino.h>        // Needed for Serial given that Arduino IDE only includes Serial in .ino files!
#include "TalkerManifesto.hpp"
#include "TalkieCodes.hpp"
#include "JsonMessage.hpp"
#include "BroadcastSocket.h"


// #define JSON_TALKER_DEBUG
// #define JSON_TALKER_DEBUG_NEW

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
class BroadcastSocket;


class JsonTalker {
protected:
    
	MessageRepeater* _message_repeater = nullptr;
	LinkType _link_type = LinkType::TALKIE_LT_NONE;

    const char* _name;      // Name of the Talker
    const char* _desc;      // Description of the Device
	TalkerManifesto* _manifesto = nullptr;
    uint8_t _channel = 255;	// Channel 255 means NO channel response
	Original _original_message = {0, MessageValue::TALKIE_MSG_NOISE};
    bool _muted_calls = false;


	static const char* _board_description() {
		
		#ifdef __AVR__
			#if (RAMEND - RAMSTART + 1) == 2048
				return "Arduino Uno/Nano (ATmega328P)";
			#elif (RAMEND - RAMSTART + 1) == 8192
				return "Arduino Mega (ATmega2560)";
			#else
				return "Unknown AVR Board";
			#endif
			
		#elif defined(ESP8266)
			static char buffer[50];
			snprintf(buffer, sizeof(buffer), "ESP8266 (Chip ID: %u)", ESP.getChipId());
			return buffer;
			
		#elif defined(ESP32)
			static char buffer[50];
			snprintf(buffer, sizeof(buffer), "ESP32 (Rev: %d)", ESP.getChipRevision());
			return buffer;
			
		#elif defined(TEENSYDUINO)
			#if defined(__IMXRT1062__)
				return "Teensy 4.0/4.1 (i.MX RT1062)";
			#elif defined(__MK66FX1M0__)
				return "Teensy 3.6 (MK66FX1M0)";
			#elif defined(__MK64FX512__)
				return "Teensy 3.5 (MK64FX512)";
			#elif defined(__MK20DX256__)
				return "Teensy 3.2/3.1 (MK20DX256)";
			#elif defined(__MKL26Z64__)
				return "Teensy LC (MKL26Z64)";
			#else
				return "Unknown Teensy Board";
			#endif

		#elif defined(__arm__)
			return "ARM-based Board";

		#else
			return "Unknown Board";

		#endif
	}


	bool _prepareMessage(JsonMessage& json_message) {

		if (json_message.has_from()) {
			if (strcmp(json_message.get_from_name(), _name) != 0) {
				// FROM is different from _name, must be swapped (replaces "f" with "t")
				json_message.swap_from_with_to();
				json_message.set_from_name(_name);
			}
		} else {
			// FROM doesn't even exist (must have)
			json_message.set_from_name(_name);
		}

		MessageValue message_value = json_message.get_message_value();
		if (message_value < MessageValue::TALKIE_MSG_ECHO) {

			#ifdef JSON_TALKER_DEBUG
			Serial.print(F("socketSend1: Setting a new identifier (i) for :"));
			json_message.write_to(Serial);
			Serial.println();  // optional: just to add a newline after the JSON
			#endif

			uint16_t message_id = (uint16_t)millis();
			if (message_value < MessageValue::TALKIE_MSG_ECHO) {
				_original_message.identity = message_id;
				_original_message.message_value = message_value;
			}
			json_message.set_identity(message_id);
		} else if (!json_message.has_identity()) { // Makes sure response messages have an "i" (identifier)

			#ifdef JSON_TALKER_DEBUG
			Serial.print(F("socketSend1: Response message with a wrong or without an identifier, now being set (i): "));
			json_message.write_to(Serial);
			Serial.println();  // optional: just to add a newline after the JSON
			#endif

			json_message.set_message_value(MessageValue::TALKIE_MSG_ERROR);
			json_message.set_identity();
			json_message.set_nth_value_number(0, static_cast<uint32_t>(ErrorValue::TALKIE_ERR_IDENTITY));

		} else {
			
			#ifdef JSON_TALKER_DEBUG
			Serial.print(F("socketSend1: Keeping the same identifier (i): "));
			json_message.write_to(Serial);
			Serial.println();  // optional: just to add a newline after the JSON
			#endif

		}
		return true;
	}

	
	bool transmissionSockets(JsonMessage& json_message);
	bool transmissionDrops(JsonMessage& json_message);
	bool transmissionDelays(JsonMessage& json_message);
	bool setSocketDelay(uint8_t socket_index, uint8_t delay_value) const;


public:

    // Explicitly disabled the default constructor
    JsonTalker() = delete;
        
    JsonTalker(const char* name, const char* desc, TalkerManifesto* manifesto = nullptr, uint8_t channel = 255)
        : _name(name), _desc(desc), _manifesto(manifesto), _channel(channel) {}

    void _loop() {
        if (_manifesto) {
            _manifesto->_loop(this);
        }
    }


    // ============================================
    // GETTERS - FIELD VALUES
    // ============================================
	
    /**
	 * @brief Get the name of the Talker
     * @return A pointer to the Talker name string
     */
	const char* get_name() const { return _name; }
	
	
    /**
	 * @brief Get the description of the Talker
	 * @return A pointer to the Talker description string
     */
	const char* get_desc() const { return _desc; }

	
    /**
     * @brief Get the channel of the Talker
     * @return The channel number of the Talker
     */
	uint8_t get_channel() const { return _channel; }
	
	
    /**
     * @brief Get the muted state of the Talker
     * @return Returns true if muted (muted calls)
     * 
     * @note This only mutes the echoes from the calls
     */
	bool get_muted() const { return _muted_calls; }


    /**
     * @brief Get the Link Type with the Message Repeater
     * @return Returns the Link Type (ex. DOWN_LINKED)
     */
	LinkType getLinkType() const { return _link_type; }

	
    /**
     * @brief Get the last, non echo message (original)
     * @return Returns Original with the message id and value
     * 
     * @note This is used to pair the message id with its echo
     */
    const Original& get_original() const { return _original_message; }


    // ============================================
    // SETTERS - FIELD MODIFICATION
    // ============================================

    /**
     * @brief Set channel number
     * @param channel Channel number for which the Talker will respond
     */
    void set_channel(uint8_t channel) { _channel = channel; }


    /**
     * @brief Intended to be used by the Message Repeater only
     * @param message_repeater The Message Repeater pointer
     * @param link_type The Link Type with the Message Repeater
     * 
     * @note This method is used by the Message Repeater to set up the Talker
     */
	void _setLink(MessageRepeater* message_repeater, LinkType link_type);
	

    /**
     * @brief Sets the Link Type of the Talker directly
     * @param link_type The Link Type with the Message Repeater
     * 
     * @note Only usefull if intended to be disabled (ex. NONE)
     */
	void setLinkType(LinkType link_type) { _link_type = link_type; }
	

    /**
     * @brief Set the Talker as muted or not muted
     * @param muted If true it mutes the call's echoes
     * 
     * @note This only mutes the echoes from the calls
     */
    void set_mute(bool muted) { _muted_calls = muted; }


	bool transmitToRepeater(JsonMessage& json_message);
	
    
    bool _handleTransmission(JsonMessage& json_message, TalkerMatch talker_match) {

		MessageValue message_value = json_message.get_message_value();

		#ifdef JSON_TALKER_DEBUG_NEW
		Serial.print(F("\t\thandleTransmission1: "));
		json_message.write_to(Serial);
		Serial.print(" | ");
		Serial.println(static_cast<int>( message_value ));
		#endif

        switch (message_value) {

			case MessageValue::TALKIE_MSG_CALL:
				json_message.set_message_value(MessageValue::TALKIE_MSG_ECHO);
				{
					if (_manifesto) {
						uint8_t index_found_i = 255;
						ValueType value_type = json_message.get_action_type();
						switch (value_type) {

							case ValueType::TALKIE_VT_STRING:
								index_found_i = _manifesto->_actionIndex(json_message.get_action_string());
								break;
							
							case ValueType::TALKIE_VT_INTEGER:
								index_found_i = _manifesto->_actionIndex(json_message.get_action_number());
								break;
							
							default: break;
						}
						if (index_found_i < 255) {

							#ifdef JSON_TALKER_DEBUG
							Serial.print(F("\tRUN found at "));
							Serial.print(index_found_i);
							Serial.println(F(", now being processed..."));
							#endif

							// ROGER should be implicit for CALL to spare json string size for more data index value nth
							if (!_manifesto->_actionByIndex(index_found_i, *this, json_message, talker_match)) {
								json_message.set_roger_value(RogerValue::TALKIE_RGR_NEGATIVE);
							}
						} else {
							json_message.set_roger_value(RogerValue::TALKIE_RGR_SAY_AGAIN);
						}
					} else {
						json_message.set_roger_value(RogerValue::TALKIE_RGR_NO_JOY);
					}
					// In the end sends back the processed message (single message, one-to-one)
					if (!_muted_calls) transmitToRepeater(json_message);
				}
				break;
			
			case MessageValue::TALKIE_MSG_TALK:
				json_message.set_message_value(MessageValue::TALKIE_MSG_ECHO);
				json_message.set_nth_value_string(0, _desc);
				// In the end sends back the processed message (single message, one-to-one)
				transmitToRepeater(json_message);
				break;
			
			case MessageValue::TALKIE_MSG_CHANNEL:
				json_message.set_message_value(MessageValue::TALKIE_MSG_ECHO);
				if (json_message.has_nth_value_number(0)) {

					#ifdef JSON_TALKER_DEBUG
					Serial.print(F("\tChannel B value is an <uint8_t>: "));
					Serial.println(json_message.get_nth_value_number(0));
					#endif

					_channel = json_message.get_nth_value_number(0);
				}
				json_message.set_nth_value_number(0, _channel);
				// In the end sends back the processed message (single message, one-to-one)
				transmitToRepeater(json_message);
				break;
			
			case MessageValue::TALKIE_MSG_PING:
				json_message.set_message_value(MessageValue::TALKIE_MSG_ECHO);
				// Talker name already set in FROM (ready to transmit)
				transmitToRepeater(json_message);
				break;
			
			case MessageValue::TALKIE_MSG_LIST:
				json_message.set_message_value(MessageValue::TALKIE_MSG_ECHO);
				{   // Because of action_index and action !!!

					uint8_t action_index = 0;
					if (_manifesto) {
						const TalkerManifesto::Action* action;
						_manifesto->_iterateActionsReset();
						while ((action = _manifesto->_iterateActionNext()) != nullptr) {	// No boilerplate
							json_message.set_nth_value_number(0, action_index++);
							json_message.set_nth_value_string(1, action->name);
							json_message.set_nth_value_string(2, action->desc);
							transmitToRepeater(json_message);	// Many-to-One
						}
						if (!action_index) {
							json_message.set_roger_value(RogerValue::TALKIE_RGR_NIL);
							transmitToRepeater(json_message);	// One-to-One
						}
					} else {
						json_message.set_roger_value(RogerValue::TALKIE_RGR_NO_JOY);
						transmitToRepeater(json_message);		// One-to-One
					}
				}
				break;
			
			case MessageValue::TALKIE_MSG_SYSTEM:
				json_message.set_message_value(MessageValue::TALKIE_MSG_ECHO);
				if (json_message.has_system()) {

					SystemValue system_value = json_message.get_system_value();

					switch (system_value) {

						case SystemValue::TALKIE_SYS_BOARD:
							json_message.set_nth_value_string(0, _board_description());
							break;

						case SystemValue::TALKIE_SYS_MUTE:
							if (json_message.has_nth_value_number(0)) {
								uint8_t mute = (uint8_t)json_message.get_nth_value_number(0);
								if (mute) {
									_muted_calls = true;
								} else {
									_muted_calls = false;
								}
							} else {
								if (_muted_calls) {
									json_message.set_nth_value_number(0, 1);
								} else {
									json_message.set_nth_value_number(0, 0);
								}
							}
							break;

						case SystemValue::TALKIE_SYS_DROPS:
							if (!transmissionDrops(json_message)) {
								json_message.set_roger_value(RogerValue::TALKIE_RGR_NO_JOY);
							} else {
        						return true;	// Avoids extra transmissions sends
							}
							break;

						case SystemValue::TALKIE_SYS_DELAY:
							if (json_message.get_nth_value_type(0) == ValueType::TALKIE_VT_INTEGER && json_message.get_nth_value_type(1) == ValueType::TALKIE_VT_INTEGER) {
								if (!setSocketDelay((uint8_t)json_message.get_nth_value_number(0), (uint8_t)json_message.get_nth_value_number(1))) {
									json_message.remove_nth_value(0);
									json_message.remove_nth_value(1);
									json_message.set_roger_value(RogerValue::TALKIE_RGR_NEGATIVE);
								}
							} else {
								if (!transmissionDelays(json_message)) {
									json_message.set_roger_value(RogerValue::TALKIE_RGR_NO_JOY);
								} else {
									return true;	// Avoids extra transmissions sends
								}
							}
							break;

						case SystemValue::TALKIE_SYS_SOCKET:
							if (!transmissionSockets(json_message)) {
								json_message.set_roger_value(RogerValue::TALKIE_RGR_NO_JOY);
							} else {
        						return true;	// Avoids extra transmissions sends
							}
							break;

						case SystemValue::TALKIE_SYS_MANIFESTO:
							if (_manifesto) {
								json_message.set_nth_value_string(0, _manifesto->class_name());
							} else {
								json_message.set_roger_value(RogerValue::TALKIE_RGR_NO_JOY);
							}
							break;

						default: break;
					}

					// In the end sends back the processed message (single message, one-to-one)
					transmitToRepeater(json_message);
				}
				break;
			
			case MessageValue::TALKIE_MSG_ECHO:
				if (_manifesto) {

					// Makes sure it has the same id first (echo condition)
					uint16_t message_id = json_message.get_identity();

					#ifdef JSON_TALKER_DEBUG_NEW
					Serial.print(F("\t\thandleTransmission1: "));
					json_message.write_to(Serial);
					Serial.print(" | ");
					Serial.print(message_id);
					Serial.print(" | ");
					Serial.println(_original_message.identity);
					#endif

					if (message_id == _original_message.identity) {
						_manifesto->_echo(*this, json_message, talker_match);
					}
				}
				break;
			
			case MessageValue::TALKIE_MSG_ERROR:
				if (_manifesto) {
					_manifesto->_error(*this, json_message, talker_match);
				}
				break;
			
			case MessageValue::TALKIE_MSG_NOISE:
				if (json_message.has_error()) {
					if (talker_match == TalkerMatch::TALKIE_MATCH_BY_NAME || talker_match == TalkerMatch::TALKIE_MATCH_BY_CHANNEL) {
						json_message.remove_all_nth_values();	// Keeps it small and clean of bad chars
						json_message.set_message_value(MessageValue::TALKIE_MSG_ERROR);
						if (!json_message.has_identity()) json_message.set_identity();
						transmitToRepeater(json_message);
					}
				} else if (_manifesto) {
					_manifesto->_noise(*this, json_message, talker_match);
				}
				break;
			
			default: return false;
        }
        return true;
    }

};


#endif // JSON_TALKER_H
