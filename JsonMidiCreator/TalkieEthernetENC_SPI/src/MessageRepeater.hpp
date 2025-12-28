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
using TalkerMatch = TalkieCodes::TalkerMatch;

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
		TalkerMatch talker_match = message.get_talker_match();

		#ifdef MESSAGE_REPEATER_DEBUG
		Serial.print(F("\t\tsocketDownlink1: "));
		message.write_to(Serial);
		Serial.print(" | ");
		Serial.print((int)broadcast);
		Serial.print(" | ");
		Serial.println((int)talker_match);
		#endif

		switch (broadcast) {
			// Uplink sockets or talkers can only process REMOTE messages
			case BroadcastValue::REMOTE:		// To downlinked nodes
			{
				switch (talker_match) {

					case TalkerMatch::ANY:
					{
						for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count;) {
							_downlinked_talkers[talker_i++]->talkerReceive(message);
							if (talker_i < _downlinked_talkers_count || _downlinked_sockets_count) {
								socket.deserialize_buffer(message);
							}
						}
					}
					break;
					
					case TalkerMatch::BY_CHANNEL:
					{
						uint8_t message_channel = message.get_to_channel();
						for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
							uint8_t talker_channel = _downlinked_talkers[talker_i]->get_channel();
							if (talker_channel == message_channel) {
								_downlinked_talkers[talker_i]->talkerReceive(message);
								if (talker_i + 1 < _downlinked_talkers_count || _downlinked_sockets_count) {
									socket.deserialize_buffer(message);
								}
							}
						}
					}
					break;
					
					case TalkerMatch::BY_NAME:
					{
						char message_to_name[NAME_LEN];
						strcpy(message_to_name, message.get_to_name());
						for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
							const char* talker_name = _downlinked_talkers[talker_i]->get_name();
							if (strcmp(talker_name, message_to_name) == 0) {
								_downlinked_talkers[talker_i]->talkerReceive(message);
								return true;
							}
						}
					}
					break;
					
					default: return false;
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

			case BroadcastValue::REMOTE:		// To uplinked nodes
			{
				if (_uplinked_talkers_count) {
					TalkerMatch talker_match = message.get_talker_match();
					// Talkers have no buffer, so a message copy will be necessary
					JsonMessage original_message(message);

					switch (talker_match) {

						case TalkerMatch::ANY:
						{
							for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count; ++talker_i) {
								_uplinked_talkers[talker_i]->talkerReceive(message);
								if (talker_i + 1 < _uplinked_talkers_count) {
									message = original_message;
								}
							}
						}
						break;
						
						case TalkerMatch::BY_CHANNEL:
						{
							uint8_t message_channel = message.get_to_channel();
							for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count; ++talker_i) {
								uint8_t talker_channel = _uplinked_talkers[talker_i]->get_channel();
								if (talker_channel == message_channel) {
									_uplinked_talkers[talker_i]->talkerReceive(message);
									if (talker_i < _uplinked_talkers_count) {
										message = original_message;
									}
								}
							}
						}
						break;
						
						case TalkerMatch::BY_NAME:
						{
							char message_to_name[NAME_LEN];
							strcpy(message_to_name, message.get_to_name());
							for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count; ++talker_i) {
								const char* talker_name = _uplinked_talkers[talker_i]->get_name();
								if (strcmp(talker_name, message_to_name) == 0) {
									_uplinked_talkers[talker_i]->talkerReceive(message);
									return true;
								}
							}
						}
						break;
						
						default: return false;
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
			
			case BroadcastValue::LOCAL:		// To downlinked nodes
			{
				if (_downlinked_talkers_count) {
					TalkerMatch talker_match = message.get_talker_match();
					// Talkers have no buffer, so a message copy will be necessary
					JsonMessage original_message(message);

					switch (talker_match) {

						case TalkerMatch::ANY:
						{
							for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
								if (_downlinked_talkers[talker_i] != &talker) {
									_downlinked_talkers[talker_i]->talkerReceive(message);
									if (talker_i + 1 < _downlinked_talkers_count) {
										message = original_message;
									}
								}
							}
						}
						break;
						
						case TalkerMatch::BY_CHANNEL:
						{
							uint8_t message_channel = message.get_to_channel();
							for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
								if (_downlinked_talkers[talker_i] != &talker) {
									uint8_t talker_channel = _downlinked_talkers[talker_i]->get_channel();
									if (talker_channel == message_channel) {
										_downlinked_talkers[talker_i]->talkerReceive(message);
										if (talker_i + 1 < _downlinked_talkers_count) {
											message = original_message;
										}
									}
								}
							}
						}
						break;
						
						case TalkerMatch::BY_NAME:
						{
							char message_to_name[NAME_LEN];
							strcpy(message_to_name, message.get_to_name());
							for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
								if (_downlinked_talkers[talker_i] != &talker) {
									const char* talker_name = _downlinked_talkers[talker_i]->get_name();
									if (strcmp(talker_name, message_to_name) == 0) {
										_downlinked_talkers[talker_i]->talkerReceive(message);
										return true;
									}
								}
							}
						}
						break;
						
						default: return false;
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
				TalkerMatch talker_match = message.get_talker_match();

				switch (talker_match) {

					case TalkerMatch::ANY:
					{
						talker.talkerReceive(message);
						return true;
					}
					break;
					
					case TalkerMatch::BY_CHANNEL:
					{
						uint8_t message_channel = message.get_to_channel();
						uint8_t talker_channel = talker.get_channel();
						if (talker_channel == message_channel) {
							talker.talkerReceive(message);
							return true;
						}
					}
					break;
					
					case TalkerMatch::BY_NAME:
					{
						char message_to_name[NAME_LEN];
						strcpy(message_to_name, message.get_to_name());
						
						const char* talker_name = talker.get_name();
						if (strcmp(talker_name, message_to_name) == 0) {
							talker.talkerReceive(message);
							return true;
						}
					}
					break;
					
					default: return false;
				}
			}
			break;
			
			default: break;	// Does nothing, typical for BroadcastValue::NONE
		}
		return false;
	}

	bool socketUplink(BroadcastSocket &socket, JsonMessage &message) {
		BroadcastValue broadcast = message.get_broadcast_value();
		TalkerMatch talker_match = message.get_talker_match();

		#ifdef MESSAGE_REPEATER_DEBUG
		Serial.print(F("\t\tsocketUplink1: "));
		message.write_to(Serial);
		Serial.print(" | ");
		Serial.println((int)broadcast);
		#endif

		switch (broadcast) {

			case BroadcastValue::REMOTE:		// To uplinked nodes
			{
				switch (talker_match) {

					case TalkerMatch::ANY:
					{
						for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count;) {
							_uplinked_talkers[talker_i++]->talkerReceive(message);
							if (talker_i < _uplinked_talkers_count || _uplinked_sockets_count) {
								socket.deserialize_buffer(message);
							}
						}
					}
					break;
					
					case TalkerMatch::BY_CHANNEL:
					{
						uint8_t message_channel = message.get_to_channel();
						for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count;) {
							uint8_t talker_channel = _uplinked_talkers[talker_i]->get_channel();
							if (talker_channel == message_channel) {
								_uplinked_talkers[talker_i++]->talkerReceive(message);
								if (talker_i < _uplinked_talkers_count || _uplinked_sockets_count) {
									socket.deserialize_buffer(message);
								}
							}
						}
					}
					break;
					
					case TalkerMatch::BY_NAME:
					{
						char message_to_name[NAME_LEN];
						strcpy(message_to_name, message.get_to_name());
						for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count; ++talker_i) {
							const char* talker_name = _uplinked_talkers[talker_i]->get_name();
							if (strcmp(talker_name, message_to_name) == 0) {
								_uplinked_talkers[talker_i]->talkerReceive(message);
								return true;
							}
						}
					}
					break;
					
					default: return false;
				}
				for (uint8_t socket_j = 0; socket_j < _uplinked_sockets_count; ++socket_j) {
					_uplinked_sockets[socket_j]->socketSend(message);
				}
				return true;
			}
			break;
			
			case BroadcastValue::LOCAL:		// To downlinked nodes
			{

				switch (talker_match) {

					case TalkerMatch::ANY:
					{
						for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count;) {
							_downlinked_talkers[talker_i++]->talkerReceive(message);
							if (talker_i < _downlinked_talkers_count || _downlinked_sockets_count) {
								socket.deserialize_buffer(message);
							}
						}
					}
					break;
					
					case TalkerMatch::BY_CHANNEL:
					{
						uint8_t message_channel = message.get_to_channel();
						for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
							uint8_t talker_channel = _downlinked_talkers[talker_i]->get_channel();
							if (talker_channel == message_channel) {
								_downlinked_talkers[talker_i]->talkerReceive(message);
								if (talker_i + 1 < _downlinked_talkers_count || _downlinked_sockets_count) {
									socket.deserialize_buffer(message);
								}
							}
						}
					}
					break;
					
					case TalkerMatch::BY_NAME:
					{
						char message_to_name[NAME_LEN];
						strcpy(message_to_name, message.get_to_name());
						for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
							const char* talker_name = _downlinked_talkers[talker_i]->get_name();
							if (strcmp(talker_name, message_to_name) == 0) {
								_downlinked_talkers[talker_i]->talkerReceive(message);
								return true;
							}
						}
					}
					break;
					
					default: return false;
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
			case BroadcastValue::REMOTE:		// To downlinked nodes
			{
				if (_downlinked_talkers_count) {
					TalkerMatch talker_match = message.get_talker_match();
					// Talkers have no buffer, so a message copy will be necessary
					JsonMessage original_message(message);

					switch (talker_match) {

						case TalkerMatch::ANY:
						{
							for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count;) {
								_downlinked_talkers[talker_i++]->talkerReceive(message);
								if (talker_i < _downlinked_talkers_count || _downlinked_sockets_count) {
									message = original_message;
								}
							}
						}
						break;
						
						case TalkerMatch::BY_CHANNEL:
						{
							uint8_t message_channel = message.get_to_channel();
							for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
								uint8_t talker_channel = _downlinked_talkers[talker_i]->get_channel();
								if (talker_channel == message_channel) {
									_downlinked_talkers[talker_i]->talkerReceive(message);
									if (talker_i + 1 < _downlinked_talkers_count || _downlinked_sockets_count) {
										message = original_message;
									}
								}
							}
						}
						break;
						
						case TalkerMatch::BY_NAME:
						{
							char message_to_name[NAME_LEN];
							strcpy(message_to_name, message.get_to_name());
							for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
								const char* talker_name = _downlinked_talkers[talker_i]->get_name();
								if (strcmp(talker_name, message_to_name) == 0) {
									_downlinked_talkers[talker_i]->talkerReceive(message);
									return true;
								}
							}
						}
						break;
						
						default: return false;
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
			
			default: break;	// Does nothing, typical for BroadcastValue::NONE
		}
		return false;
	}

};



#endif // MESSAGE_REPEATER_HPP
