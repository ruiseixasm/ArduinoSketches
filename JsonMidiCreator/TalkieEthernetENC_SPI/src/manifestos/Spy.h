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

	String original_talker = "";

	// ALWAYS MAKE SURE THE DIMENSIONS OF THE ARRAYS BELOW ARE THE CORRECT!

    Action runs[1] = {
		{"ping", "Ping every talker around me and report to you"}
    };
    
    Action sets[0] = {
    };
    
    Action gets[0] = {
    };

    const Action* getRunsArray() const override { return runs; }
    const Action* getSetsArray() const override { return sets; }
    const Action* getGetsArray() const override { return gets; }

    // Size methods
    uint8_t runsCount() const override { return sizeof(runs)/sizeof(Action); }
    uint8_t setsCount() const override { return sizeof(sets)/sizeof(Action); }
    uint8_t getsCount() const override { return sizeof(gets)/sizeof(Action); }


public:

    // Action implementations - MUST be implemented by derived
    bool runByIndex(uint8_t index, JsonObject& json_message, JsonTalker* talker) override;
    void echo(JsonObject& json_message, JsonTalker* talker) override;
    void error(JsonObject& json_message, JsonTalker* talker) override;

};


#endif // SPY_MANIFESTO_H
