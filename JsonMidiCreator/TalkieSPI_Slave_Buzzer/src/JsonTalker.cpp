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


bool JsonTalker::remoteSend(JsonObject& json_message, bool as_reply, uint8_t target_index) {
    if (_muted || !_socket) return false;
	#ifdef JSON_TALKER_DEBUG
	Serial.print(_name);
	Serial.print(F(": "));
	Serial.println(F("Sending a REMOTE message"));
	#endif
	json_message[ JsonKey::CHECKSUM ] = REMOTE_C;	// 'c' = 0 means REMOTE_C communication
    return _socket->remoteSend(json_message, as_reply, target_index);
}


BroadcastSocket& JsonTalker::getSocket() {
	return *_socket;
}

void JsonTalker::set_delay(uint8_t delay) {
    return _socket->set_max_delay(delay);
}

uint8_t JsonTalker::get_delay() {
    return _socket->get_max_delay();
}

uint16_t JsonTalker::get_drops() {
    return _socket->get_drops_count();
}



