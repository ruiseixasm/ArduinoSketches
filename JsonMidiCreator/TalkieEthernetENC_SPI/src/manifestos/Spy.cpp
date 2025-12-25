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
bool Spy::actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& new_json_message) {
	
	bool ping = false;

	// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
	// As a spy it only answers to REMOTE calls
	SourceValue source_data = new_json_message.get_source_value();
	if (source_data == SourceValue::REMOTE) {

		if (index < actionsCount()) {
			// Actual implementation would do something based on index
			switch(index) {
				case 0:
				{
					ping = true;
					// 1. Start by collecting info from message
					_original_talker = new_json_message.get_from_name();
					_original_message.identity = new_json_message.get_identity();
					_original_message.message_data = MessageValue::PING;	// It's is the emulated message (not CALL)
					// 2. Repurpose it to be a LOCAL PING
					new_json_message.set_message(MessageValue::PING);
					new_json_message.remove_identity();
					if (new_json_message.get_nth_value_type(0) == ValueType::STRING) {
						new_json_message.set_to_name(new_json_message.get_nth_value_string(0));
					} else {	// Removes the original TO
						new_json_message.remove_to();	// Without TO works as broadcast
					}
					new_json_message.set_from_name(talker.get_name());	// Avoids the swapping
					// 3. Sends the message LOCALLY
					talker.localSend(new_json_message);	// Dispatches it directly as LOCAL
					// 4. Finally, makes sure the message isn't returned to the REMOTE sender by setting its source as NONE
					new_json_message.set_source_value(SourceValue::NONE);
				}
				break;
				case 1:
				{
					ping = true;
					// 1. Start by collecting info from message
					_original_talker = new_json_message.get_from_name();	// Explicit conversion
					_original_message.identity = new_json_message.get_identity();
					_original_message.message_data = MessageValue::PING;	// It's is the emulated message (not CALL)
					// 2. Repurpose it to be a SELF PING
					new_json_message.set_message(MessageValue::PING);
					new_json_message.remove_identity();	// Makes sure a new IDENTITY is set
					new_json_message.set_from_name(talker.get_name());	// Avoids swapping
					// 3. Sends the message to myself
					talker.selfSend(new_json_message);	// Dispatches it directly as LOCAL
					// 4. Finally, makes sure the message isn't returned to the REMOTE sender by setting its source as NONE
					new_json_message.set_source_value(SourceValue::NONE);
				}
				break;
			}
		}
	}
	return ping;
}


void Spy::echo(JsonTalker& talker, JsonMessage& new_json_message) {
	
	Original original_message = talker.get_original();
	switch (original_message.message_data) {

		// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
		case MessageValue::PING:
			{
				// In condition to calculate the delay right away, no need to extra messages
				uint16_t actual_time = static_cast<uint16_t>(millis());
				uint16_t message_time = new_json_message.get_timestamp();	// must have
				uint16_t time_delay = actual_time - message_time;
				new_json_message.set_nth_value_number(0, time_delay);
				new_json_message.set_nth_value_string(1, new_json_message.get_from_name());
				// Prepares headers for the original REMOTE sender
				new_json_message.set_to_name(_original_talker);
				new_json_message.set_from_name(talker.get_name());
				// Emulates the REMOTE original call
				new_json_message.set_identity(_original_message.identity);
				// It's already an ECHO message, it's because of that that entered here
				// Finally answers to the REMOTE caller by repeating all other json fields
				talker.remoteSend(new_json_message);
			}
			break;

		default: break;	// Ignores all the rest
	}
}


void Spy::error(JsonTalker& talker, JsonMessage& new_json_message) {
	// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
	Serial.print(new_json_message.get_from_name());
	Serial.print(" - ");
	ValueType value_type = new_json_message.get_nth_value_type(0);
	switch (value_type) {

		case ValueType::STRING:
			Serial.println(new_json_message.get_nth_value_string(0));
			break;
		
		case ValueType::NUMBER:
			Serial.println(new_json_message.get_nth_value_number(0));
			break;
		
		default:
			Serial.println(F("Empty error received!"));
			break;
	}
}

