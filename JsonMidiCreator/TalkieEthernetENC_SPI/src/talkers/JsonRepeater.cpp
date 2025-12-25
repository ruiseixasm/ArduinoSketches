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

#include "JsonRepeater.h"
#include "../BroadcastSocket.hpp"    // MUST include the full definition!


bool JsonRepeater::remoteSend(JsonMessage& json_message) {
    if (!_socket) return false;	// Ignores if it's muted or not

	#ifdef JSON_REPEATER_DEBUG
	Serial.print(_name);
	Serial.print(F(": "));
	Serial.println(F("Sending a REMOTE message"));
	#endif

	// DOESN'T SET IDENTITY, IT'S ONLY A REPEATER !!
	
    return _socket->remoteSend(json_message);
}


bool JsonRepeater::localSend(JsonMessage& json_message) {

	#ifdef JSON_TALKER_DEBUG
	Serial.print(F("\t"));
	Serial.print(_name);
	Serial.print(F(": "));
	Serial.println(F("Sending a LOCAL message"));
	#endif

	// DOESN'T CALL prepareMessage METHOD !!

	// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
	json_message.set_source_value(BroadcastValue::LOCAL);
	// Triggers all local Talkers to processes the json_message
	bool sent_message = false;
	TalkerMatch talker_match = TalkerMatch::NONE;
	for (uint8_t talker_i = 0; talker_i < _talker_count && talker_match > TalkerMatch::BY_NAME; ++talker_i) {

		#ifdef JSON_REPEATER_DEBUG_NEW
		Serial.print(F("\t\t\t\tlocalSend1.1: "));
		json_message.write_to(Serial);
		Serial.print(" | ");
		Serial.print(talker_i);
		Serial.print(" | ");
		Serial.println(_json_talkers[talker_i]->get_name());
		#endif

		if (_json_talkers[talker_i] != this && !_json_talkers[talker_i]->inSameSocket(_socket)) {  // Can't send to myself

			// CREATE COPY for each talker
			// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
			JsonMessage json_message_copy(json_message);
			
			talker_match = _json_talkers[talker_i]->processMessage(json_message_copy);
			sent_message = true;
		}
	}
	return sent_message;
}


