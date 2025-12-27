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
#include "JsonMessage.hpp"
#include "BroadcastSocket.h"
#include "JsonTalker.h"

// #define MESSAGE_REPEATER_DEBUG

using LinkType = TalkieCodes::LinkType;

class MessageRepeater {
protected:

	BroadcastSocket* const* const _uplinked_sockets;
	const uint8_t _uplinked_sockets_count;
	JsonTalker* const* const _downlinked_talkers;
	const uint8_t _downlinked_talkers_count;
	BroadcastSocket* const* const _downlinked_sockets;
	const uint8_t _downlinked_sockets_count;
	JsonTalker* const* const _uplinked_talkers;
	const uint8_t _uplinked_talkers_count;
	

public:

    virtual const char* class_name() const { return "MessageRepeater"; }

    // Constructor
    MessageRepeater(
			BroadcastSocket* const* const uplinked_sockets, uint8_t uplinked_sockets_count,
			JsonTalker* const* const downlinked_talkers, uint8_t downlinked_talkers_count,
			BroadcastSocket* const* const downlinked_sockets = nullptr, uint8_t downlinked_sockets_count = 0,
			JsonTalker* const* const uplinked_talkers = nullptr, uint8_t uplinked_talkers_count = 0
		)
        : _uplinked_sockets(uplinked_sockets), _uplinked_sockets_count(uplinked_sockets_count),
        _downlinked_talkers(downlinked_talkers), _downlinked_talkers_count(downlinked_talkers_count),
        _downlinked_sockets(downlinked_sockets), _downlinked_sockets_count(downlinked_sockets_count),
        _uplinked_talkers(uplinked_talkers), _uplinked_talkers_count(uplinked_talkers_count)
    {
		for (uint8_t socket_j = 0; socket_j < _uplinked_sockets_count; ++socket_j) {
			_uplinked_sockets[socket_j]->setLink(this, LinkType::UP_LINKED);
		}
		for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
			_downlinked_talkers[talker_i]->setLink(this, LinkType::DOWN_LINKED);
		}
		for (uint8_t socket_j = 0; socket_j < _downlinked_sockets_count; ++socket_j) {
			_downlinked_sockets[socket_j]->setLink(this, LinkType::DOWN_LINKED);
		}
		for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count; ++talker_i) {
			_uplinked_talkers[talker_i]->setLink(this, LinkType::UP_LINKED);
		}
	}

	~MessageRepeater() {
		// Does nothing
	}

    void loop() {
		for (uint8_t socket_j = 0; socket_j < _uplinked_sockets_count; ++socket_j) {
			_uplinked_sockets[socket_j]->loop();
		}
		for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
			_downlinked_talkers[talker_i]->loop();
		}
		for (uint8_t socket_j = 0; socket_j < _downlinked_sockets_count; ++socket_j) {
			_downlinked_sockets[socket_j]->loop();
		}
		for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count; ++talker_i) {
			_uplinked_talkers[talker_i]->loop();
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
				for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count;) {

					match = _downlinked_talkers[talker_i++]->talkerReceive(message);
					switch (match) {

						case TalkerMatch::BY_NAME:
							return true;
						break;
						
						case TalkerMatch::ANY:
						case TalkerMatch::BY_CHANNEL:
							if (talker_i < _downlinked_talkers_count || _downlinked_sockets_count) {
								socket.deserialize_buffer(message);
							}
						break;
						
						case TalkerMatch::FAIL:
							return false;
						break;
						
						default: break;
					}
				}
				for (uint8_t socket_j = 0; socket_j < _downlinked_sockets_count; ++socket_j) {
					_downlinked_sockets[socket_j]->socketSend(message);
				}
				return true;
			}
			break;
			
			default: break;	// Does nothing, typical for BroadcastValue::NONE
		}
		return false;
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
				if (_uplinked_talkers_count) {
					// Talkers have no buffer, so a message copy will be necessary
					JsonMessage original_message(message);

					for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count;) {

						match = _uplinked_talkers[talker_i++]->talkerReceive(message);
						switch (match) {

							case TalkerMatch::BY_NAME:
								return true;
							break;
							
							case TalkerMatch::ANY:
							case TalkerMatch::BY_CHANNEL:
								if (talker_i < _uplinked_talkers_count) {
									message = original_message;
								}
							break;
							
							case TalkerMatch::FAIL:
								return false;
							break;
							
							default: break;
						}
					}
					for (uint8_t socket_j = 0; socket_j < _uplinked_sockets_count; ++socket_j) {
						_uplinked_sockets[socket_j]->socketSend(original_message);
					}
				} else {
					for (uint8_t socket_j = 0; socket_j < _uplinked_sockets_count; ++socket_j) {
						_uplinked_sockets[socket_j]->socketSend(message);
					}
				}
				return true;
			}
			break;
			
			case BroadcastValue::LOCAL:
			{
				if (_downlinked_talkers_count) {
					// Talkers have no buffer, so a message copy will be necessary
					JsonMessage original_message(message);

					for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count;) {
						if (_downlinked_talkers[talker_i] != &talker) {	// Shouldn't locally Uplink to itself

							match = _downlinked_talkers[talker_i++]->talkerReceive(message);
							switch (match) {

								case TalkerMatch::BY_NAME:
									return true;
								break;
								
								case TalkerMatch::ANY:
								case TalkerMatch::BY_CHANNEL:
									if (talker_i < _downlinked_talkers_count) {
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
					for (uint8_t socket_j = 0; socket_j < _downlinked_sockets_count; ++socket_j) {
						_downlinked_sockets[socket_j]->socketSend(original_message);
					}
				} else {
					for (uint8_t socket_j = 0; socket_j < _downlinked_sockets_count; ++socket_j) {
						_downlinked_sockets[socket_j]->socketSend(message);
					}
				}
				return true;
			}
			break;
			
			case BroadcastValue::SELF:
			{
				talker.talkerReceive(message);
			}
			break;
			
			default: break;	// Does nothing, typical for BroadcastValue::NONE
		}
		return false;
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
				for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count;) {

					match = _uplinked_talkers[talker_i++]->talkerReceive(message);
					switch (match) {

						case TalkerMatch::BY_NAME:
							return true;
						break;
						
						case TalkerMatch::ANY:
						case TalkerMatch::BY_CHANNEL:
							if (talker_i < _uplinked_talkers_count || _uplinked_sockets_count) {
								socket.deserialize_buffer(message);
							}
						break;
						
						case TalkerMatch::FAIL:
							return false;
						break;
						
						default: break;
					}
				}
				for (uint8_t socket_j = 0; socket_j < _uplinked_sockets_count; ++socket_j) {
					_uplinked_sockets[socket_j]->socketSend(message);
				}
				return true;
			}
			break;
			
			case BroadcastValue::LOCAL:
			{
				for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count;) {

					match = _uplinked_talkers[talker_i++]->talkerReceive(message);
					switch (match) {

						case TalkerMatch::BY_NAME:
							return true;
						break;
						
						case TalkerMatch::ANY:
						case TalkerMatch::BY_CHANNEL:
							if (talker_i < _uplinked_talkers_count || _downlinked_sockets_count) {
								socket.deserialize_buffer(message);
							}
						break;
						
						case TalkerMatch::FAIL:
							return false;
						break;
						
						default: break;
					}
				}
				for (uint8_t socket_j = 0; socket_j < _downlinked_sockets_count; ++socket_j) {
					if (_downlinked_sockets[socket_j] != &socket) {	// Shouldn't locally Uplink to itself
						_downlinked_sockets[socket_j]->socketSend(message);
					}
				}
				return true;
			}
			break;
			
			default: break;	// Does nothing, typical for BroadcastValue::NONE
		}
		return false;
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
				if (_uplinked_talkers_count) {
					// Talkers have no buffer, so a message copy will be necessary
					JsonMessage original_message(message);

					for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count;) {
						if (_uplinked_talkers[talker_i++] != &talker) {

							match = _uplinked_talkers[talker_i++]->talkerReceive(message);
							switch (match) {

								case TalkerMatch::BY_NAME:
									return true;
								break;
								
								case TalkerMatch::ANY:
								case TalkerMatch::BY_CHANNEL:
									if (talker_i < _uplinked_talkers_count) {
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
					for (uint8_t socket_j = 0; socket_j < _uplinked_sockets_count; ++socket_j) {
						_uplinked_sockets[socket_j]->socketSend(original_message);
					}
				} else {
					for (uint8_t socket_j = 0; socket_j < _uplinked_sockets_count; ++socket_j) {
						_uplinked_sockets[socket_j]->socketSend(message);
					}
				}
				return true;
			}
			break;
			
			default: break;	// Does nothing, typical for BroadcastValue::NONE
		}
		return false;
	}

};



#endif // MESSAGE_REPEATER_HPP
