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
#ifndef JSON_TALKER_H
#define JSON_TALKER_H

#include <Arduino.h>        // Needed for Serial given that Arduino IDE only includes Serial in .ino files!
#include "TalkerManifesto.hpp"
#include "TalkieCodes.hpp"
#include "JsonMessage.hpp"
#include "BroadcastSocket.h"


#ifndef BROADCAST_SOCKET_BUFFER_SIZE
#define BROADCAST_SOCKET_BUFFER_SIZE 128
#endif
#ifndef NAME_LEN
#define NAME_LEN 16
#endif

// #define JSON_TALKER_DEBUG
// #define JSON_TALKER_DEBUG_NEW

using LinkType = TalkieCodes::LinkType;
using BroadcastValue = TalkieCodes::BroadcastValue;
using MessageValue = TalkieCodes::MessageValue;
using InfoValue = TalkieCodes::InfoValue;
using RogerValue = TalkieCodes::RogerValue;
using ErrorValue = TalkieCodes::ErrorValue;
using Original = TalkerManifesto::Original;
using ValueType = JsonMessage::ValueType;


class MessageRepeater;
class BroadcastSocket;


class JsonTalker {
protected:
    
	MessageRepeater* _message_repeater = nullptr;
	LinkType _link_type = LinkType::TALKIE_DOWN_LINKED;

    const char* _name;      // Name of the Talker
    const char* _desc;      // Description of the Device
	TalkerManifesto* _manifesto = nullptr;
    uint8_t _channel = 0;
	MessageValue _received_message = MessageValue::TALKIE_MSG_NOISE;
	Original _original_message = {0, MessageValue::TALKIE_MSG_NOISE};
    bool _muted_calls = false;

public:

    // Explicit default constructor
    JsonTalker() = delete;
        
    JsonTalker(const char* name, const char* desc, TalkerManifesto* manifesto = nullptr)
        : _name(name), _desc(desc), _manifesto(manifesto) {
		// AVOIDS EVERY TALKER WITH THE SAME CHANNEL
		// XOR is great for 8-bit mixing
		_channel ^= strlen(this->class_name()) << 1;    // Shift before XOR
		_channel ^= strlen(_name) << 2;
		_channel ^= strlen(_desc) << 3;
		// Add microsecond LSB for entropy
		_channel ^= micros() & 0xFF;
		if (_manifesto) {	// Safe code
			_channel ^= strlen(_manifesto->class_name()) << 4;
			_channel = _manifesto->getChannel(_channel, this);
		}
    }

    const char* class_name() const { return "JsonTalker"; }


    void loop() {
        if (_manifesto) {
            _manifesto->loop(this);
        }
    }

	// Getter and setters

	void setLink(MessageRepeater* message_repeater, LinkType link_type);

	void setLinkType(LinkType link_type) {
		_link_type = link_type;
	}

	LinkType getLinkType() const {
		return _link_type;
	}


    void setSocket(BroadcastSocket* socket);
    BroadcastSocket& getSocket();

	const char* socket_class_name();

    const char* get_name() const { return _name; }
    const char* get_desc() const { return _desc; }
    void set_channel(uint8_t channel) { _channel = channel; }
    uint8_t get_channel() const { return _channel; }
    const Original& get_original() const { return _original_message; }
    
    JsonTalker& mute() {    // It does NOT make a copy!
        _muted_calls = true;
        return *this;
    }

    JsonTalker& unmute() {
        _muted_calls = false;
        return *this;
    }

    bool muted() { return _muted_calls; }

    void set_delay(uint8_t delay);
    uint8_t get_delay();
    uint16_t get_drops();
	


	bool prepareMessage(JsonMessage& json_message) {

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

			// _muted_calls mutes CALL echoes only
			if (_muted_calls && _received_message == MessageValue::TALKIE_MSG_CALL) {
				_received_message = MessageValue::TALKIE_MSG_NOISE;	// Avoids false mutes for self generated messages (safe code)
				return false;
			} else {
				_received_message = MessageValue::TALKIE_MSG_NOISE;	// Avoids false mutes for self generated messages (safe code)
			}

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

	

	bool transmitToRepeater(JsonMessage& json_message);

	bool transmitSockets(JsonMessage& json_message);
	bool transmitDrops(JsonMessage& json_message);
	bool transmitDelays(JsonMessage& json_message);

    
    TalkerMatch talkerReceive(JsonMessage& json_message) {

        TalkerMatch talker_match = TalkerMatch::TALKIE_MATCH_NONE;

		MessageValue message_value = json_message.get_message_value();

		#ifdef JSON_TALKER_DEBUG_NEW
		Serial.print(F("\t\ttalkerReceive1: "));
		json_message.write_to(Serial);
		Serial.print(" | ");
		Serial.println(static_cast<int>( message_value ));
		#endif

		// Doesn't apply to ECHO nor TALKIE_SB_ERROR
		if (message_value < MessageValue::TALKIE_MSG_ECHO) {
			_received_message = message_value;
			json_message.set_message_value(MessageValue::TALKIE_MSG_ECHO);
		}

        switch (message_value) {

			case MessageValue::TALKIE_MSG_CALL:
				{
					if (_manifesto) {
						uint8_t index_found_i = 255;
						ValueType value_type = json_message.get_action_type();
						switch (value_type) {

							case ValueType::TALKIE_VT_STRING:
								index_found_i = _manifesto->actionIndex(json_message.get_action_string());
								break;
							
							case ValueType::TALKIE_VT_INTEGER:
								index_found_i = _manifesto->actionIndex(json_message.get_action_number());
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
							if (!_manifesto->actionByIndex(index_found_i, *this, json_message)) {
								json_message.set_roger_value(RogerValue::TALKIE_RGR_NEGATIVE);
							}
						} else {
							json_message.set_roger_value(RogerValue::TALKIE_RGR_SAY_AGAIN);
						}
					} else {
						json_message.set_roger_value(RogerValue::TALKIE_RGR_NO_JOY);
					}
				}
				// In the end sends back the processed message (single message, one-to-one)
				transmitToRepeater(json_message);
				break;
			
			case MessageValue::TALKIE_MSG_TALK:
				json_message.set_nth_value_string(0, _desc);
				// In the end sends back the processed message (single message, one-to-one)
				transmitToRepeater(json_message);
				break;
			
			case MessageValue::TALKIE_MSG_CHANNEL:
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
				// Talker name already set in FROM (ready to transmit)
				transmitToRepeater(json_message);
				break;
			
			case MessageValue::TALKIE_MSG_LIST:
				{   // Because of action_index and action !!!

					#ifdef JSON_TALKER_DEBUG
					Serial.print("\t=== This object is: ");
					Serial.println(class_name());
					#endif

					uint8_t action_index = 0;
					if (_manifesto) {
						const TalkerManifesto::Action* action;
						_manifesto->iterateActionsReset();
						while ((action = _manifesto->iterateActionNext()) != nullptr) {	// No boilerplate
							json_message.set_nth_value_number(0, action_index++);
							json_message.set_nth_value_string(1, action->name);
							json_message.set_nth_value_string(2, action->desc);
							transmitToRepeater(json_message);	// Many-to-One
						}
					}
					if (!action_index) {
						json_message.set_roger_value(RogerValue::TALKIE_RGR_NIL);
						transmitToRepeater(json_message);	// One-to-One
					}
				}
				break;
			
			case MessageValue::TALKIE_MSG_INFO:
				if (json_message.has_info()) {

					InfoValue system_code = json_message.get_info_value();

					switch (system_code) {

						case InfoValue::TALKIE_INFO_BOARD:
							json_message.set_nth_value_string(0, board_description());
							break;

						case InfoValue::TALKIE_INFO_MUTE:
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

						case InfoValue::TALKIE_INFO_SOCKET:
							if (!transmitSockets(json_message)) {
								json_message.set_nth_value_string(0, "none");
							} else {
        						return talker_match;	// Avoids extra transmissions sends
							}
							break;

						case InfoValue::TALKIE_INFO_TALKER:
							json_message.set_nth_value_string(0, class_name());
							break;

						case InfoValue::TALKIE_INFO_MANIFESTO:
							if (_manifesto) {
								json_message.set_nth_value_string(0, _manifesto->class_name());
							} else {
								json_message.set_nth_value_string(0, "none");
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
					Serial.print(F("\t\ttalkerReceive1: "));
					json_message.write_to(Serial);
					Serial.print(" | ");
					Serial.print(message_id);
					Serial.print(" | ");
					Serial.println(_original_message.identity);
					#endif

					if (message_id == _original_message.identity) {
						_manifesto->echo(*this, json_message);
					}
				}
				break;
			
			case MessageValue::TALKIE_MSG_ERROR:
				if (_manifesto) {
					_manifesto->error(*this, json_message);
				}
				break;
			
			default:
				break;
        }
        return talker_match;
    }


	static const char* board_description() {
		
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


};


#endif // JSON_TALKER_H
