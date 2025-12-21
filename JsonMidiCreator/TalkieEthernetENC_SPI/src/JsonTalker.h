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
#include <ArduinoJson.h>    // Include ArduinoJson Library
#include "TalkerManifesto.hpp"
#include "TalkieCodes.hpp"
#include "JsonMessage.hpp"


#ifndef BROADCAST_SOCKET_BUFFER_SIZE
#define BROADCAST_SOCKET_BUFFER_SIZE 128
#endif
#ifndef NAME_LEN
#define NAME_LEN 16
#endif

// #define JSON_TALKER_DEBUG

using SourceValue = TalkieCodes::SourceValue;
using MessageValue = TalkieCodes::MessageValue;
using InfoValue = TalkieCodes::InfoValue;
using RogerValue = TalkieCodes::RogerValue;
using ErrorValue = TalkieCodes::ErrorValue;
using TalkieKey = TalkieCodes::TalkieKey;
using Original = TalkerManifesto::Original;


class BroadcastSocket;


class JsonTalker {
protected:
    
    // The socket can't be static becaus different talkers may use different sockets (remote)
    BroadcastSocket* _socket = nullptr;
    // Pointer PRESERVE the polymorphism while objects don't!
    static JsonTalker** _json_talkers;  // It's capable of communicate with other talkers (local)
    static uint8_t _talker_count;

    const char* _name;      // Name of the Talker
    const char* _desc;      // Description of the Device
	TalkerManifesto* _manifesto = nullptr;
    uint8_t _channel = 0;
	MessageValue _received_message = MessageValue::NOISE;
	Original _original_message = {0, MessageValue::NOISE};
    bool _muted_calls = false;

public:

    // Explicit default constructor
    JsonTalker() = delete;
        
    JsonTalker(const char* name, const char* desc, TalkerManifesto* manifesto)
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


	static const char* valueKey(size_t nth = 0) {
		return TalkieCodes::valueKey(nth);
	}

	bool inSameSocket(const BroadcastSocket* socket) const {
		if (_socket) {	// Being in nullptr is NOT in a socket
			return socket == _socket;
		}
		return false;
	}


	// Getter and setters

    void setSocket(BroadcastSocket* socket);
    BroadcastSocket& getSocket();

	const char* socket_class_name();

    const char* get_name() { return _name; }
    const char* get_desc() { return _desc; }
    void set_channel(uint8_t channel) { _channel = channel; }
    uint8_t get_channel() { return _channel; }
    Original& get_original() { return _original_message; }
    
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
	

    virtual const char* class_name() const { return "JsonTalker"; }


    virtual void loop() {
        if (_manifesto) {
            _manifesto->loop(this);
        }
    }


    static void connectTalkers(JsonTalker** json_talkers, uint8_t talker_count) {
        _json_talkers = json_talkers;
        _talker_count = talker_count;
    }


	// PARALLEL DEVELOPMENT WITH ARDUINOJSON
	
	bool prepareMessage(JsonObject& json_message, JsonMessage& new_json_message) {

		if (json_message[ TalkieKey::FROM ].is<const char*>()) {
			if (strcmp(json_message[ TalkieKey::FROM ].as<const char*>(), _name) != 0) {
				// FROM is different from _name, must be swapped
				json_message[ TalkieKey::TO ] = json_message[ TalkieKey::FROM ];
				json_message[ TalkieKey::FROM ] = _name;
			}
		} else {
			// FROM doesn't even exist (must have)
			json_message[ TalkieKey::FROM ] = _name;
		}
		// *************** PARALLEL DEVELOPMENT WITH ARDUINOJSON (IN PROGRESS) ***************
		if (new_json_message.has_key( MessageKey::FROM )) {
			char from_name[NAME_LEN] = {'\0'};
			test_json_message.get_string('f', from_name, NAME_LEN);
			if (strcmp(from_name, _name) != 0) {
				// FROM is different from _name, must be swapped (replaces "f" with "t")
				new_json_message.swap_key(MessageKey::FROM, MessageKey::TO);
				new_json_message.set_string(MessageKey::FROM, _name);
			}
		} else {
			// FROM doesn't even exist (must have)
			new_json_message.set_string(MessageKey::FROM, _name);
		}

		MessageValue message_data = static_cast<MessageValue>( json_message[ TalkieKey::MESSAGE ].as<int>() );
		if (message_data < MessageValue::ECHO) {

			#ifdef JSON_TALKER_DEBUG
			Serial.print(F("remoteSend1: Setting a new identifier (i) for :"));
			serializeJson(json_message, Serial);
			Serial.println();  // optional: just to add a newline after the JSON
			#endif

			// _muted_calls mutes CALL echoes only
			if (_muted_calls && _received_message == MessageValue::CALL) {
				_received_message = MessageValue::NOISE;	// Avoids false mutes for self generated messages (safe code)
				return false;
			} else {
				_received_message = MessageValue::NOISE;	// Avoids false mutes for self generated messages (safe code)
			}

			uint16_t message_id = (uint16_t)millis();
			json_message[ TalkieKey::IDENTITY ] = message_id;
			if (message_data < MessageValue::ECHO) {
				_original_message.identity = message_id;
				_original_message.message_data = message_data;
			}

		} else if (!json_message[ TalkieKey::IDENTITY ].is<uint16_t>()) { // Makes sure response messages have an "i" (identifier)

			#ifdef JSON_TALKER_DEBUG
			Serial.print(F("remoteSend1: Response message with a wrong or without an identifier, now being set (i): "));
			serializeJson(json_message, Serial);
			Serial.println();  // optional: just to add a newline after the JSON
			#endif

			json_message[ TalkieKey::MESSAGE ] = static_cast<int>(MessageValue::ERROR);
			json_message[ valueKey(0) ] = static_cast<int>(ErrorValue::IDENTITY);
			json_message[ TalkieKey::IDENTITY ] = (uint16_t)millis();

		} else {
			
			#ifdef JSON_TALKER_DEBUG
			Serial.print(F("remoteSend1: Keeping the same identifier (i): "));
			serializeJson(json_message, Serial);
			Serial.println();  // optional: just to add a newline after the JSON
			#endif

		}
		return true;
	}

	
    virtual bool remoteSend(JsonObject& json_message, JsonMessage& new_json_message);


    virtual bool localSend(JsonObject& json_message, JsonMessage& new_json_message) {
		(void)new_json_message;

		#ifdef JSON_TALKER_DEBUG
		Serial.print(F("\t"));
		Serial.print(_name);
		Serial.print(F(": "));
		Serial.println(F("Sending a LOCAL message"));
		#endif

		if (prepareMessage(json_message, new_json_message)) {

			// Tags the message as LOCAL sourced
			json_message[ TalkieKey::SOURCE ] = static_cast<int>(SourceValue::LOCAL);
			// Triggers all local Talkers to processes the json_message
			bool sent_message = false;
			bool pre_validated = false;
			for (uint8_t talker_i = 0; talker_i < _talker_count; ++talker_i) {	// _talker_count makes the code safe
				if (_json_talkers[talker_i] != this) {  // Can't send to myself

					// CREATE COPY for each talker
					// JsonDocument in the stack makes sure its memory is released (NOT GLOBAL)
					#if ARDUINOJSON_VERSION_MAJOR >= 7
					JsonDocument doc_copy;
					#else
					StaticJsonDocument<BROADCAST_SOCKET_BUFFER_SIZE> doc_copy;
					#endif
					JsonObject json_copy = doc_copy.to<JsonObject>();
					
					// Copy all data from original
					for (JsonPair kv : json_message) {
						json_copy[kv.key()] = kv.value();
					}
				
					pre_validated = _json_talkers[talker_i]->processMessage(json_copy, new_json_message);
					sent_message = true;
					if (!pre_validated) break;
				}
			}
			return sent_message;
		}
		return false;
    }


    virtual bool selfSend(JsonObject& json_message, JsonMessage& new_json_message) {
		(void)new_json_message;

		#ifdef JSON_TALKER_DEBUG
		Serial.print(F("\t"));
		Serial.print(_name);
		Serial.print(F(": "));
		Serial.println(F("Sending a SELF message"));
		#endif

		// Tags the message as LOCAL sourced
		json_message[ TalkieKey::SOURCE ] = static_cast<int>(SourceValue::SELF);
		// Despite being a SELF message it also needs to be prepared like any other
		if (prepareMessage(json_message, new_json_message)) {
			return processMessage(json_message, new_json_message);	// Calls my self processMessage method right away
		}
		return false;
    }


	virtual bool noneSend(JsonObject& json_message, JsonMessage& new_json_message) {
        (void)json_message; // Silence unused parameter warning
		(void)new_json_message;

		// It's absolutely neutral, does nothing, NONE
		return true;
	}


    virtual bool transmitMessage(JsonObject& json_message, JsonMessage& new_json_message) {
		(void)new_json_message;

		#ifdef JSON_TALKER_DEBUG
		Serial.print(F("\t"));
		Serial.print(_name);
		Serial.print(F(": "));
		#endif

		if (json_message[ TalkieKey::SOURCE ].is<int>()) {
			SourceValue source_data = static_cast<SourceValue>( json_message[ TalkieKey::SOURCE ].as<int>() );
			switch (source_data) {

				case SourceValue::LOCAL:
					#ifdef JSON_TALKER_DEBUG
					Serial.println(F("\tTransmitted a LOCAL message"));
					#endif
					return localSend(json_message, new_json_message);
				
				case SourceValue::SELF:
					#ifdef JSON_TALKER_DEBUG
					Serial.println(F("\tTransmitted an SELF message"));
					#endif
					return selfSend(json_message, new_json_message);

				case SourceValue::NONE:
					#ifdef JSON_TALKER_DEBUG
					Serial.println(F("\tTransmitted an SELF message"));
					#endif
					return noneSend(json_message, new_json_message);

				// By default it's sent to REMOTE because it's safer ("c" = 0 auto set by socket)
				default: break;
			}
		}
		// By default it's sent to REMOTE because it's safer ("c" = 0 auto set by socket)
		#ifdef JSON_TALKER_DEBUG
		Serial.println(F("\tTransmitted a REMOTE message"));
		#endif
		return remoteSend(json_message, new_json_message);
    }

    
    virtual bool processMessage(JsonObject& json_message, JsonMessage& new_json_message) {
		(void)new_json_message;

        #ifdef JSON_TALKER_DEBUG
        Serial.println(F("\tProcessing JSON message..."));
        #endif
        
        bool dont_interrupt = true;   // Doesn't interrupt next talkers process

		if (!json_message[ TalkieKey::MESSAGE ].is<int>()) {
			return false;
		}

		MessageValue message_data = static_cast<MessageValue>( json_message[ TalkieKey::MESSAGE ].as<int>() );

        // Is it for me?
        if (json_message[ TalkieKey::TO ].is<uint8_t>()) {
            if (json_message[ TalkieKey::TO ].as<uint8_t>() != _channel)
                return true;    // It's still validated (just not for me as a target)
        } else if (json_message[ TalkieKey::TO ].is<String>()) {
            if (json_message[ TalkieKey::TO ] != _name) {
                #ifdef JSON_TALKER_DEBUG
                Serial.println(F("\tMessage NOT for me!"));
                #endif
                return true;    // It's still validated (just not for me as a target)
            } else {
                dont_interrupt = false; // Found by name, interrupts next Talkers process
            }
        } else if (message_data > MessageValue::PING) {
			// Only TALK, CHANNEL and PING can be broadcasted
			return false;	// AVOIDS DANGEROUS ALL AT ONCE TRIGGERING (USE CHANNEL INSTEAD)
		} else if (json_message[ valueKey(0) ].is<uint8_t>()) {
			return false;	// AVOIDS DANGEROUS SETTING OF ALL CHANNELS AT ONCE
		}

        #ifdef JSON_TALKER_DEBUG
        Serial.print(F("\tProcess: "));
        serializeJson(json_message, Serial);
        Serial.println();  // optional: just to add a newline after the JSON
        #endif

		// Doesn't apply to ECHO nor ERROR
		if (message_data < MessageValue::ECHO) {
			_received_message = static_cast<MessageValue>( json_message[ TalkieKey::MESSAGE ].as<int>() );
			json_message[ TalkieKey::MESSAGE ] = static_cast<int>(MessageValue::ECHO);
		}

        switch (message_data) {

			case MessageValue::CALL:
				{
					uint8_t index_found_i = 255;
					if (json_message[ TalkieKey::ACTION ].is<uint8_t>()) {
						index_found_i = _manifesto->actionIndex(json_message[ TalkieKey::ACTION ].as<uint8_t>());
					} else if (json_message[ TalkieKey::ACTION ].is<const char *>()) {
						index_found_i = _manifesto->actionIndex(json_message[ TalkieKey::ACTION ].as<const char *>());
					}
					if (index_found_i < 255) {

						#ifdef JSON_TALKER_DEBUG
						Serial.print(F("\tRUN found at "));
						Serial.print(index_found_i);
						Serial.println(F(", now being processed..."));
						#endif

						if (_manifesto->actionByIndex(index_found_i, *this, json_message, new_json_message)) {
							// ROGER should be implicit for CALL to spare json string size for more data (for valueKey(n))
							// json_message[ TalkieKey::ROGER ] = static_cast<int>(RogerValue::ROGER);
						} else {
							json_message[ TalkieKey::ROGER ] = static_cast<int>(RogerValue::NEGATIVE);
						}
					} else {
						json_message[ TalkieKey::ROGER ] = static_cast<int>(RogerValue::SAY_AGAIN);
					}
				}
				// In the end sends back the processed message (single message, one-to-one)
				transmitMessage(json_message, new_json_message);
				break;
			
			case MessageValue::TALK:
				json_message[ valueKey(0) ] = _desc;
				// In the end sends back the processed message (single message, one-to-one)
				transmitMessage(json_message, new_json_message);
				break;
			
			case MessageValue::CHANNEL:
				if (json_message[ valueKey(0) ].is<uint8_t>()) {

					#ifdef JSON_TALKER_DEBUG
					Serial.print(F("\tChannel B value is an <uint8_t>: "));
					Serial.println(json_message[ valueKey(0) ].is<uint8_t>());
					#endif

					_channel = json_message[ valueKey(0) ].as<uint8_t>();
				}
				json_message[ valueKey(0) ] = _channel;
				// In the end sends back the processed message (single message, one-to-one)
				transmitMessage(json_message, new_json_message);
				break;
			
			case MessageValue::PING:
				// Talker name already set in FROM (ready to transmit)
				transmitMessage(json_message, new_json_message);
				break;
			
			case MessageValue::LIST:
				{   // Because of action_index and action !!!

					#ifdef JSON_TALKER_DEBUG
					Serial.print("\t=== This object is: ");
					Serial.println(class_name());
					#endif

					uint8_t action_index = 0;
					const TalkerManifesto::Action* action;
					_manifesto->iterateActionsReset();
					while ((action = _manifesto->iterateActionNext()) != nullptr) {	// No boilerplate
						json_message[ valueKey(0) ] = action_index++;
						json_message[ valueKey(1) ] = action->name;
						json_message[ valueKey(2) ] = action->desc;
						transmitMessage(json_message, new_json_message);	// One-to-Many
					}
					if (!action_index) {
						json_message[ TalkieKey::ROGER ] = static_cast<int>(RogerValue::NIL);
					}
				}
				break;
			
			case MessageValue::INFO:
				if (json_message[ TalkieKey::INFO ].is<int>()) {

					InfoValue system_code = static_cast<InfoValue>(json_message[ TalkieKey::INFO ].as<int>());

					switch (system_code) {

						case InfoValue::BOARD:
							json_message[ valueKey(0) ] = board_description();
							break;

						case InfoValue::DROPS:
							if (_socket) {
								json_message[ valueKey(0) ] = get_drops();
							}
							break;

						case InfoValue::DELAY:
							if (_socket) {
								json_message[ valueKey(0) ] = get_delay();
							}
							break;

						case InfoValue::MUTE:
							if (json_message[ valueKey(0) ].is<uint8_t>()) {
								uint8_t mute = json_message[ valueKey(0) ].as<uint8_t>();
								if (mute) {
									_muted_calls = true;
								} else {
									_muted_calls = false;
								}
							} else {
								if (_muted_calls) {
									json_message[ valueKey(0) ] = 1;
								} else {
									json_message[ valueKey(0) ] = 0;
								}
							}
							break;

						case InfoValue::SOCKET:
							json_message[ valueKey(0) ] = socket_class_name();
							break;

						case InfoValue::TALKER:
							json_message[ valueKey(0) ] = class_name();
							break;

						case InfoValue::MANIFESTO:
							if (_manifesto) {
								json_message[ valueKey(0) ] = _manifesto->class_name();
							} else {
								json_message[ valueKey(0) ] = "none";
							}
							break;

						default: break;
					}

					// In the end sends back the processed message (single message, one-to-one)
					transmitMessage(json_message, new_json_message);
				}
				break;
			
			case MessageValue::ECHO:
				if (_manifesto) {
					// Makes sure it has the same id first (echo condition)
					uint16_t message_id = json_message[ TalkieKey::IDENTITY ].as<uint16_t>();
					if (message_id == _original_message.identity) {
						_manifesto->echo(*this, json_message, new_json_message);
					}
				}
				break;
			
			case MessageValue::ERROR:
				if (_manifesto) {
					_manifesto->error(*this, json_message, new_json_message);
				}
				break;
			
			default:
				break;
        }
        return dont_interrupt;
    }

	// PARALLEL DEVELOPMENT WITH ARDUINOJSON

	
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
