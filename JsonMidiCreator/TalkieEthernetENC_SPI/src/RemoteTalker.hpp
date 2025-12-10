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
#ifndef REMOTE_TALKER_HPP
#define REMOTE_TALKER_HPP

#include "JsonTalker.h"         // Includes the ArduinoJson Library

// #define MULTI_PLAYER_DEBUG


class RemoteTalker : public JsonTalker {
public:

    const char* class_name() const override { return "RemoteTalker"; }

    RemoteTalker(const char* name, const char* desc)
        : JsonTalker(name, desc) {}


	// Works as a router to SPI send
    bool processData(JsonObject& json_message, bool pre_validated = false) override {
        (void)pre_validated;	// Silence unused parameter warning

        uint16_t c = json_message["c"].as<uint16_t>();
		if (c == 0) {	// From remote
			return localSend(json_message);
		} else {		// From local
			return remoteSend(json_message);
		}
	}
};


#endif // REMOTE_TALKER_HPP
