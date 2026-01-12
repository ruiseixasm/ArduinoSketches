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

#include "BroadcastSocket.h"
#include "MessageRepeater.hpp"


// #define MESSAGE_REPEATER_DEBUG


void BroadcastSocket::_setLink(MessageRepeater* message_repeater, LinkType link_type) {
	_message_repeater = message_repeater;
	_link_type = link_type;
}


void BroadcastSocket::_transmitToRepeater(JsonMessage& json_message) {

	#ifdef MESSAGE_REPEATER_DEBUG
	Serial.print(F("\t\t_transmitToRepeater(Socket): "));
	json_message.write_to(Serial);
	Serial.println();  // optional: just to add a newline after the JSON
	#endif

	if (_message_repeater) {
		switch (_link_type) {
			case LinkType::TALKIE_LT_UP_LINKED:
			case LinkType::TALKIE_LT_UP_BRIDGED:
				_message_repeater->_socketDownlink(*this, json_message);
				break;
			case LinkType::TALKIE_LT_DOWN_LINKED:
				_message_repeater->_socketUplink(*this, json_message);
				break;
			default: break;
		}
	}	
}

