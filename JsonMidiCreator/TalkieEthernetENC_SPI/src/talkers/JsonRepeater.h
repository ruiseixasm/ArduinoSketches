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
#ifndef JSON_REPEATER_H
#define JSON_REPEATER_H

#include "../JsonTalker.h"

// #define JSON_REPEATER_DEBUG
// #define JSON_REPEATER_DEBUG_NEW


class JsonRepeater : public JsonTalker {
public:

    const char* class_name() const override { return "JsonRepeater"; }

    JsonRepeater(const char* name, const char* desc)	// Has no Manifesto, it's just a repeater, without Calls
        : JsonTalker(name, desc, nullptr) {}


    bool remoteSend(JsonMessage& json_message) override;
    bool localSend(JsonMessage& json_message) override;

	
	TalkerMatch processMessage(JsonMessage& json_message) override {

		#ifdef JSON_REPEATER_DEBUG
		Serial.print(F("\t"));
		Serial.print(_name);
		Serial.print(F(": "));
		#endif

		// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
		BroadcastValue broadcast_value = json_message.get_source_value();

		#ifdef JSON_REPEATER_DEBUG_NEW
		Serial.print(F("\t\t\t\tprocessMessage1.1: "));
		json_message.write_to(Serial);
		Serial.println();
		Serial.print(F("\t\t\t\tprocessMessage1.2: "));
		json_message.write_to(Serial);
		Serial.print(" | ");
		Serial.println(_name);
		#endif

		switch (broadcast_value) {

			case BroadcastValue::REMOTE:
				#ifdef JSON_REPEATER_DEBUG
				Serial.println(F("Repeated a REMOTE message LOCALLY"));
				#endif
				localSend(json_message);	// Cross transmission
				return TalkerMatch::NONE;
			
			case BroadcastValue::SELF:
				#ifdef JSON_REPEATER_DEBUG
				Serial.println(F("Repeated an SELF message to SELF"));
				#endif
				selfSend(json_message);	// Straight transmission
				return TalkerMatch::NONE;
			
			case BroadcastValue::NONE:
				#ifdef JSON_REPEATER_DEBUG
				Serial.println(F("\tTransmitted an SELF message"));
				#endif
				noneSend(json_message);	// Straight transmission
				return TalkerMatch::NONE;

			// By default it's sent to REMOTE because it's safer ("c" = 0 auto set by socket)
			default: break;
		}
		// By default it's sent to REMOTE because it's safer ("c" = 0 auto set by socket)
		#ifdef JSON_REPEATER_DEBUG
		Serial.println(F("Repeated a LOCAL message REMOTELY"));
		#endif
		remoteSend(json_message);			// Cross transmission
		return TalkerMatch::NONE;
	}

};


#endif // JSON_REPEATER_H
