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


void Spy::loop(JsonTalker* talker) {
	
	if (_ping) {
		_ping = false;
		
		// CREATE a new json_message
		// JsonDocument in the stack makes sure its memory is released (NOT GLOBAL)
		#if ARDUINOJSON_VERSION_MAJOR >= 7
		JsonDocument doc_copy;
		#else
		StaticJsonDocument<BROADCAST_SOCKET_BUFFER_SIZE> doc_copy;
		#endif
		JsonObject json_message = doc_copy.to<JsonObject>();

		json_message[ JsonKey::MESSAGE ] = static_cast<int>(MessageData::PING);
		if (_ping_talker != "") {
			json_message[ JsonKey::TO ] = _ping_talker;
		}
		// Missing TO makes it a Broadcast message
		// FROM and TIMESTAMP is automatically set by Send method
		talker->localSend(json_message);	// Only inquires locally
	}
}


// Index-based operations (simplified examples)
bool Spy::callByIndex(uint8_t index, JsonObject& json_message, JsonTalker* talker) {
	
	// As a spy it only answers to REMOTE calls
	SourceData source_data = static_cast<SourceData>( json_message[ JsonKey::SOURCE ].as<int>() );
	if (source_data == SourceData::REMOTE) {

		if (index < callsCount()) {
			// Actual implementation would do something based on index
			switch(index) {
				case 0:
				{	// Has FROM for sure
					_original_talker = json_message[ JsonKey::FROM ].as<String>();	// Explicit conversion
					if (json_message[ valueKey(0) ].is<String>()) {
						_ping_talker = json_message[ valueKey(0) ].as<String>();
					} else {
						_ping_talker = "";
					}
					_ping = true;
					return true;
				}
				break;
			}
		}
	}
	return false;
}


void Spy::echo(JsonObject& json_message, JsonTalker* talker) {
	
	MessageData original_message = static_cast<MessageData>( json_message[ JsonKey::ORIGINAL ].as<int>() );
	switch (original_message) {

		case MessageData::PING:
			{
				// In condition to calculate the delay right away, no need to extra messages
				uint16_t actual_time = static_cast<uint16_t>(millis());
				uint16_t message_time = json_message[ JsonKey::TIMESTAMP ].as<uint16_t>();	// must have
				uint16_t time_delay = actual_time - message_time;
				json_message[ valueKey(0) ] = time_delay;
				json_message[ JsonKey::REPLY ] = json_message[ JsonKey::FROM ];	// Informs the Talker as reply
				// Prepares headers for the original REMOTE sender
				json_message[ JsonKey::TO ] = _original_talker;
				json_message[ JsonKey::FROM ] = talker->get_name();	// Avoids swapping
				// Finally answers to the REMOTE caller by repeating all other json fields
				talker->remoteSend(json_message);
			}
			break;

		default: break;	// Ignores all the rest
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

