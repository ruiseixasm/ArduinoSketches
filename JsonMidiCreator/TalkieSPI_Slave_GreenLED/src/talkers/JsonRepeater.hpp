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
#ifndef JSON_REPEATER_HPP
#define JSON_REPEATER_HPP

#include "../JsonTalker.h"         // Includes the ArduinoJson Library

// #define REPEATER_TALKER_DEBUG


class JsonRepeater : public JsonTalker {
public:

    const char* class_name() const override { return "JsonRepeater"; }

    JsonRepeater(const char* name, const char* desc)
        : JsonTalker(name, desc) {}


	// Works as a router to LOCAL_C send
    bool processData(JsonObject& json_message) override {
        (void)pre_validated;	// Silence unused parameter warning

		#ifdef REPEATER_TALKER_DEBUG
		Serial.print(_name);
		Serial.print(F(": "));
		#endif
        uint16_t c = json_message["c"].as<uint16_t>();
		if (c == LOCAL_C) {
			#ifdef REPEATER_TALKER_DEBUG
			Serial.println(F("Received a LOCAL message"));
			#endif
			return remoteSend(json_message);
		}
		#ifdef REPEATER_TALKER_DEBUG
		Serial.println(F("Received a REMOTE message"));
		#endif
		return localSend(json_message);
	}
};


#endif // JSON_REPEATER_HPP
