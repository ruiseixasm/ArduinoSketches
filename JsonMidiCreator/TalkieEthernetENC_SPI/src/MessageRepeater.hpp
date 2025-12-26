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
#include "BroadcastSocket.hpp"
#include "JsonTalker.h"

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
			_uplink_sockets[uplink_socket_i]->setLinkType(LinkType::UP);
		}
		for (uint8_t downlink_socket_i = 0; downlink_socket_i < _downlink_talkers_count; ++downlink_socket_i) {
			_downlink_talkers[downlink_socket_i]->setLinkType(LinkType::DOWN);
		}
		for (uint8_t downlink_socket_i = 0; downlink_socket_i < _downlink_sockets_count; ++downlink_socket_i) {
			_downlink_sockets[downlink_socket_i]->setLinkType(LinkType::DOWN);
		}
		for (uint8_t uplink_socket_i = 0; uplink_socket_i < _uplink_talkers_count; ++uplink_socket_i) {
			_uplink_talkers[uplink_socket_i]->setLinkType(LinkType::UP);
		}
	}

	~MessageRepeater() {
		// Does nothing
	}


	void socketDownlink(BroadcastSocket &socket, JsonMessage &message) {

	}

	void talkerUplink(JsonTalker &talker, JsonMessage &message) {

	}

	void socketUplink(BroadcastSocket &socket, JsonMessage &message) {

	}

	void talkerDownlink(JsonTalker &talker, JsonMessage &message) {

	}

};



#endif // MESSAGE_REPEATER_HPP
