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


void JsonTalker::_setLink(MessageRepeater* message_repeater, LinkType link_type) {
	_message_repeater = message_repeater;
	_link_type = link_type;
}


bool JsonTalker::transmitToRepeater(JsonMessage& json_message) {

	#ifdef JSON_TALKER_DEBUG_NEW
	Serial.print(F("\t\t\t_transmitToRepeater(Talker): "));
	json_message.write_to(Serial);
	Serial.println();  // optional: just to add a newline after the JSON
	#endif

	if (_message_repeater && _prepareMessage(json_message)) {
		switch (_link_type) {
			case LinkType::TALKIE_LT_UP_LINKED:
			case LinkType::TALKIE_LT_UP_BRIDGED:
				return _message_repeater->_talkerDownlink(*this, json_message);
			case LinkType::TALKIE_LT_DOWN_LINKED:
				return _message_repeater->_talkerUplink(*this, json_message);
			default: return false;
		}
	}
}

uint8_t JsonTalker::socketsCount() {
	if (_message_repeater) {
		uint8_t countUplinkedSockets = _message_repeater->uplinkedSocketsCount();
		uint8_t countDownlinkedSockets = _message_repeater->downlinkedSocketsCount();
		return countUplinkedSockets + countDownlinkedSockets;
	}
	return 0;
}


BroadcastSocket* JsonTalker::getSocket(uint8_t socket_index) {
	if (_message_repeater) {
		uint8_t countUplinkedSockets = _message_repeater->uplinkedSocketsCount();
		if (socket_index < countUplinkedSockets) {
			return _message_repeater->getUplinkedSocket(socket_index);
		} else {
			return _message_repeater->getDownlinkedSocket(socket_index - countUplinkedSockets);
		}
	}
	return nullptr;
}


