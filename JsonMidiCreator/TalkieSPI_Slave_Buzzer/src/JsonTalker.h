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
#define REMOTE_C ((uint16_t)0)
#define LOCAL_C ((uint16_t)1)


class BroadcastSocket;


class JsonTalker {
protected:
    
    // The socket can't be static becaus different talkers may use different sockets (remote)
    BroadcastSocket* _socket = nullptr;
    // Pointer PRESERVE the polymorphism while objects don't!
    static JsonTalker** _json_talkers;  // It's capable of communicate with other talkers (local)
    static uint8_t _talker_count;

public:

    virtual const char* class_name() const { return "JsonTalker"; }


protected:

    const char* _name;      // Name of the Talker
    const char* _desc;      // Description of the Device
	IManifesto* _manifesto;
    uint8_t _channel = 0;
    bool _muted = false;


    bool remoteSend(JsonObject& json_message, bool as_reply = false, uint8_t target_index = 255);

    bool localSend(JsonObject& json_message, bool as_reply = false, uint8_t target_index = 255) {
        (void)as_reply; 	// Silence unused parameter warning
        (void)target_index; // Silence unused parameter warning

		#ifdef JSON_TALKER_DEBUG
		Serial.print(F("\t"));
		Serial.print(_name);
		Serial.print(F(": "));
		Serial.println(F("Sending a LOCAL message"));
		#endif

        json_message[ key_str(JsonKey::CHECKSUM) ] = LOCAL_C;	// 'c' = 1 means LOCAL_C communication
        // Triggers all local Talkers to processes the json_message
        bool sent_message = false;
		if (target_index < _talker_count) {
			if (_json_talkers[target_index] != this) {  // Can't send to myself

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
            
				_json_talkers[target_index]->processData(json_copy);
				sent_message = true;
			}
		} else {
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
				
					pre_validated = _json_talkers[talker_i]->processData(json_copy, pre_validated);
					sent_message = true;
					if (!pre_validated) break;
				}
			}
		}
        return sent_message;
    }


    bool replyMessage(JsonObject& json_message, bool as_reply = true) {

		#ifdef JSON_TALKER_DEBUG
		Serial.print(F("\t"));
		Serial.print(_name);
		Serial.print(F(": "));
		#endif

		uint16_t c = json_message[ key_str(JsonKey::CHECKSUM) ].as<uint16_t>();
		if (c == LOCAL_C) {	// c == 1 means a local message while 0 means a remote one
			#ifdef JSON_TALKER_DEBUG
			Serial.println(F("\tReplied a LOCAL message"));
			#endif
			return localSend(json_message, as_reply);
		}
		#ifdef JSON_TALKER_DEBUG
		Serial.println(F("Replied a REMOTE message"));
		#endif
        return remoteSend(json_message, as_reply);
    }

    void set_delay(uint8_t delay);
    uint8_t get_delay();
    uint16_t get_drops();


public:

    // Explicit default constructor
    JsonTalker() = delete;
        
    JsonTalker(const char* name, const char* desc, IManifesto* manifesto)
        : _name(name), _desc(desc), _manifesto(manifesto) {
            // Nothing to see here
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
    const char* get_name() { return _name; }
    void set_channel(uint8_t channel) { _channel = channel; }
    uint8_t get_channel() { return _channel; }
    

    JsonTalker& mute() {    // It does NOT make a copy!
        _muted = true;
        return *this;
    }

    JsonTalker& unmute() {
        _muted = false;
        return *this;
    }

    bool muted() { return _muted; }

    
    virtual bool processData(JsonObject& json_message, bool pre_validated = false) {

        #ifdef JSON_TALKER_DEBUG
        Serial.println(F("\tProcessing JSON message..."));
        #endif
        
        // Error types:
        //     0 - Unknown sender   // Removed, useless kind of error report that results in UDP flooding
        //     1 - Message missing the checksum
        //     2 - Message corrupted
        //     3 - Wrong message code
        //     4 - Message NOT identified
        //     5 - Set command arrived too late

        if (!pre_validated) {

            if (!json_message[ key_str(JsonKey::FROM) ].is<String>()) {
                #ifdef JSON_TALKER_DEBUG
                Serial.println(0);
                #endif
                return false;
            }
            if (!json_message[ key_str(JsonKey::IDENTITY) ].is<uint32_t>()) {
                #ifdef JSON_TALKER_DEBUG
                Serial.println(4);
                #endif
                json_message[ key_str(JsonKey::MESSAGE) ] = 7;   // error
                json_message[ key_str(JsonKey::ERROR) ] = 4;

                replyMessage(json_message, true);	// Includes reply swap
                return false;
            }
        }

        
        bool dont_interrupt = true;   // Doesn't interrupt next talkers process

        // Is it for me?
        if (json_message[ key_str(JsonKey::TO) ].is<uint8_t>()) {
            if (json_message[ key_str(JsonKey::TO) ].as<uint8_t>() != _channel)
                return true;    // It's still validated (just not for me as a target)
        } else if (json_message[ key_str(JsonKey::TO) ].is<String>()) {
            if (json_message[ key_str(JsonKey::TO) ] != _name) {
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

        // From one to many, starts to set the returning target in this single place only
        json_message[ key_str(JsonKey::TO) ] = json_message[ key_str(JsonKey::FROM) ];
        json_message[ key_str(JsonKey::FROM) ] = _name;

        MessageCode message_code = static_cast<MessageCode>(json_message[ key_str(JsonKey::MESSAGE) ].as<int>());
        json_message[ key_str(JsonKey::ORIGINAL) ] = json_message[ key_str(JsonKey::MESSAGE) ].as<int>();
        json_message[ key_str(JsonKey::MESSAGE) ] = static_cast<int>(MessageCode::ECHO);

        switch (message_code)
        {
        case MessageCode::TALK:
            json_message[ key_str(JsonKey::DESCRIPTION) ] = _desc;
            replyMessage(json_message, true);	// Includes reply swap
            break;
        
        case MessageCode::LIST:
            {   // Because of none_list !!!
                bool none_list = true;

                // In your list handler:
                
                #ifdef JSON_TALKER_DEBUG
                Serial.print("\t=== This object is: ");
                Serial.println(class_name());
                #endif

				json_message[ key_str(JsonKey::ACTION) ] = static_cast<int>(MessageCode::RUN);
				_manifesto->iterateRunsReset();
				const IManifesto::Action* run;
				uint8_t action_index = 0;
				while ((run = _manifesto->iterateRunsNext()) != nullptr) {	// No boilerplate
					none_list = false;
					json_message[ key_str(JsonKey::NAME) ] = run->name;      // Direct access
					json_message[ key_str(JsonKey::DESCRIPTION) ] = run->desc;
					json_message[ key_str(JsonKey::INDEX) ] = action_index++;
					replyMessage(json_message, true);
				}

                json_message[ key_str(JsonKey::ACTION) ] = static_cast<int>(MessageCode::SET);
				_manifesto->iterateSetsReset();
				const IManifesto::Action* set;
				action_index = 0;
				while ((set = _manifesto->iterateSetsNext()) != nullptr) {	// No boilerplate
					none_list = false;
					json_message[ key_str(JsonKey::NAME) ] = set->name;      // Direct access
					json_message[ key_str(JsonKey::DESCRIPTION) ] = set->desc;
					json_message[ key_str(JsonKey::INDEX) ] = action_index++;
					replyMessage(json_message, true);
				}
				
                json_message[ key_str(JsonKey::ACTION) ] = static_cast<int>(MessageCode::GET);
				_manifesto->iterateGetsReset();
				const IManifesto::Action* get;
				action_index = 0;
				while ((get = _manifesto->iterateGetsNext()) != nullptr) {	// No boilerplate
					none_list = false;
					json_message[ key_str(JsonKey::NAME) ] = get->name;      // Direct access
					json_message[ key_str(JsonKey::DESCRIPTION) ] = get->desc;
					json_message[ key_str(JsonKey::INDEX) ] = action_index++;
					replyMessage(json_message, true);
				}

                if(none_list) {
                    json_message[ key_str(JsonKey::ROGER) ] = static_cast<int>(EchoCode::SAY_AGAIN);
                }
            }
            break;
        
        case MessageCode::RUN:
			{
				uint8_t index_found_i = 255;
				if (json_message[ key_str(JsonKey::INDEX) ].is<uint8_t>()) {
					index_found_i = _manifesto->runIndex(json_message[ key_str(JsonKey::INDEX) ].as<uint8_t>());
				} else if (json_message[ key_str(JsonKey::NAME) ].is<const char *>()) {
					index_found_i = _manifesto->runIndex(json_message[ key_str(JsonKey::NAME) ].as<const char *>());
				}
				if (index_found_i < 255) {

					#ifdef JSON_TALKER_DEBUG
					Serial.print(F("\tRUN found at "));
					Serial.print(index_found_i);
					Serial.println(F(", now being processed..."));
					#endif

					if (_manifesto->runByIndex(index_found_i, json_message, this)) {
						json_message[ key_str(JsonKey::ROGER) ] = static_cast<int>(EchoCode::ROGER);
					} else {
						json_message[ key_str(JsonKey::ROGER) ] = static_cast<int>(EchoCode::NEGATIVE);
					}
					replyMessage(json_message, true);
				} else {
					json_message[ key_str(JsonKey::ROGER) ] = static_cast<int>(EchoCode::SAY_AGAIN);
					replyMessage(json_message, true);
				}
			}
            break;
        
        case MessageCode::SET:
			{
				uint8_t index_found_i = 255;
				if (json_message[ key_str(JsonKey::INDEX) ].is<uint8_t>()) {
					index_found_i = _manifesto->setIndex(json_message[ key_str(JsonKey::INDEX) ].as<uint8_t>());
				} else if (json_message[ key_str(JsonKey::NAME) ].is<const char *>()) {
					index_found_i = _manifesto->setIndex(json_message[ key_str(JsonKey::NAME) ].as<const char *>());
				}
				if (index_found_i < 255) {

					#ifdef JSON_TALKER_DEBUG
					Serial.print(F("\tSET found at "));
					Serial.print(index_found_i);
					Serial.println(F(", now being processed..."));
					#endif

					if (_manifesto->setByIndex(index_found_i, json_message[ key_str(JsonKey::VALUE) ].as<uint32_t>(), json_message, this)) {
						json_message[ key_str(JsonKey::ROGER) ] = static_cast<int>(EchoCode::ROGER);
					} else {
						json_message[ key_str(JsonKey::ROGER) ] = static_cast<int>(EchoCode::NEGATIVE);
					}
					replyMessage(json_message, true);
				} else {
					json_message[ key_str(JsonKey::ROGER) ] = static_cast<int>(EchoCode::SAY_AGAIN);
					replyMessage(json_message, true);
				}
			}
            break;
        
        case MessageCode::GET:
			{
				uint8_t index_found_i = 255;
				if (json_message[ key_str(JsonKey::INDEX) ].is<uint8_t>()) {
					index_found_i = _manifesto->getIndex(json_message[ key_str(JsonKey::INDEX) ].as<uint8_t>());
				} else if (json_message[ key_str(JsonKey::NAME) ].is<const char *>()) {
					index_found_i = _manifesto->getIndex(json_message[ key_str(JsonKey::NAME) ].as<const char *>());
				}
				if (index_found_i < 255) {

					#ifdef JSON_TALKER_DEBUG
					Serial.print(F("\tGET found at "));
					Serial.print(index_found_i);
					Serial.println(F(", now being processed..."));
					#endif

					// No memory leaks because message_doc exists in the listen() method stack
					// The return of the value works as an implicit ROGER (avoids network flooding)
					json_message[ key_str(JsonKey::VALUE) ] = _manifesto->getByIndex(index_found_i, json_message, this);
					replyMessage(json_message, true);
				} else {
					json_message[ key_str(JsonKey::ROGER) ] = static_cast<int>(EchoCode::SAY_AGAIN);
					replyMessage(json_message, true);
				}
			}
            break;
        
        case MessageCode::SYS:
			if (json_message["s"].is<int>()) {

        		SystemCode system_code = static_cast<SystemCode>(json_message["s"].as<int>());

				switch (system_code)
				{
				case SystemCode::MUTE:
					if (!_muted) {
						json_message[ key_str(JsonKey::ROGER) ] = static_cast<int>(EchoCode::ROGER);
						_muted = true;
					} else {
						json_message[ key_str(JsonKey::ROGER) ] = static_cast<int>(EchoCode::NEGATIVE);
						json_message["r"] = "Already muted";
					}
					{
						bool muted = _muted;
						_muted = false;	// temporaily unmute device to send state
						replyMessage(json_message, true);
						_muted = muted;
					}
					break;
				case SystemCode::UNMUTE:
					if (_muted) {
						json_message[ key_str(JsonKey::ROGER) ] = static_cast<int>(EchoCode::ROGER);
						_muted = false;
					} else {
						json_message[ key_str(JsonKey::ROGER) ] = static_cast<int>(EchoCode::NEGATIVE);
						json_message["r"] = "Already NOT muted";
					}
					replyMessage(json_message, true);
					break;
				case SystemCode::MUTED:
					json_message[ key_str(JsonKey::ROGER) ] = static_cast<int>(EchoCode::ROGER);
					json_message[ key_str(JsonKey::VALUE) ] = _muted;
					{
						bool muted = _muted;
						_muted = false;	// temporaily unmute device to send state
						replyMessage(json_message, true);
						_muted = muted;
					}
					break;
				case SystemCode::BOARD:
					json_message[ key_str(JsonKey::ROGER) ] = static_cast<int>(EchoCode::ROGER);
					
				// AVR Boards (Uno, Nano, Mega) - Check RAM size
				#ifdef __AVR__
					#if (RAMEND - RAMSTART + 1) == 2048
						json_message[ key_str(JsonKey::DESCRIPTION) ] = "Arduino Uno/Nano (ATmega328P)";
					#elif (RAMEND - RAMSTART + 1) == 8192
						json_message[ key_str(JsonKey::DESCRIPTION) ] = "Arduino Mega (ATmega2560)";
					#else
						json_message[ key_str(JsonKey::DESCRIPTION) ] = "Unknown AVR Board";
					#endif
					
				// ESP8266
				#elif defined(ESP8266)
					json_message[ key_str(JsonKey::DESCRIPTION) ] = "ESP8266 (Chip ID: " + String(ESP.getChipId()) + ")";
					
				// ESP32
				#elif defined(ESP32)
					json_message[ key_str(JsonKey::DESCRIPTION) ] = "ESP32 (Rev: " + String(ESP.getChipRevision()) + ")";
					
				// Teensy Boards
				#elif defined(TEENSYDUINO)
					#if defined(__IMXRT1062__)
						json_message[ key_str(JsonKey::DESCRIPTION) ] = "Teensy 4.0/4.1 (i.MX RT1062)";
					#elif defined(__MK66FX1M0__)
						json_message[ key_str(JsonKey::DESCRIPTION) ] = "Teensy 3.6 (MK66FX1M0)";
					#elif defined(__MK64FX512__)
						json_message[ key_str(JsonKey::DESCRIPTION) ] = "Teensy 3.5 (MK64FX512)";
					#elif defined(__MK20DX256__)
						json_message[ key_str(JsonKey::DESCRIPTION) ] = "Teensy 3.2/3.1 (MK20DX256)";
					#elif defined(__MKL26Z64__)
						json_message[ key_str(JsonKey::DESCRIPTION) ] = "Teensy LC (MKL26Z64)";
					#else
						json_message[ key_str(JsonKey::DESCRIPTION) ] = "Unknown Teensy Board";
					#endif

				// ARM (Due, Zero, etc.)
				#elif defined(__arm__)
					json_message[ key_str(JsonKey::DESCRIPTION) ] = "ARM-based Board";

				// Unknown Board
				#else
					json_message[ key_str(JsonKey::DESCRIPTION) ] = "Unknown Board";

				#endif

					replyMessage(json_message, true);

					// TO INSERT HERE EXTRA DATA !!
					
					break;

				case SystemCode::PING:
					json_message[ key_str(JsonKey::ROGER) ] = static_cast<int>(EchoCode::ROGER);
					replyMessage(json_message, true);
					break;

				case SystemCode::DROPS:
					json_message[ key_str(JsonKey::ROGER) ] = static_cast<int>(EchoCode::ROGER);
					json_message[ key_str(JsonKey::VALUE) ] = get_drops();
					replyMessage(json_message, true);
					break;

				case SystemCode::DELAY:
					json_message[ key_str(JsonKey::ROGER) ] = static_cast<int>(EchoCode::ROGER);
					json_message[ key_str(JsonKey::VALUE) ] = get_delay();
					replyMessage(json_message, true);
					break;

				default:
					json_message[ key_str(JsonKey::ROGER) ] = static_cast<int>(EchoCode::SAY_AGAIN);
					replyMessage(json_message, true);
					break;
				}
            }
            break;
        
        case MessageCode::ECHO:
			if (_manifesto) {
				_manifesto->echo(json_message, this);
			}
            break;
        
        case MessageCode::ERROR:
			if (_manifesto) {
				_manifesto->error(json_message, this);
			}
            break;
        
        case MessageCode::CHANNEL:
            if (json_message[ key_str(JsonKey::VALUE) ].is<uint8_t>()) {

                #ifdef JSON_TALKER_DEBUG
                Serial.print(F("\tChannel B value is an <uint8_t>: "));
                Serial.println(json_message[ key_str(JsonKey::VALUE) ].is<uint8_t>());
                #endif

                _channel = json_message[ key_str(JsonKey::VALUE) ].as<uint8_t>();
            }
            json_message[ key_str(JsonKey::VALUE) ] = _channel;
            replyMessage(json_message, true);
            break;
        
        default:
            break;
        }

        return dont_interrupt;
    }

};


#endif // JSON_TALKER_H
