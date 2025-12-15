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


bool JsonRepeater::remoteSend(JsonObject& json_message, bool as_reply, uint8_t target_index) {
    if (!_socket) return false;	// Ignores if it's muted or not

	#ifdef JSON_REPEATER_DEBUG
	Serial.print(_name);
	Serial.print(F(": "));
	Serial.println(F("Sending a REMOTE message"));
	#endif


	// DOESN'T SET IDENTITY, IT'S ONLY A REPEATER !!
	

    return _socket->remoteSend(json_message, as_reply, target_index);
}

