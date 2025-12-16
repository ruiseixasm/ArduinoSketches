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


bool JsonTalker::remoteSend(JsonObject& json_message, uint8_t target_index) {
    if (!_socket) return false;

	#ifdef JSON_TALKER_DEBUG
	Serial.print(_name);
	Serial.print(F(": "));
	Serial.println(F("Sending a REMOTE message"));
	#endif


	// It also sets the IDENTITY if applicable, these settings are of the Talker exclusive responsibility (NO DELEGATION TO SOCKET !!)
	
	MessageData message_code = static_cast<MessageData>(json_message[ JsonKey::MESSAGE ].as<int>());
	if (message_code < MessageData::ECHO) {

		#ifdef JSON_TALKER_DEBUG
		Serial.print(F("remoteSend1: Setting a new identifier (i) for :"));
		serializeJson(json_message, Serial);
		Serial.println();  // optional: just to add a newline after the JSON
		#endif

		if (_muted_action && message_code < MessageData::TALK) return false;

		json_message[ JsonKey::IDENTITY ] = (uint16_t)millis();

	} else if (!json_message[ JsonKey::IDENTITY ].is<uint16_t>()) { // Makes sure response messages have an "i" (identifier)

		#ifdef JSON_TALKER_DEBUG
		Serial.print(F("remoteSend1: Response message with a wrong or without an identifier, now being set (i): "));
		serializeJson(json_message, Serial);
		Serial.println();  // optional: just to add a newline after the JSON
		#endif

		json_message[ JsonKey::MESSAGE ] = static_cast<int>(MessageData::ERROR);
		json_message[ JsonKey::ERROR ] = static_cast<int>(ErrorData::IDENTITY);
		json_message[ JsonKey::IDENTITY ] = (uint16_t)millis();

	} else {
		
		#ifdef JSON_TALKER_DEBUG
		Serial.print(F("remoteSend1: Keeping the same identifier (i): "));
		serializeJson(json_message, Serial);
		Serial.println();  // optional: just to add a newline after the JSON
		#endif

	}

    return _socket->remoteSend(json_message, target_index);
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

