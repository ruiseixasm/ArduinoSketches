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
#include "BroadcastSocket.h"    // MUST include the full definition!
#include "MessageRepeater.hpp"


JsonTalker* const* JsonTalker::_json_talkers = nullptr;
uint8_t JsonTalker::_talker_count = 0;


void JsonTalker::setLink(MessageRepeater* message_repeater, LinkType link_type) {
	_message_repeater = message_repeater;
	_link_type = link_type;
}


bool JsonTalker::transmitToRepeater(JsonMessage& json_message) {
	if (_message_repeater && prepareMessage(json_message)) {
		switch (_link_type) {

			case LinkType::UP:
			{
				return _message_repeater->talkerDownlink(*this, json_message);
			}
			break;
			
			case LinkType::DOWN:
			{
				return _message_repeater->talkerUplink(*this, json_message);
			}
			break;
			
			default: break;
		}
	}
	return false;
}


// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
bool JsonTalker::socketSend(JsonMessage& json_message) {
    if (!_socket) return false;

	#ifdef JSON_TALKER_DEBUG
	Serial.print(F("\t"));
	Serial.print(_name);
	Serial.print(F(": "));
	Serial.println(F("Sending a REMOTE message"));
	#endif

	// Tags the message as REMOTE sourced
	// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
	json_message.set_broadcast_value(BroadcastValue::REMOTE);
	if (prepareMessage(json_message)) {
    	return _socket->socketSend(json_message);
	}
	return false;
}


void JsonTalker::setSocket(BroadcastSocket* socket) {
	_socket = socket;
}

BroadcastSocket& JsonTalker::getSocket() {
	return *_socket;
}

const char* JsonTalker::socket_class_name() {
	if (_socket) {	// Safe code
		return _socket->class_name();
	}
	return "none";
}

void JsonTalker::set_delay(uint8_t delay) {
	if (_socket) {	// Safe code
    	return _socket->set_max_delay(delay);
	}
}

uint8_t JsonTalker::get_delay() {
	if (_socket) {	// Safe code
    	return _socket->get_max_delay();
	}
	return 0xFF;	// 255
}

uint16_t JsonTalker::get_drops() {
	if (_socket) {	// Safe code
    	return _socket->get_drops_count();
	}
	return 0xFFFF;	// 2^16 - 1
}

