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
#include "IManifesto.hpp"
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
	IManifesto* _manifesto = nullptr;
    uint8_t _channel = 0;
    bool _muted_action = false;

public:

    // Explicit default constructor
    JsonTalker() = delete;
        
    JsonTalker(const char* name, const char* desc, IManifesto* manifesto)
        : _name(name), _desc(desc), _manifesto(manifesto) {
            // Nothing to see here
        }


    void set_delay(uint8_t delay);
    uint8_t get_delay();
    uint16_t get_drops();
	

    virtual const char* class_name() const { return "JsonTalker"; }

    virtual bool remoteSend(JsonObject& json_message);

    virtual bool localSend(JsonObject& json_message) {

		#ifdef JSON_TALKER_DEBUG
		Serial.print(F("\t"));
		Serial.print(_name);
		Serial.print(F(": "));
		Serial.println(F("Sending a LOCAL message"));
		#endif

		if (_json_talkers) {	// Safe code

			json_message[ JsonKey::SOURCE ] = static_cast<int>(SourceData::LOCAL);
			// Triggers all local Talkers to processes the json_message
			bool sent_message = false;
			bool pre_validated = false;
			for (uint8_t talker_i = 0; talker_i < _talker_count; ++talker_i) {
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
		}
        return sent_message;
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


    virtual bool replyMessage(JsonObject& json_message) {

		#ifdef JSON_TALKER_DEBUG
		Serial.print(F("\t"));
		Serial.print(_name);
		Serial.print(F(": "));
		#endif

        SourceData source_data = static_cast<SourceData>( json_message[ JsonKey::SOURCE ].as<int>() );
		switch (source_data) {

			case SourceData::REMOTE:
				#ifdef JSON_TALKER_DEBUG
				Serial.println(F("\tReplied a LOCAL message"));
				#endif
				return remoteSend(json_message);
			
			case SourceData::LOCAL:
				#ifdef JSON_TALKER_DEBUG
				Serial.println(F("\tReplied a LOCAL message"));
				#endif
				return localSend(json_message);
			
			case SourceData::HERE:
				#ifdef JSON_TALKER_DEBUG
				Serial.println(F("\tReplied a LOCAL message"));
				#endif
				return hereSend(json_message);

		}
		return false;
    }


    virtual void loop() {
        if (_manifesto) {
            _manifesto->loop(this);
        }
    }


    static void connectTalkers(JsonTalker** json_talkers, uint8_t talker_count) {
        _json_talkers = json_talkers;
        _talker_count = talker_count;
    }


	// Getter and setters

    void setSocket(BroadcastSocket* socket) { _socket = socket; }
    BroadcastSocket& getSocket();

	const char* socket_class_name();

    const char* get_name() { return _name; }
    void set_channel(uint8_t channel) { _channel = channel; }
    uint8_t get_channel() { return _channel; }
    

    JsonTalker& mute() {    // It does NOT make a copy!
        _muted_action = true;
        return *this;
    }

    JsonTalker& unmute() {
        _muted_action = false;
        return *this;
    }

    bool muted() { return _muted_action; }

    
    virtual bool processMessage(JsonObject& json_message) {

        #ifdef JSON_TALKER_DEBUG
        Serial.println(F("\tProcessing JSON message..."));
        #endif
        
        
        bool dont_interrupt = true;   // Doesn't interrupt next talkers process

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
        }   // else: If it has no "t" it means for every Talkers


        #ifdef JSON_TALKER_DEBUG
        Serial.print(F("\tProcess: "));
        serializeJson(json_message, Serial);
        Serial.println();  // optional: just to add a newline after the JSON
        #endif

		MessageData message_data = static_cast<MessageData>(json_message[ JsonKey::MESSAGE ].as<int>());

		// Doesn't apply to ECHO nor ERROR
		if (message_data < MessageData::ECHO) {

			// From one to many, starts to set the returning target in this single place only
			json_message[ JsonKey::TO ] = json_message[ JsonKey::FROM ];
			json_message[ JsonKey::FROM ] = _name;

			json_message[ JsonKey::ORIGINAL ] = json_message[ JsonKey::MESSAGE ].as<int>();
			json_message[ JsonKey::MESSAGE ] = static_cast<int>(MessageData::ECHO);
		}

        switch (message_data) {

			case MessageData::RUN:
				{
					uint8_t index_found_i = 255;
					if (json_message[ JsonKey::INDEX ].is<uint8_t>()) {
						index_found_i = _manifesto->runIndex(json_message[ JsonKey::INDEX ].as<uint8_t>());
					} else if (json_message[ JsonKey::NAME ].is<const char *>()) {
						index_found_i = _manifesto->runIndex(json_message[ JsonKey::NAME ].as<const char *>());
					}
					if (index_found_i < 255) {

						#ifdef JSON_TALKER_DEBUG
						Serial.print(F("\tRUN found at "));
						Serial.print(index_found_i);
						Serial.println(F(", now being processed..."));
						#endif

						if (_manifesto->runByIndex(index_found_i, json_message, this)) {
							json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::ROGER);
						} else {
							json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::NEGATIVE);
						}
					} else {
						json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::SAY_AGAIN);
					}
				}
				// In the end sends back the processed message (single message, one-to-one)
				replyMessage(json_message);
				break;
			
			case MessageData::SET:
				if (json_message[ JsonKey::VALUE ].is<uint32_t>()) {
					uint8_t index_found_i = 255;
					if (json_message[ JsonKey::INDEX ].is<uint8_t>()) {
						index_found_i = _manifesto->setIndex(json_message[ JsonKey::INDEX ].as<uint8_t>());
					} else if (json_message[ JsonKey::NAME ].is<const char *>()) {
						index_found_i = _manifesto->setIndex(json_message[ JsonKey::NAME ].as<const char *>());
					}
					if (index_found_i < 255) {

						#ifdef JSON_TALKER_DEBUG
						Serial.print(F("\tSET found at "));
						Serial.print(index_found_i);
						Serial.println(F(", now being processed..."));
						#endif

						if (_manifesto->setByIndex(index_found_i, json_message[ JsonKey::VALUE ].as<uint32_t>(), json_message, this)) {
							json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::ROGER);
						} else {
							json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::NEGATIVE);
						}
					} else {
						json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::SAY_AGAIN);
					}
					// Remove unecessary keys to reduce overhead data
					json_message.remove( JsonKey::VALUE );
				} else {
					json_message[ JsonKey::MESSAGE ] = static_cast<int>(MessageData::ERROR);
					json_message[ JsonKey::ERROR ] = static_cast<int>(ErrorData::FIELD);
				}
				// In the end sends back the processed message (single message, one-to-one)
				replyMessage(json_message);
				break;
			
			case MessageData::GET:
				{
					uint8_t index_found_i = 255;
					if (json_message[ JsonKey::INDEX ].is<uint8_t>()) {
						index_found_i = _manifesto->getIndex(json_message[ JsonKey::INDEX ].as<uint8_t>());
					} else if (json_message[ JsonKey::NAME ].is<const char *>()) {
						index_found_i = _manifesto->getIndex(json_message[ JsonKey::NAME ].as<const char *>());
					}
					if (index_found_i < 255) {

						#ifdef JSON_TALKER_DEBUG
						Serial.print(F("\tGET found at "));
						Serial.print(index_found_i);
						Serial.println(F(", now being processed..."));
						#endif

						// No memory leaks because message_doc exists in the listen() method stack
						// The return of the value works as an implicit ROGER (avoids network flooding)
						json_message[ JsonKey::VALUE ] = _manifesto->getByIndex(index_found_i, json_message, this);
					} else {
						json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::SAY_AGAIN);
					}
				}
				// In the end sends back the processed message (single message, one-to-one)
				replyMessage(json_message);
				break;
			
			case MessageData::TALK:
				json_message[ JsonKey::DESCRIPTION ] = _desc;
				// In the end sends back the processed message (single message, one-to-one)
				replyMessage(json_message);
				break;
			
			case MessageData::LIST:
				{   // Because of none_list !!!
					bool no_list = true;

					// In your list handler:
					
					#ifdef JSON_TALKER_DEBUG
					Serial.print("\t=== This object is: ");
					Serial.println(class_name());
					#endif

					json_message[ JsonKey::ACTION ] = static_cast<int>(MessageData::RUN);
					_manifesto->iterateRunsReset();
					const IManifesto::Action* run;
					uint8_t action_index = 0;
					while ((run = _manifesto->iterateRunsNext()) != nullptr) {	// No boilerplate
						no_list = false;
						json_message[ JsonKey::NAME ] = run->name;      // Direct access
						json_message[ JsonKey::DESCRIPTION ] = run->desc;
						json_message[ JsonKey::INDEX ] = action_index++;
						replyMessage(json_message);	// One-to-Many
					}

					json_message[ JsonKey::ACTION ] = static_cast<int>(MessageData::SET);
					_manifesto->iterateSetsReset();
					const IManifesto::Action* set;
					action_index = 0;
					while ((set = _manifesto->iterateSetsNext()) != nullptr) {	// No boilerplate
						no_list = false;
						json_message[ JsonKey::NAME ] = set->name;      // Direct access
						json_message[ JsonKey::DESCRIPTION ] = set->desc;
						json_message[ JsonKey::INDEX ] = action_index++;
						replyMessage(json_message);	// One-to-Many
					}
					
					json_message[ JsonKey::ACTION ] = static_cast<int>(MessageData::GET);
					_manifesto->iterateGetsReset();
					const IManifesto::Action* get;
					action_index = 0;
					while ((get = _manifesto->iterateGetsNext()) != nullptr) {	// No boilerplate
						no_list = false;
						json_message[ JsonKey::NAME ] = get->name;      // Direct access
						json_message[ JsonKey::DESCRIPTION ] = get->desc;
						json_message[ JsonKey::INDEX ] = action_index++;
						replyMessage(json_message);	// One-to-Many
					}

					if(no_list) {
						json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::NIL);
						replyMessage(json_message);	// One-to-Many
					}
				}
				break;
			
			case MessageData::CHANNEL:
				if (json_message[ JsonKey::VALUE ].is<uint8_t>()) {

					#ifdef JSON_TALKER_DEBUG
					Serial.print(F("\tChannel B value is an <uint8_t>: "));
					Serial.println(json_message[ JsonKey::VALUE ].is<uint8_t>());
					#endif

					_channel = json_message[ JsonKey::VALUE ].as<uint8_t>();
				}
				json_message[ JsonKey::VALUE ] = _channel;
				// In the end sends back the processed message (single message, one-to-one)
				replyMessage(json_message);
				break;
			
			case MessageData::SYS:
				if (json_message["s"].is<int>()) {

					SystemData system_code = static_cast<SystemData>(json_message["s"].as<int>());

					switch (system_code) {

					case SystemData::BOARD:
						
						// AVR Boards (Uno, Nano, Mega) - Check RAM size
						#ifdef __AVR__
							#if (RAMEND - RAMSTART + 1) == 2048
								json_message[ JsonKey::DESCRIPTION ] = F("Arduino Uno/Nano (ATmega328P)");
							#elif (RAMEND - RAMSTART + 1) == 8192
								json_message[ JsonKey::DESCRIPTION ] = F("Arduino Mega (ATmega2560)");
							#else
								json_message[ JsonKey::DESCRIPTION ] = "Unknown AVR Board";
							#endif
							
						// ESP8266
						#elif defined(ESP8266)
							json_message[ JsonKey::DESCRIPTION ] = "ESP8266 (Chip ID: " + String(ESP.getChipId()) + ")";
							
						// ESP32
						#elif defined(ESP32)
							json_message[ JsonKey::DESCRIPTION ] = "ESP32 (Rev: " + String(ESP.getChipRevision()) + ")";
							
						// Teensy Boards
						#elif defined(TEENSYDUINO)
							#if defined(__IMXRT1062__)
								json_message[ JsonKey::DESCRIPTION ] = "Teensy 4.0/4.1 (i.MX RT1062)";
							#elif defined(__MK66FX1M0__)
								json_message[ JsonKey::DESCRIPTION ] = "Teensy 3.6 (MK66FX1M0)";
							#elif defined(__MK64FX512__)
								json_message[ JsonKey::DESCRIPTION ] = "Teensy 3.5 (MK64FX512)";
							#elif defined(__MK20DX256__)
								json_message[ JsonKey::DESCRIPTION ] = "Teensy 3.2/3.1 (MK20DX256)";
							#elif defined(__MKL26Z64__)
								json_message[ JsonKey::DESCRIPTION ] = "Teensy LC (MKL26Z64)";
							#else
								json_message[ JsonKey::DESCRIPTION ] = "Unknown Teensy Board";
							#endif

						// ARM (Due, Zero, etc.)
						#elif defined(__arm__)
							json_message[ JsonKey::DESCRIPTION ] = "ARM-based Board";

						// Unknown Board
						#else
							json_message[ JsonKey::DESCRIPTION ] = "Unknown Board";

						#endif

						// TO INSERT HERE EXTRA DATA !!
						break;

					case SystemData::PING:
					
						#ifdef JSON_TALKER_DEBUG
						Serial.print(F("\tPing replied as message code: "));
						Serial.println(json_message[ JsonKey::MESSAGE ].is<int>());
						#endif
						// Replies as soon as possible (best case scenario)
						break;

					case SystemData::DROPS:
						if (_socket) {
							json_message[ JsonKey::VALUE ] = get_drops();
						} else {
							json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::NIL);
						}
						break;

					case SystemData::DELAY:
						if (_socket) {
							json_message[ JsonKey::VALUE ] = get_delay();
						} else {
							json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::NIL);
						}
						break;

					case SystemData::MUTE:
						if (!_muted_action) {
							_muted_action = true;
							json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::ROGER);
						} else {
							json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::NEGATIVE);
							json_message["r"] = "Already muted";
						}
						break;
					case SystemData::UNMUTE:
						if (_muted_action) {
							_muted_action = false;
							json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::ROGER);
						} else {
							json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::NEGATIVE);
							json_message["r"] = "Already NOT muted";
						}
						break;

					case SystemData::MUTED:
						if (_muted_action) {
							json_message[ JsonKey::VALUE ] = 1;
						} else {
							json_message[ JsonKey::VALUE ] = 0;
						}
						break;

					case SystemData::SOCKET:
						if (_socket) {
							json_message[ JsonKey::VALUE ] = socket_class_name();
						} else {
							json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::NIL);
						}
						break;

					case SystemData::TALKER:
						json_message[ JsonKey::VALUE ] = class_name();
						break;

					case SystemData::MANIFESTO:
						if (_manifesto) {
							json_message[ JsonKey::VALUE ] = _manifesto->class_name();
						} else {
							json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::NIL);
						}
						break;

					default:
						json_message[ JsonKey::ROGER ] = static_cast<int>(EchoData::SAY_AGAIN);
						break;
					}

					// In the end sends back the processed message (single message, one-to-one)
					replyMessage(json_message);
				}
				break;
			
			case MessageData::ECHO:
				if (_manifesto) {
					_manifesto->echo(json_message, this);
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

};


#endif // JSON_TALKER_H
