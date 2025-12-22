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

#include "JsonRepeater.h"         // Includes the ArduinoJson Library
#include "../BroadcastSocket.hpp"    // MUST include the full definition!


bool JsonRepeater::remoteSend(JsonObject& json_message, JsonMessage& new_json_message) {
    if (!_socket) return false;	// Ignores if it's muted or not

	#ifdef JSON_REPEATER_DEBUG
	Serial.print(_name);
	Serial.print(F(": "));
	Serial.println(F("Sending a REMOTE message"));
	#endif

	// DOESN'T SET IDENTITY, IT'S ONLY A REPEATER !!
	
    return _socket->remoteSend(json_message, new_json_message);
}


bool JsonRepeater::localSend(JsonObject& json_message, JsonMessage& new_json_message) {

	#ifdef JSON_TALKER_DEBUG
	Serial.print(F("\t"));
	Serial.print(_name);
	Serial.print(F(": "));
	Serial.println(F("Sending a LOCAL message"));
	#endif

	// DOESN'T CALL prepareMessage METHOD !!

	json_message[ TalkieKey::SOURCE ] = static_cast<int>(SourceValue::LOCAL);
	// Triggers all local Talkers to processes the json_message
	bool sent_message = false;
	bool pre_validated = false;
	for (uint8_t talker_i = 0; talker_i < _talker_count; ++talker_i) {
		if (_json_talkers[talker_i] != this && !_json_talkers[talker_i]->inSameSocket(_socket)) {  // Can't send to myself

			// CREATE COPY for each talker
			// JsonDocument in the stack makes sure its memory is released (NOT GLOBAL)
			#if ARDUINOJSON_VERSION_MAJOR >= 7
			JsonDocument doc_copy;
			#else
			StaticJsonDocument<BROADCAST_SOCKET_BUFFER_SIZE> doc_copy;
			#endif
			JsonObject json_copy = doc_copy.to<JsonObject>();
			// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (IN PROGRESS) ***************
			JsonMessage new_json_message_copy(new_json_message);
			
			// Copy all data from original
			for (JsonPair kv : json_message) {
				json_copy[kv.key()] = kv.value();
			}
		
			pre_validated = _json_talkers[talker_i]->processMessage(json_copy, new_json_message_copy);
			sent_message = true;
			if (!pre_validated) break;
		}
	}
	return sent_message;
}


