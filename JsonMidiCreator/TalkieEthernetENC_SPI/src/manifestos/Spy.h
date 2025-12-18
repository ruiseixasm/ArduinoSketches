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
#ifndef SPY_MANIFESTO_H
#define SPY_MANIFESTO_H

#include "../IManifesto.hpp"

// #define SPY_MANIFESTO_DEBUG

using JsonKey = TalkieCodes::JsonKey;

class Spy : public IManifesto {
public:

    const char* class_name() const override { return "Spy"; }

    Spy() : IManifesto() {}	// Constructor


protected:

	String _original_talker = "";
	String _ping_talker = "";
	bool _ping = false;

	// ALWAYS MAKE SURE THE DIMENSIONS OF THE ARRAYS BELOW ARE THE CORRECT!

    Call calls[1] = {
		{"ping", "I ping every talker around me"}
    };
    
    const Call* getCallsArray() const override { return calls; }

    // Size methods
    uint8_t callsCount() const override { return sizeof(calls)/sizeof(Call); }


public:

	void loop(JsonTalker* talker) override;

    // Call implementations - MUST be implemented by derived
    bool callByIndex(uint8_t index, JsonObject& json_message, JsonTalker* talker) override;
    void echo(JsonObject& json_message, JsonTalker* talker) override;
    void error(JsonObject& json_message, JsonTalker* talker) override;

};


#endif // SPY_MANIFESTO_H
