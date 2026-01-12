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
#include "TalkerManifesto.hpp"
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
			default: break;
		}
	}
	return false;
}


uint8_t JsonTalker::_socketsCount() {
	if (_message_repeater) {
		uint8_t countUplinkedSockets = _message_repeater->_uplinkedSocketsCount();
		uint8_t countDownlinkedSockets = _message_repeater->_downlinkedSocketsCount();
		return countUplinkedSockets + countDownlinkedSockets;
	}
	return 0;
}


BroadcastSocket* JsonTalker::_getSocket(uint8_t socket_index) {
	if (_message_repeater) {
		uint8_t countUplinkedSockets = _message_repeater->_uplinkedSocketsCount();
		if (socket_index < countUplinkedSockets) {
			return _message_repeater->_getUplinkedSocket(socket_index);
		} else {
			return _message_repeater->_getDownlinkedSocket(socket_index - countUplinkedSockets);
		}
	}
	return nullptr;
}


const char* JsonTalker::_manifesto_name() const {
	if (_manifesto) {
		return _manifesto->class_name();
	}
	return nullptr;
}


void JsonTalker::_loop() {
	if (_manifesto) _manifesto->_loop(*this);
}


uint8_t JsonTalker::_actionsCount() const {
	if (_manifesto) {
		return _manifesto->_actionsCount();
	}
	return 0;
}

const Action* JsonTalker::_getActionsArray() const {
	if (_manifesto) {
		return _manifesto->_getActionsArray();
	}
	return nullptr;
}


uint8_t JsonTalker::_actionIndex(const char* name) const {
	if (_manifesto) {
		return _manifesto->_actionIndex(name);
	}
	return 255;
}

uint8_t JsonTalker::_actionIndex(uint8_t index) const {
	if (_manifesto) {
		return _manifesto->_actionIndex(index);
	}
	return 255;
}

bool JsonTalker::_actionByIndex(uint8_t index, JsonMessage& json_message, TalkerMatch talker_match) {
	if (_manifesto) {
		return _manifesto->_actionByIndex(index, *this, json_message, talker_match);
	}
	return false;
}


void JsonTalker::_echo(JsonMessage& json_message, TalkerMatch talker_match) {
	if (_manifesto) _manifesto->_echo(*this, json_message, talker_match);
}

void JsonTalker::_error(JsonMessage& json_message, TalkerMatch talker_match) {
	if (_manifesto) _manifesto->_error(*this, json_message, talker_match);
}

void JsonTalker::_noise(JsonMessage& json_message, TalkerMatch talker_match) {
	if (_manifesto) _manifesto->_noise(*this, json_message, talker_match);
}

