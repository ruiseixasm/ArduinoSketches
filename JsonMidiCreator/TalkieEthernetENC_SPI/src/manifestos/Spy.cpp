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
#include "Spy.h"
#include "../JsonTalker.h"  // It NEEDS to know because the .h file doesn't

    
using MessageData = TalkieCodes::MessageData;
using SystemData = TalkieCodes::SystemData;

// Index-based operations (simplified examples)
bool Spy::runByIndex(uint8_t index, JsonObject& json_message, JsonTalker* talker) {
	
	if (index < runsCount()) {
		// Actual implementation would do something based on index
		switch(index) {
			case 0:
			{
				// from-to already reversed
				// In the beginning I know no one, so I ask them to talk
				json_message[ JsonKey::MESSAGE ] = static_cast<int>(MessageData::TALK);
				// Keep track of the original talker
				original_talker = json_message[ JsonKey::TO ].as<String>();	// Explicit conversion
				json_message.remove( JsonKey::TO );	// Needs to be for everybody
				talker->replyMessage(json_message);
				return true;
			}
			break;
		}
	}
	return false;
}


void Spy::echo(JsonObject& json_message, JsonTalker* talker) {
	
	MessageData original_message = static_cast<MessageData>(json_message[ JsonKey::ORIGINAL ].as<int>());

	switch (original_message) {

	case MessageData::TALK:
		// from-to not yet reversed, doing it now
		json_message[ JsonKey::TO ] = json_message[ JsonKey::FROM ];
		json_message[ JsonKey::FROM ] = talker->get_name();
		// Now that they started to talk, I ask for more information
		json_message[ JsonKey::MESSAGE ] = static_cast<int>(MessageData::SYS);
		json_message[ JsonKey::SYSTEM ] = static_cast<int>(SystemData::PING);
		// The id will be set by me at the exit point (same clock)
		talker->replyMessage(json_message);
		break;
	
	case MessageData::SYS:
		// Need to check first if it's a answer to a ping
		if (json_message[ JsonKey::SYSTEM ] == static_cast<int>(SystemData::PING)) {
			// Time to calculate the delay
			uint16_t actual_time = static_cast<uint16_t>(millis());
			uint16_t message_time = json_message[ JsonKey::IDENTITY ];
			uint16_t time_delay = actual_time - message_time;
			// Report back to the original talker T as echo (it's waiting for it)
			json_message[ JsonKey::TO ] = original_talker;	// Already an echo message
			json_message[ JsonKey::VALUE ] = time_delay;
			talker->replyMessage(json_message);
		}
		break;

	default:
		break;
	}
}


void Spy::error(JsonObject& json_message, JsonTalker* talker) {
	(void)talker;		// Silence unused parameter warning
	Serial.print(json_message[ JsonKey::FROM ].as<String>());
	Serial.print(" - ");
	if (json_message["r"].is<String>()) {
		Serial.println(json_message["r"].as<String>());
	} else if (json_message[ JsonKey::DESCRIPTION ].is<String>()) {
		Serial.println(json_message[ JsonKey::DESCRIPTION ].as<String>());
	} else {
		Serial.println(F("Empty error received!"));
	}
}

