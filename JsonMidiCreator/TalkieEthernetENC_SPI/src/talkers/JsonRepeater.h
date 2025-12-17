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

#include "../JsonTalker.h"         // Includes the ArduinoJson Library

// #define JSON_REPEATER_DEBUG


class JsonRepeater : public JsonTalker {
public:

    const char* class_name() const override { return "JsonRepeater"; }

    JsonRepeater(const char* name, const char* desc)	// Has no Manifesto, it's just a repeater, without Actions
        : JsonTalker(name, desc, nullptr) {}


    bool remoteSend(JsonObject& json_message) override;
    bool localSend(JsonObject& json_message) override;

	
	bool processMessage(JsonObject& json_message) override {

		#ifdef JSON_REPEATER_DEBUG
		Serial.print(F("\t"));
		Serial.print(_name);
		Serial.print(F(": "));
		#endif

		if (json_message[ JsonKey::SOURCE ].is<int>()) {
			SourceData source_data = static_cast<SourceData>( json_message[ JsonKey::SOURCE ].as<int>() );
			switch (source_data) {

				case SourceData::REMOTE:
					#ifdef JSON_REPEATER_DEBUG
					Serial.println(F("Repeated a REMOTE message LOCALLY"));
					#endif
					return localSend(json_message);	// Cross transmission
				
				case SourceData::HERE:
					#ifdef JSON_REPEATER_DEBUG
					Serial.println(F("Repeated an HERE message to HERE"));
					#endif
					return hereSend(json_message);	// Straight transmission
				
				// By default it's sent to REMOTE because it's safer ("c" = 0 auto set by socket)
				default: break;
			}
		}
		// By default it's sent to REMOTE because it's safer ("c" = 0 auto set by socket)
		#ifdef JSON_REPEATER_DEBUG
		Serial.println(F("Repeated a LOCAL message REMOTELY"));
		#endif
		return remoteSend(json_message);			// Cross transmission
	}

};


#endif // JSON_REPEATER_H
