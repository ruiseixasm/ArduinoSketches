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

#include "BroadcastSocket.h"    // MUST include the full definition!
#include "MessageRepeater.hpp"


// #define MESSAGE_REPEATER_DEBUG


void BroadcastSocket::setLink(MessageRepeater* message_repeater, LinkType link_type) {
	_message_repeater = message_repeater;
	_link_type = link_type;
}


bool BroadcastSocket::transmitToRepeater(JsonMessage& json_message) {

	#ifdef MESSAGE_REPEATER_DEBUG
	Serial.print(F("\t\ttransmitToRepeater(Socket): "));
	json_message.write_to(Serial);
	Serial.println();  // optional: just to add a newline after the JSON
	#endif

	if (_message_repeater) {
		if (_link_type == LinkType::TALKIE_DOWN_LINKED) {
			return _message_repeater->socketUplink(*this, json_message);
		} else {
			return _message_repeater->socketDownlink(*this, json_message);
		}
	}
	return false;
}

