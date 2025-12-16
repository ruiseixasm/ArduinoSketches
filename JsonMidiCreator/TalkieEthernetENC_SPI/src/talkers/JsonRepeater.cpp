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


bool JsonRepeater::remoteSend(JsonObject& json_message) {
    if (!_socket) return false;	// Ignores if it's muted or not

	#ifdef JSON_REPEATER_DEBUG
	Serial.print(_name);
	Serial.print(F(": "));
	Serial.println(F("Sending a REMOTE message"));
	#endif

	// DOESN'T SET IDENTITY, IT'S ONLY A REPEATER !!
	
    return _socket->remoteSend(json_message);
}


// Works as a repeater to LOCAL send
bool JsonRepeater::processData(JsonObject& json_message) {

	#ifdef JSON_REPEATER_DEBUG
	Serial.print(_name);
	Serial.print(F(": "));
	#endif
	SourceData source_data = static_cast<SourceData>( json_message[ JsonKey::SOURCE ].as<int>() );
	if (source_data == SourceData::LOCAL) {
		#ifdef JSON_REPEATER_DEBUG
		Serial.println(F("Received a LOCAL message"));
		#endif
		return remoteSend(json_message);
	}
	#ifdef JSON_REPEATER_DEBUG
	Serial.println(F("Received a REMOTE message"));
	#endif

	// Should make sure it doesn't send to same socket talkers, because they already received from REMOTE
    if (!_socket) return localSend(json_message);
	if (!_socket->hasTalker(this)) return localSend(json_message);
	return false;
}


