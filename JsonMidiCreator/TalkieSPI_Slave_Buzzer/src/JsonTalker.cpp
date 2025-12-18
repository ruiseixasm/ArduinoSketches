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

#include "JsonTalker.h"         // Includes the ArduinoJson Library
#include "BroadcastSocket.hpp"    // MUST include the full definition!


JsonTalker** JsonTalker::_json_talkers = nullptr;
uint8_t JsonTalker::_talker_count = 0;


bool JsonTalker::remoteSend(JsonObject& json_message) {
    if (!_socket) return false;

	#ifdef JSON_TALKER_DEBUG
	Serial.print(F("\t"));
	Serial.print(_name);
	Serial.print(F(": "));
	Serial.println(F("Sending a REMOTE message"));
	#endif

	if (prepareMessage(json_message)) {
    	return _socket->remoteSend(json_message);
	}
	return false;
}

void JsonTalker::setSocket(BroadcastSocket* socket) {
	_socket = socket;
	// AVOIDS EVERY TALKER WITH THE SAME CHANNEL
	// XOR is great for 8-bit mixing
	if (_socket) {		// Safe code
		_channel ^= strlen(_socket->class_name());
	}
	_channel ^= strlen(this->class_name()) << 1;    // Shift before XOR
	_channel ^= strlen(_name) << 2;
	_channel ^= strlen(_desc) << 3;
	// Add microsecond LSB for entropy
	_channel ^= micros() & 0xFF;
	if (_manifesto) {	// Safe code
		_channel ^= strlen(_manifesto->class_name()) << 4;
		_channel = _manifesto->getChannel(_channel, this);
	}
}

BroadcastSocket& JsonTalker::getSocket() {
	return *_socket;
}

const char* JsonTalker::socket_class_name() {
	if (_socket) {	// Safe code
		return _socket->class_name();
	}
	return "";
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

