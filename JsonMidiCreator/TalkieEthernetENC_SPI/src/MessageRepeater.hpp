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
#ifndef MESSAGE_REPEATER_HPP
#define MESSAGE_REPEATER_HPP

#include <Arduino.h>        // Needed for Serial given that Arduino IDE only includes Serial in .ino files!
#include "TalkieCodes.hpp"
#include "JsonMessage.h"
#include "BroadcastSocket.h"
#include "JsonTalker.h"

#define MESSAGE_REPEATER_DEBUG

using LinkType = TalkieCodes::LinkType;

class MessageRepeater {
protected:

	BroadcastSocket* const* const _uplink_sockets;
	const uint8_t _uplink_sockets_count;
	JsonTalker* const* const _downlink_talkers;
	const uint8_t _downlink_talkers_count;
	BroadcastSocket* const* const _downlink_sockets;
	const uint8_t _downlink_sockets_count;
	JsonTalker* const* const _uplink_talkers;
	const uint8_t _uplink_talkers_count;
	

public:

    virtual const char* class_name() const { return "MessageRepeater"; }

    // Constructor
    MessageRepeater(
			BroadcastSocket* const* const uplink_sockets, uint8_t uplink_sockets_count,
			JsonTalker* const* const downlink_talkers, uint8_t downlink_talkers_count,
			BroadcastSocket* const* const downlink_sockets = nullptr, uint8_t downlink_sockets_count = 0,
			JsonTalker* const* const uplink_talkers = nullptr, uint8_t uplink_talkers_count = 0
		)
        : _uplink_sockets(uplink_sockets), _uplink_sockets_count(uplink_sockets_count),
        _downlink_talkers(downlink_talkers), _downlink_talkers_count(downlink_talkers_count),
        _downlink_sockets(downlink_sockets), _downlink_sockets_count(downlink_sockets_count),
        _uplink_talkers(uplink_talkers), _uplink_talkers_count(uplink_talkers_count)
    {
		for (uint8_t uplink_socket_i = 0; uplink_socket_i < _uplink_sockets_count; ++uplink_socket_i) {
			_uplink_sockets[uplink_socket_i]->setLink(this, LinkType::UP);
		}
		for (uint8_t downlink_talker_i = 0; downlink_talker_i < _downlink_talkers_count; ++downlink_talker_i) {
			_downlink_talkers[downlink_talker_i]->setLink(this, LinkType::DOWN);
		}
		for (uint8_t downlink_socket_i = 0; downlink_socket_i < _downlink_sockets_count; ++downlink_socket_i) {
			_downlink_sockets[downlink_socket_i]->setLink(this, LinkType::DOWN);
		}
		for (uint8_t uplink_talker_i = 0; uplink_talker_i < _uplink_talkers_count; ++uplink_talker_i) {
			_uplink_talkers[uplink_talker_i]->setLink(this, LinkType::UP);
		}
	}

	~MessageRepeater() {
		// Does nothing
	}

    void loop() {
		for (uint8_t uplink_socket_i = 0; uplink_socket_i < _uplink_sockets_count; ++uplink_socket_i) {
			_uplink_sockets[uplink_socket_i]->loop();
		}
		for (uint8_t downlink_talker_i = 0; downlink_talker_i < _downlink_talkers_count; ++downlink_talker_i) {
			_downlink_talkers[downlink_talker_i]->loop();
		}
		for (uint8_t downlink_socket_i = 0; downlink_socket_i < _downlink_sockets_count; ++downlink_socket_i) {
			_downlink_sockets[downlink_socket_i]->loop();
		}
		for (uint8_t uplink_talker_i = 0; uplink_talker_i < _uplink_talkers_count; ++uplink_talker_i) {
			_uplink_talkers[uplink_talker_i]->loop();
		}
    }


	bool socketDownlink(BroadcastSocket &socket, JsonMessage &message) {
		BroadcastValue broadcast = message.get_broadcast_value();
		TalkerMatch match = TalkerMatch::NONE;

		#ifdef MESSAGE_REPEATER_DEBUG
		Serial.print(F("\t\tsocketDownlink1: "));
		message.write_to(Serial);
		Serial.print(" | ");
		Serial.println((int)broadcast);
		#endif

		switch (broadcast) {
			// Uplink sockets or talkers can only process REMOTE messages
			case BroadcastValue::REMOTE:
			{
				for (uint8_t downlink_talker_i = 0; downlink_talker_i < _downlink_talkers_count;) {

					match = _downlink_talkers[downlink_talker_i++]->talkerReceive(message);
					switch (match) {

						case TalkerMatch::BY_NAME:
							return true;
						break;
						
						case TalkerMatch::ANY:
						case TalkerMatch::BY_CHANNEL:
							if (downlink_talker_i < _downlink_talkers_count || _downlink_sockets_count) {
								socket.deserialize_buffer(message);
							}
						break;
						
						case TalkerMatch::FAIL:
							return false;
						break;
						
						default: break;
					}
				}
				for (uint8_t downlink_socket_i = 0; downlink_socket_i < _downlink_sockets_count; ++downlink_socket_i) {
					_downlink_sockets[downlink_socket_i]->socketSend(message);
				}
			}
			break;
			
			default: break;	// Does nothing, typical for BroadcastValue::NONE
		}
	}

	bool talkerUplink(JsonTalker &talker, JsonMessage &message) {
		BroadcastValue broadcast = message.get_broadcast_value();
		TalkerMatch match = TalkerMatch::NONE;

		#ifdef MESSAGE_REPEATER_DEBUG
		Serial.print(F("\t\ttalkerUplink1: "));
		message.write_to(Serial);
		Serial.print(" | ");
		Serial.println((int)broadcast);
		#endif

		switch (broadcast) {

			case BroadcastValue::REMOTE:
			{
				for (uint8_t uplink_socket_i = 0; uplink_socket_i < _uplink_sockets_count; ++uplink_socket_i) {
					_uplink_sockets[uplink_socket_i]->socketSend(message);
				}
			}
			break;
			
			case BroadcastValue::LOCAL:
			{
				// Talkers have no buffer, so a message copy will be necessary
				JsonMessage original_message(message);

				for (uint8_t downlink_talker_i = 0; downlink_talker_i < _downlink_talkers_count;) {
					if (_downlink_talkers[downlink_talker_i] != &talker) {	// Shouldn't locally Uplink to itself

						match = _downlink_talkers[downlink_talker_i++]->talkerReceive(message);
						switch (match) {

							case TalkerMatch::BY_NAME:
								return true;
							break;
							
							case TalkerMatch::ANY:
							case TalkerMatch::BY_CHANNEL:
								if (downlink_talker_i < _downlink_talkers_count) {
									message = original_message;
								}
							break;
						
							case TalkerMatch::FAIL:
								return false;
							break;
							
							default: break;
						}
					}
				}
				for (uint8_t downlink_socket_i = 0; downlink_socket_i < _downlink_sockets_count; ++downlink_socket_i) {
					_downlink_sockets[downlink_socket_i]->socketSend(original_message);
				}
			}
			break;
			
			case BroadcastValue::SELF:
			{
				talker.socketSend(message);
			}
			break;
			
			default: break;	// Does nothing, typical for BroadcastValue::NONE
		}
	}

	bool socketUplink(BroadcastSocket &socket, JsonMessage &message) {
		BroadcastValue broadcast = message.get_broadcast_value();
		TalkerMatch match = TalkerMatch::NONE;

		#ifdef MESSAGE_REPEATER_DEBUG
		Serial.print(F("\t\tsocketUplink1: "));
		message.write_to(Serial);
		Serial.print(" | ");
		Serial.println((int)broadcast);
		#endif

		switch (broadcast) {

			case BroadcastValue::REMOTE:
			{
				for (uint8_t uplink_talker_i = 0; uplink_talker_i < _uplink_talkers_count;) {

					match = _uplink_talkers[uplink_talker_i++]->talkerReceive(message);
					switch (match) {

						case TalkerMatch::BY_NAME:
							return true;
						break;
						
						case TalkerMatch::ANY:
						case TalkerMatch::BY_CHANNEL:
							if (uplink_talker_i < _uplink_talkers_count || _uplink_sockets_count) {
								socket.deserialize_buffer(message);
							}
						break;
						
						case TalkerMatch::FAIL:
							return false;
						break;
						
						default: break;
					}
				}
				for (uint8_t uplink_socket_i = 0; uplink_socket_i < _uplink_sockets_count; ++uplink_socket_i) {
					_uplink_sockets[uplink_socket_i]->socketSend(message);
				}
			}
			break;
			
			case BroadcastValue::LOCAL:
			{
				for (uint8_t downlink_talker_i = 0; downlink_talker_i < _uplink_talkers_count;) {

					match = _uplink_talkers[downlink_talker_i++]->talkerReceive(message);
					switch (match) {

						case TalkerMatch::BY_NAME:
							return true;
						break;
						
						case TalkerMatch::ANY:
						case TalkerMatch::BY_CHANNEL:
							if (downlink_talker_i < _uplink_talkers_count || _downlink_sockets_count) {
								socket.deserialize_buffer(message);
							}
						break;
						
						case TalkerMatch::FAIL:
							return false;
						break;
						
						default: break;
					}
				}
				for (uint8_t downlink_socket_i = 0; downlink_socket_i < _downlink_sockets_count; ++downlink_socket_i) {
					if (_downlink_sockets[downlink_socket_i] != &socket) {	// Shouldn't locally Uplink to itself
						_downlink_sockets[downlink_socket_i]->socketSend(message);
					}
				}
			}
			break;
			
			default: break;	// Does nothing, typical for BroadcastValue::NONE
		}
	}

	bool talkerDownlink(JsonTalker &talker, JsonMessage &message) {
		BroadcastValue broadcast = message.get_broadcast_value();
		TalkerMatch match = TalkerMatch::NONE;

		#ifdef MESSAGE_REPEATER_DEBUG
		Serial.print(F("\t\ttalkerDownlink1: "));
		message.write_to(Serial);
		Serial.print(" | ");
		Serial.println((int)broadcast);
		#endif

		switch (broadcast) {
			// Uplink sockets or talkers can only process REMOTE messages
			case BroadcastValue::REMOTE:
			{
				// Talkers have no buffer, so a message copy will be necessary
				JsonMessage original_message(message);

				for (uint8_t uplink_talker_i = 0; uplink_talker_i < _uplink_talkers_count;) {
					if (_uplink_talkers[uplink_talker_i++] != &talker) {

						match = _uplink_talkers[uplink_talker_i++]->talkerReceive(message);
						switch (match) {

							case TalkerMatch::BY_NAME:
								return true;
							break;
							
							case TalkerMatch::ANY:
							case TalkerMatch::BY_CHANNEL:
								if (uplink_talker_i < _uplink_talkers_count) {
									message = original_message;
								}
							break;
							
							case TalkerMatch::FAIL:
								return false;
							break;
							
							default: break;
						}
					}
				}
				for (uint8_t uplink_socket_i = 0; uplink_socket_i < _uplink_sockets_count; ++uplink_socket_i) {
					_uplink_sockets[uplink_socket_i]->socketSend(original_message);
				}
			}
			break;
			
			default: break;	// Does nothing, typical for BroadcastValue::NONE
		}
	}

};



#endif // MESSAGE_REPEATER_HPP
