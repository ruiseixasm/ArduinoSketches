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


    bool remoteSend(JsonObject& json_message, uint8_t target_index = 255) override;


	// Works as a repeater to LOCAL send
    bool processData(JsonObject& json_message) override {

		#ifdef JSON_REPEATER_DEBUG
		Serial.print(_name);
		Serial.print(F(": "));
		#endif
        SourceData c_source = static_cast<SourceData>( json_message[ JsonKey::SOURCE ].as<int>() );
		if (c_source == SourceData::LOCAL) {
			#ifdef JSON_REPEATER_DEBUG
			Serial.println(F("Received a LOCAL message"));
			#endif
			return remoteSend(json_message);
		}
		#ifdef JSON_REPEATER_DEBUG
		Serial.println(F("Received a REMOTE message"));
		#endif
		return localSend(json_message);
	}
};


#endif // JSON_REPEATER_H
