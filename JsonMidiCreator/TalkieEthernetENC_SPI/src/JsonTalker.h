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
// #define JSON_TALKER_DEBUG_NEW

using SourceValue = TalkieCodes::SourceValue;
using MessageValue = TalkieCodes::MessageValue;
using InfoValue = TalkieCodes::InfoValue;
using RogerValue = TalkieCodes::RogerValue;
using ErrorValue = TalkieCodes::ErrorValue;
using TalkieKey = TalkieCodes::TalkieKey;
using Original = TalkerManifesto::Original;
using ValueType = JsonMessage::ValueType;


class BroadcastSocket;


class JsonTalker {
protected:
    
    // The socket can't be static becaus different talkers may use different sockets (remote)
    BroadcastSocket* _socket = nullptr;
    // Pointer PRESERVE the polymorphism while objects don't!
    static JsonTalker* const* _json_talkers;  // It's capable of communicate with other talkers (local)
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


    static void connectTalkers(JsonTalker* const* json_talkers, uint8_t talker_count) {
        _json_talkers = json_talkers;
        _talker_count = talker_count;
    }


	// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
	bool prepareMessage(JsonMessage& new_json_message) {

		// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
		if (new_json_message.has_from()) {
			if (strcmp(new_json_message.get_from(), _name) != 0) {
				// FROM is different from _name, must be swapped (replaces "f" with "t")
				new_json_message.swap_from_with_to();
				new_json_message.set_from(_name);
			}
		} else {
			// FROM doesn't even exist (must have)
			new_json_message.set_from(_name);
		}

		// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
		MessageValue message_data = new_json_message.get_message();
		if (message_data < MessageValue::ECHO) {

			#ifdef JSON_TALKER_DEBUG
			Serial.print(F("remoteSend1: Setting a new identifier (i) for :"));
			new_json_message.write_to(Serial);
			Serial.println();  // optional: just to add a newline after the JSON
			#endif

			// _muted_calls mutes CALL echoes only
			if (_muted_calls && _received_message == MessageValue::CALL) {
				_received_message = MessageValue::NOISE;	// Avoids false mutes for self generated messages (safe code)
				return false;
			} else {
				_received_message = MessageValue::NOISE;	// Avoids false mutes for self generated messages (safe code)
			}

			// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
			new_json_message.set_identity(message_id);
			new_json_message.has_identity();	// Checks if it has identity
		} else if (!new_json_message.has_identity()) { // Makes sure response messages have an "i" (identifier)

			#ifdef JSON_TALKER_DEBUG
			Serial.print(F("remoteSend1: Response message with a wrong or without an identifier, now being set (i): "));
			new_json_message.write_to(Serial);
			Serial.println();  // optional: just to add a newline after the JSON
			#endif

			// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
			new_json_message.set_message(MessageValue::ERROR);
			new_json_message.set_identity();
			new_json_message.set_nth_value_number(0, static_cast<uint32_t>(ErrorValue::IDENTITY));

		} else {
			
			#ifdef JSON_TALKER_DEBUG
			Serial.print(F("remoteSend1: Keeping the same identifier (i): "));
			new_json_message.write_to(Serial);
			Serial.println();  // optional: just to add a newline after the JSON
			#endif

		}
		return true;
	}

	
    virtual bool remoteSend(JsonMessage& new_json_message);


    virtual bool localSend(JsonMessage& new_json_message) {
		(void)new_json_message;

		#ifdef JSON_TALKER_DEBUG
		Serial.print(F("\t"));
		Serial.print(_name);
		Serial.print(F(": "));
		Serial.println(F("Sending a LOCAL message"));
		#endif

		if (prepareMessage(new_json_message)) {

			// Tags the message as LOCAL sourced
			// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
			new_json_message.set_source(SourceValue::LOCAL);
			
			#ifdef JSON_TALKER_DEBUG_NEW
			Serial.print(F("\t\t\t\tlocalSend1.1: "));
			new_json_message.write_to(Serial);
			Serial.println();
			#endif

			// Triggers all local Talkers to processes the json_message
			bool sent_message = false;
			bool pre_validated = false;
			for (uint8_t talker_i = 0; talker_i < _talker_count; ++talker_i) {	// _talker_count makes the code safe
				if (_json_talkers[talker_i] != this) {  // Can't send to myself

					// CREATE COPY for each talker
					// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
					JsonMessage new_json_message_copy(new_json_message);
					
					#ifdef JSON_TALKER_DEBUG_NEW
					Serial.print(F("\t\t\t\tlocalSend1.2: "));
					new_json_message_copy.write_to(Serial);
					Serial.print(" | ");
					Serial.println(talker_i);
					#endif

					pre_validated = _json_talkers[talker_i]->processMessage(new_json_message_copy);
					sent_message = true;
					if (!pre_validated) break;
				}
			}
			return sent_message;
		}
		return false;
    }


    virtual bool selfSend(JsonMessage& new_json_message) {
		(void)new_json_message;

		#ifdef JSON_TALKER_DEBUG
		Serial.print(F("\t"));
		Serial.print(_name);
		Serial.print(F(": "));
		Serial.println(F("Sending a SELF message"));
		#endif

		// Tags the message as LOCAL sourced
		// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
		new_json_message.set_source(SourceValue::SELF);
		// Despite being a SELF message it also needs to be prepared like any other
		if (prepareMessage(new_json_message)) {
			return processMessage(new_json_message);	// Calls my self processMessage method right away
		}
		return false;
    }


	virtual bool noneSend(JsonMessage& new_json_message) {
		(void)new_json_message;

		// It's absolutely neutral, does nothing, NONE
		return true;
	}


    virtual bool transmitMessage(JsonMessage& new_json_message) {
		(void)new_json_message;

		#ifdef JSON_TALKER_DEBUG
		Serial.print(F("\t"));
		Serial.print(_name);
		Serial.print(F(": "));
		#endif

			// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
			SourceValue source_value = new_json_message.get_source();	// Already returns NOISE
			switch (source_value) {

				case SourceValue::LOCAL:
					#ifdef JSON_TALKER_DEBUG
					Serial.println(F("\tTransmitted a LOCAL message"));
					#endif
					return localSend(new_json_message);
				
				case SourceValue::SELF:
					#ifdef JSON_TALKER_DEBUG
					Serial.println(F("\tTransmitted an SELF message"));
					#endif
					return selfSend(new_json_message);

				case SourceValue::NONE:
					#ifdef JSON_TALKER_DEBUG
					Serial.println(F("\tTransmitted an SELF message"));
					#endif
					return noneSend(new_json_message);

				// By default it's sent to REMOTE because it's safer ("c" = 0 auto set by socket)
				default:
					// By default it's sent to REMOTE because it's safer ("c" = 0 auto set by socket)
					#ifdef JSON_TALKER_DEBUG
					Serial.println(F("\tTransmitted a REMOTE message"));
					#endif
					return remoteSend(new_json_message);
				break;
			}
		}
    }

    
    virtual bool processMessage(JsonMessage& new_json_message) {
		(void)new_json_message;

        #ifdef JSON_TALKER_DEBUG
        Serial.println(F("\tProcessing JSON message..."));
        #endif
        
        bool dont_interrupt = true;   // Doesn't interrupt next talkers process

		// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
		MessageValue message_data = new_json_message.get_message();

		#ifdef JSON_TALKER_DEBUG_NEW
		Serial.print(F("\t\tprocessMessage1: "));
		new_json_message.write_to(Serial);
		Serial.print(" | ");
		Serial.println(static_cast<int>( message_data ));
		#endif

        // Is it for me?
		if (new_json_message.has_to) {
			if (!new_json_message.for_me(_name, _channel)) {
				return false;
			}
		} else {
			// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (TO DO) ***************
			if (message_data > MessageValue::PING) {
				// Only TALK, CHANNEL and PING can be broadcasted
				return false;	// AVOIDS DANGEROUS ALL AT ONCE TRIGGERING (USE CHANNEL INSTEAD)
			} else if (old_json_message[ valueKey(0) ].is<uint8_t>()) {
				return false;	// AVOIDS DANGEROUS SETTING OF ALL CHANNELS AT ONCE
			}
		}

        #ifdef JSON_TALKER_DEBUG
        Serial.print(F("\tProcess: "));
        new_json_message.write_to(Serial);
        Serial.println();  // optional: just to add a newline after the JSON
        #endif

		// Doesn't apply to ECHO nor ERROR
		if (message_data < MessageValue::ECHO) {
			_received_message = message_data;
			// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
			new_json_message.set_message(MessageValue::ECHO);
		}

        switch (message_data) {

			case MessageValue::CALL:
				{
					uint8_t index_found_i = 255;
					// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
					ValueType value_type = new_json_message.get_value_type(TalkieKey::ACTION);
					switch (value_type) {

						case ValueType::STRING:
							index_found_i = _manifesto->actionIndex(new_json_message.get_action_string());
							break;
						
						case ValueType::NUMBER:
							index_found_i = _manifesto->actionIndex(new_json_message.get_action_number());
							break;
						
						default: break;
					}
					// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
					if (index_found_i < 255) {

						#ifdef JSON_TALKER_DEBUG
						Serial.print(F("\tRUN found at "));
						Serial.print(index_found_i);
						Serial.println(F(", now being processed..."));
						#endif

						// ROGER should be implicit for CALL to spare json string size for more data (for valueKey(n))
						if (!_manifesto->actionByIndex(index_found_i, *this, new_json_message)) {
							new_json_message.set_roger(RogerValue::NEGATIVE);
						}
					} else {
						// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
						new_json_message.set_roger(RogerValue::SAY_AGAIN);
					}
				}
				// In the end sends back the processed message (single message, one-to-one)
				transmitMessage(new_json_message);
				break;
			
			case MessageValue::TALK:
				// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
				new_json_message.set_nth_value_string(0, _desc);
				// In the end sends back the processed message (single message, one-to-one)
				transmitMessage(new_json_message);
				break;
			
			case MessageValue::CHANNEL:
				// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
				if (new_json_message.has_nth_value_number(0)) {

					#ifdef JSON_TALKER_DEBUG
					Serial.print(F("\tChannel B value is an <uint8_t>: "));
					Serial.println(new_json_message.get_nth_value_number(0));
					#endif

					_channel = new_json_message.get_nth_value_number(0);
				}
				// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
				new_json_message.set_nth_value_number(0, _channel);
				// In the end sends back the processed message (single message, one-to-one)
				transmitMessage(new_json_message);
				break;
			
			case MessageValue::PING:
				// Talker name already set in FROM (ready to transmit)
				transmitMessage(new_json_message);
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
						// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
						new_json_message.set_nth_value_number(0, action_index++);
						new_json_message.set_nth_value_string(1, action->name);
						new_json_message.set_nth_value_string(2, action->desc);
						transmitMessage(new_json_message);	// One-to-Many
					}
					if (!action_index) {
						// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
						new_json_message.set_roger(RogerValue::NIL);
					}
				}
				break;
			
			case MessageValue::INFO:
				// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
				if (new_json_message.has_info()) {

					InfoValue system_code = new_json_message.get_info();

					switch (system_code) {

						case InfoValue::BOARD:
							// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
							new_json_message.set_nth_value_string(0, board_description());
							break;

						case InfoValue::DROPS:
							if (_socket) {
								// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
								new_json_message.set_nth_value_number(0, get_drops());
							} else {
								// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
								new_json_message.set_roger(RogerValue::NIL);
							}
							break;

						case InfoValue::DELAY:
							if (_socket) {
								// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
								new_json_message.set_nth_value_number(0, get_delay());
							} else {
								// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
								new_json_message.set_roger(RogerValue::NIL);
							}
							break;

						case InfoValue::MUTE:
							// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
							if (new_json_message.has_nth_value_number(0)) {
								uint8_t mute = (uint8_t)new_json_message.get_nth_value_number(0);
								if (mute) {
									_muted_calls = true;
								} else {
									_muted_calls = false;
								}
							} else {
								if (_muted_calls) {
									new_json_message.set_nth_value_number(0, 1);
								} else {
									new_json_message.set_nth_value_number(0, 0);
								}
							}
							break;

						case InfoValue::SOCKET:
							// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
							new_json_message.set_nth_value_string(0, socket_class_name());
							break;

						case InfoValue::TALKER:
							// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
							new_json_message.set_nth_value_string(0, class_name());
							break;

						case InfoValue::MANIFESTO:
							// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
							if (_manifesto) {
								new_json_message.set_nth_value_string(0, _manifesto->class_name());
							} else {
								new_json_message.set_nth_value_string(0, "none");
							}
							break;

						default: break;
					}

					// In the end sends back the processed message (single message, one-to-one)
					transmitMessage(new_json_message);
				}
				break;
			
			case MessageValue::ECHO:
				if (_manifesto) {
					// Makes sure it has the same id first (echo condition)
					// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
					uint16_t message_id = new_json_message.get_identity();
					if (message_id == _original_message.identity) {
						_manifesto->echo(*this, new_json_message);
					}
				}
				break;
			
			case MessageValue::ERROR:
				if (_manifesto) {
					_manifesto->error(*this, new_json_message);
				}
				break;
			
			default:
				break;
        }
        return dont_interrupt;
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
