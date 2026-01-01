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

#include "JsonTalker.h"
#include "MessageRepeater.hpp"


void JsonTalker::setLink(MessageRepeater* message_repeater, LinkType link_type) {
	_message_repeater = message_repeater;
	_link_type = link_type;
}


bool JsonTalker::transmitToRepeater(JsonMessage& json_message) {

	#ifdef JSON_TALKER_DEBUG_NEW
	Serial.print(F("\t\t\ttransmitToRepeater(Talker): "));
	json_message.write_to(Serial);
	Serial.println();  // optional: just to add a newline after the JSON
	#endif

	if (_message_repeater && prepareMessage(json_message)) {
		if (_link_type == LinkType::TALKIE_DOWN_LINKED) {
			return _message_repeater->talkerUplink(*this, json_message);
		} else {
			return _message_repeater->talkerDownlink(*this, json_message);
		}
	}
	return false;
}


bool JsonTalker::transmitSockets(JsonMessage& json_message) {
	if (_message_repeater) {
		uint8_t socket_index = 0;
		const BroadcastSocket* socket;
		_message_repeater->iterateSocketsReset();
		while ((socket = _message_repeater->iterateSocketNext()) != nullptr) {	// No boilerplate
			json_message.set_nth_value_number(0, socket_index++);
			json_message.set_nth_value_string(1, socket->class_name());
			transmitToRepeater(json_message);	// Many-to-One
		}
		return socket_index > 0;
	}
	return false;
}


bool JsonTalker::transmitDrops(JsonMessage& json_message) {
	if (_message_repeater) {
		uint8_t socket_index = 0;
		const BroadcastSocket* socket;
		_message_repeater->iterateSocketsReset();
		while ((socket = _message_repeater->iterateSocketNext()) != nullptr) {	// No boilerplate
			json_message.set_nth_value_number(0, socket_index++);
			json_message.set_nth_value_number(1, socket->get_drops_count());
			transmitToRepeater(json_message);	// Many-to-One
		}
		return socket_index > 0;
	}
	return false;
}


bool JsonTalker::transmitDelays(JsonMessage& json_message) {
	if (_message_repeater) {
		uint8_t socket_index = 0;
		const BroadcastSocket* socket;
		_message_repeater->iterateSocketsReset();
		while ((socket = _message_repeater->iterateSocketNext()) != nullptr) {	// No boilerplate
			json_message.set_nth_value_number(0, socket_index++);
			json_message.set_nth_value_number(1, socket->get_max_delay());
			transmitToRepeater(json_message);	// Many-to-One
		}
		return socket_index > 0;
	}
	return false;
}


bool JsonTalker::setSocketDelay(uint8_t socket_index, uint8_t delay_value) {
	if (_message_repeater) {
		BroadcastSocket* socket = _message_repeater->accessSocket(socket_index);
		socket->set_max_delay(delay_value);
		return true;
	}
	return false;
}

