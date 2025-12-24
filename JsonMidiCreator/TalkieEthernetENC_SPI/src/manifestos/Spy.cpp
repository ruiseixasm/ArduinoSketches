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

    
using MessageValue = TalkieCodes::MessageValue;
using InfoValue = TalkieCodes::InfoValue;



// Index-based operations (simplified examples)
bool Spy::actionByIndex(uint8_t index, JsonTalker& talker, JsonObject& old_json_message, JsonMessage& new_json_message) {
	
	bool ping = false;

	// As a spy it only answers to REMOTE calls
	SourceValue source_data = static_cast<SourceValue>( old_json_message[ TalkieKey::SOURCE ].as<int>() );
	if (source_data == SourceValue::REMOTE) {

		if (index < actionsCount()) {
			// Actual implementation would do something based on index
			switch(index) {
				case 0:
				{
					ping = true;
					// 1. Start by collecting info from message
					_original_talker = old_json_message[ TalkieKey::FROM ].as<String>();	// Explicit conversion
					_original_message.identity = old_json_message[ TalkieKey::IDENTITY ].as<uint16_t>();
					_original_message.message_data = MessageValue::PING;	// It's is the emulated message (not CALL)
					// 2. Repurpose it to be a LOCAL PING
					old_json_message[ TalkieKey::MESSAGE ] = static_cast<int>(MessageValue::PING);
					old_json_message.remove( TalkieKey::IDENTITY );	// Makes sure a new IDENTITY is set
					if (old_json_message[ valueKey(0) ].is<String>()) {
						old_json_message[ TalkieKey::TO ] = old_json_message[ valueKey(0) ].as<String>();
					} else {	// Removes the original TO
						old_json_message.remove( TalkieKey::TO );	// Without TO works as broadcast
					}
					old_json_message[ TalkieKey::FROM ] = talker.get_name();	// Avoids swapping
					// 3. Sends the message LOCALLY
					talker.localSend(old_json_message, new_json_message);	// Dispatches it directly as LOCAL
					// 4. Finally, makes sure the message isn't returned to the REMOTE sender by setting its source as NONE
					old_json_message[ TalkieKey::SOURCE ] = static_cast<int>(SourceValue::NONE);
				}
				break;
				case 1:
				{
					ping = true;
					// 1. Start by collecting info from message
					_original_talker = old_json_message[ TalkieKey::FROM ].as<String>();	// Explicit conversion
					_original_message.identity = old_json_message[ TalkieKey::IDENTITY ].as<uint16_t>();
					_original_message.message_data = MessageValue::PING;	// It's is the emulated message (not CALL)
					// 2. Repurpose it to be a SELF PING
					old_json_message[ TalkieKey::MESSAGE ] = static_cast<int>(MessageValue::PING);
					old_json_message.remove( TalkieKey::IDENTITY );	// Makes sure a new IDENTITY is set
					old_json_message[ TalkieKey::FROM ] = talker.get_name();	// Avoids swapping
					// 3. Sends the message to myself
					talker.selfSend(old_json_message, new_json_message);	// Dispatches it directly as LOCAL
					// 4. Finally, makes sure the message isn't returned to the REMOTE sender by setting its source as NONE
					old_json_message[ TalkieKey::SOURCE ] = static_cast<int>(SourceValue::NONE);
				}
				break;
			}
		}
	}
	return ping;
}


void Spy::echo(JsonTalker& talker, JsonObject& old_json_message, JsonMessage& new_json_message) {
	
	Original original_message = talker.get_original();
	switch (original_message.message_data) {

		case MessageValue::PING:
			{
				// In condition to calculate the delay right away, no need to extra messages
				uint16_t actual_time = static_cast<uint16_t>(millis());
				uint16_t message_time = old_json_message[ TalkieKey::TIMESTAMP ].as<uint16_t>();	// must have
				uint16_t time_delay = actual_time - message_time;
				old_json_message[ valueKey(0) ] = time_delay;
				old_json_message[ valueKey(1) ] = old_json_message[ TalkieKey::FROM ];	// Informs the Talker as reply
				// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (IN PROGRESS) ***************
				new_json_message.set_nth_value_number(0, time_delay);
				new_json_message.set_nth_value_string(1, new_json_message.get_from());
				// Prepares headers for the original REMOTE sender
				old_json_message[ TalkieKey::TO ] = _original_talker;
				old_json_message[ TalkieKey::FROM ] = talker.get_name();	// Avoids swapping
				// Emulates the REMOTE original call
				old_json_message[ TalkieKey::IDENTITY ] = _original_message.identity;
				// It's already an ECHO message, it's because of that that entered here
				// Finally answers to the REMOTE caller by repeating all other json fields
				talker.remoteSend(old_json_message, new_json_message);
			}
			break;

		default: break;	// Ignores all the rest
	}
}


void Spy::error(JsonTalker& talker, JsonObject& old_json_message, JsonMessage& new_json_message) {
	(void)talker;				// Silence unused parameter warning
	(void)new_json_message;		// Silence unused parameter warning
	Serial.print(old_json_message[ TalkieKey::FROM ].as<String>());
	Serial.print(" - ");
	if (old_json_message["r"].is<String>()) {
		Serial.println(old_json_message["r"].as<String>());
	} else if (old_json_message[ valueKey(0) ].is<String>()) {
		Serial.println(old_json_message[ valueKey(0) ].as<String>());
	} else {
		Serial.println(F("Empty error received!"));
	}
}

