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


// #define JSON_TALKER_DEBUG

// Readjust if absolutely necessary
#define BROADCAST_SOCKET_BUFFER_SIZE 128


using SourceData = TalkieCodes::SourceData;
using MessageData = TalkieCodes::MessageData;
using SystemData = TalkieCodes::SystemData;
using EchoData = TalkieCodes::EchoData;
using ErrorData = TalkieCodes::ErrorData;
using JsonKey = TalkieCodes::JsonKey;
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
	Original _original_message;
    bool _muted_calls = false;

public:

    // Explicit default constructor
    JsonTalker() = delete;
        
    JsonTalker(const char* name, const char* desc, TalkerManifesto* manifesto)
        : _name(name), _desc(desc), _manifesto(manifesto) {
            // Nothing to see here
        }


	static const char* valueKey(size_t nth = 0) {
		return TalkieCodes::valueKey(nth);
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


	bool prepareMessage(JsonObject& json_message) {
		if (json_message[ JsonKey::FROM ].is<const char*>()) {
			if (strcmp(json_message[ JsonKey::FROM ].as<const char*>(), _name) != 0) {
				// FROM is different from _name, must be swapped
				json_message[ JsonKey::TO ] = json_message[ JsonKey::FROM ];
				json_message[ JsonKey::FROM ] = _name;
			}
		} else {
			// FROM doesn't even exist (must have)
			json_message[ JsonKey::FROM ] = _name;
		}

		MessageData message_code = static_cast<MessageData>(json_message[ JsonKey::MESSAGE ].as<int>());
		if (message_code < MessageData::ECHO) {

			#ifdef JSON_TALKER_DEBUG
			Serial.print(F("remoteSend1: Setting a new identifier (i) for :"));
			serializeJson(json_message, Serial);
			Serial.println();  // optional: just to add a newline after the JSON
			#endif

			// _muted_calls mutes CALL echoes only
			if (_muted_calls && _original_message.message_data == MessageData::CALL) return false;

			_original_message.identity = (uint16_t)millis();
			json_message[ JsonKey::IDENTITY ] = _original_message.identity;

		} else if (!json_message[ JsonKey::IDENTITY ].is<uint16_t>()) { // Makes sure response messages have an "i" (identifier)

			#ifdef JSON_TALKER_DEBUG
			Serial.print(F("remoteSend1: Response message with a wrong or without an identifier, now being set (i): "));
			serializeJson(json_message, Serial);
			Serial.println();  // optional: just to add a newline after the JSON
			#endif

			json_message[ JsonKey::MESSAGE ] = static_cast<int>(MessageData::ERROR);
			json_message[ JsonKey::ERROR ] = static_cast<int>(ErrorData::IDENTITY);
			_original_message.identity = (uint16_t)millis();
			json_message[ JsonKey::IDENTITY ] = _original_message.identity;

		} else {
			
			#ifdef JSON_TALKER_DEBUG
			Serial.print(F("remoteSend1: Keeping the same identifier (i): "));
			serializeJson(json_message, Serial);
			Serial.println();  // optional: just to add a newline after the JSON
			#endif

		}
		return true;
	}

	
    virtual bool remoteSend(JsonObject& json_message);


    virtual bool localSend(JsonObject& json_message) {

		#ifdef JSON_TALKER_DEBUG
		Serial.print(F("\t"));
		Serial.print(_name);
		Serial.print(F(": "));
		Serial.println(F("Sending a LOCAL message"));
		#endif

		if (prepareMessage(json_message)) {

			// Tags the message as LOCAL sourced
			json_message[ JsonKey::SOURCE ] = static_cast<int>(SourceData::LOCAL);
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
				
					pre_validated = _json_talkers[talker_i]->processMessage(json_copy);
					sent_message = true;
					if (!pre_validated) break;
				}
			}
			return sent_message;
		}
		return false;
    }


    virtual bool hereSend(JsonObject& json_message) {

		#ifdef JSON_TALKER_DEBUG
		Serial.print(F("\t"));
		Serial.print(_name);
		Serial.print(F(": "));
		Serial.println(F("Sending a HERE message"));
		#endif

        return processMessage(json_message);
    }


    virtual bool transmitMessage(JsonObject& json_message) {

		#ifdef JSON_TALKER_DEBUG
		Serial.print(F("\t"));
		Serial.print(_name);
		Serial.print(F(": "));
		#endif

		if (json_message[ JsonKey::SOURCE ].is<int>()) {
			SourceData source_data = static_cast<SourceData>( json_message[ JsonKey::SOURCE ].as<int>() );
			switch (source_data) {

				case SourceData::LOCAL:
					#ifdef JSON_TALKER_DEBUG
					Serial.println(F("\tTransmitted a LOCAL message"));
					#endif
					return localSend(json_message);
				
				case SourceData::HERE:
					#ifdef JSON_TALKER_DEBUG
					Serial.println(F("\tTransmitted an HERE message"));
					#endif
					return hereSend(json_message);

				// By default it's sent to REMOTE because it's safer ("c" = 0 auto set by socket)
				default: break;
			}
		}
		// By default it's sent to REMOTE because it's safer ("c" = 0 auto set by socket)
		#ifdef JSON_TALKER_DEBUG
		Serial.println(F("\tTransmitted a REMOTE message"));
		#endif
		return remoteSend(json_message);
    }

    
    virtual bool processMessage(JsonObject& json_message) {

        #ifdef JSON_TALKER_DEBUG
        Serial.println(F("\tProcessing JSON message..."));
        #endif
        
        bool dont_interrupt = true;   // Doesn't interrupt next talkers process

		if (!json_message[ JsonKey::MESSAGE ].is<int>()) {
			return false;
		}

		MessageData message_data = static_cast<MessageData>( json_message[ JsonKey::MESSAGE ].as<int>() );

        // Is it for me?
        if (json_message[ JsonKey::TO ].is<uint8_t>()) {
            if (json_message[ JsonKey::TO ].as<uint8_t>() != _channel)
                return true;    // It's still validated (just not for me as a target)
        } else if (json_message[ JsonKey::TO ].is<String>()) {
            if (json_message[ JsonKey::TO ] != _name) {
                #ifdef JSON_TALKER_DEBUG
                Serial.println(F("\tMessage NOT for me!"));
                #endif
                return true;    // It's still validated (just not for me as a target)
            } else {
                dont_interrupt = false; // Found by name, interrupts next Talkers process
            }
        } else if (message_data > MessageData::PING) {
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
		if (message_data < MessageData::ECHO) {
			_original_message.message_data = message_data;
			json_message[ JsonKey::MESSAGE ] = static_cast<int>(MessageData::ECHO);
		}

        switch (message_data) {

			case MessageData::CALL:
				{
					uint8_t index_found_i = 255;
					if (json_message[ JsonKey::INDEX ].is<uint8_t>()) {
						index_found_i = _manifesto->actionIndex(json_message[ JsonKey::INDEX ].as<uint8_t>());
					} else if (json_message[ JsonKey::NAME ].is<const char *>()) {
						index_found_i = _manifesto->actionIndex(json_message[ JsonKey::NAME ].as<const char *>());
					}
					if (index_found_i < 255) {

						#ifdef JSON_TALKER_DEBUG
						Serial.print(F("\tRUN found at "));
						Serial.print(index_found_i);
						Serial.println(F(", now being processed..."));
						#endif

						if (_manifesto->actionByIndex(index_found_i, json_message, this)) {
							json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::ROGER);
						} else {
							json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::NEGATIVE);
						}
					} else {
						json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::SAY_AGAIN);
					}
				}
				// In the end sends back the processed message (single message, one-to-one)
				transmitMessage(json_message);
				break;
			
			case MessageData::TALK:
				json_message[ JsonKey::DESCRIPTION ] = _desc;
				// In the end sends back the processed message (single message, one-to-one)
				transmitMessage(json_message);
				break;
			
			case MessageData::CHANNEL:
				if (json_message[ valueKey(0) ].is<uint8_t>()) {

					#ifdef JSON_TALKER_DEBUG
					Serial.print(F("\tChannel B value is an <uint8_t>: "));
					Serial.println(json_message[ valueKey(0) ].is<uint8_t>());
					#endif

					_channel = json_message[ valueKey(0) ].as<uint8_t>();
				}
				json_message[ valueKey(0) ] = _channel;
				// In the end sends back the processed message (single message, one-to-one)
				transmitMessage(json_message);
				break;
			
			case MessageData::PING:
				// Talker name already set in FROM (ready to transmit)
				transmitMessage(json_message);
				break;
			
			case MessageData::LIST:
				{   // Because of none_list !!!
					bool no_list = true;

					// In your list handler:
					
					#ifdef JSON_TALKER_DEBUG
					Serial.print("\t=== This object is: ");
					Serial.println(class_name());
					#endif

					_manifesto->iterateActionsReset();
					const TalkerManifesto::Action* run;
					uint8_t action_index = 0;
					while ((run = _manifesto->iterateActionNext()) != nullptr) {	// No boilerplate
						no_list = false;
						json_message[ JsonKey::NAME ] = run->name;      // Direct access
						json_message[ JsonKey::DESCRIPTION ] = run->desc;
						json_message[ JsonKey::INDEX ] = action_index++;
						transmitMessage(json_message);	// One-to-Many
					}

					if(no_list) {
						json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::NIL);
						transmitMessage(json_message);	// One-to-Many
					}
				}
				break;
			
			case MessageData::SYS:
				if (json_message["s"].is<int>()) {

					SystemData system_code = static_cast<SystemData>(json_message["s"].as<int>());

					switch (system_code) {

						case SystemData::BOARD:
							if (_socket) {
								json_message[ JsonKey::DESCRIPTION ] = board_description();
							} else {
								json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::NIL);
							}
							break;

						case SystemData::DROPS:
							if (_socket) {
								json_message[ valueKey(0) ] = get_drops();
							} else {
								json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::NIL);
							}
							break;

						case SystemData::DELAY:
							if (_socket) {
								json_message[ valueKey(0) ] = get_delay();
							} else {
								json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::NIL);
							}
							break;

						case SystemData::MUTE:
							if (!_muted_calls) {
								_muted_calls = true;
								json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::ROGER);
							} else {
								json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::NEGATIVE);
								json_message["r"] = "Already muted";
							}
							break;

						case SystemData::UNMUTE:
							if (_muted_calls) {
								_muted_calls = false;
								json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::ROGER);
							} else {
								json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::NEGATIVE);
								json_message["r"] = "Already NOT muted";
							}
							break;

						case SystemData::MUTED:
							if (_muted_calls) {
								json_message[ valueKey(0) ] = 1;
							} else {
								json_message[ valueKey(0) ] = 0;
							}
							break;

						case SystemData::SOCKET:
							if (_socket) {
								json_message[ valueKey(0) ] = socket_class_name();
							} else {
								json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::NIL);
							}
							break;

						case SystemData::TALKER:
							json_message[ valueKey(0) ] = class_name();
							break;

						case SystemData::MANIFESTO:
							if (_manifesto) {
								json_message[ valueKey(0) ] = _manifesto->class_name();
							} else {
								json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::NIL);
							}
							break;

						default:
							json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::SAY_AGAIN);
							break;
					}

					// In the end sends back the processed message (single message, one-to-one)
					transmitMessage(json_message);
				}
				break;
			
			case MessageData::ECHO:
				if (_manifesto) {
					// Makes sure it has the same id first (echo condition)
					uint16_t message_id = json_message[ JsonKey::IDENTITY ].as<uint16_t>();
					if (message_id == _original_message.identity) {
						_manifesto->echo(json_message, this);
					}
				}
				break;
			
			case MessageData::ERROR:
				if (_manifesto) {
					_manifesto->error(json_message, this);
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
