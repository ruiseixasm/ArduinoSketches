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

#include "../TalkerManifesto.hpp"

#define SPY_MANIFESTO_DEBUG


class Spy : public TalkerManifesto {
public:

    const char* class_name() const override { return "Spy"; }

    Spy() : TalkerManifesto() {}	// Constructor


protected:

	String _original_talker = "";
	Original _original_message = {0, MessageValue::NOISE};

	// ALWAYS MAKE SURE THE DIMENSIONS OF THE ARRAYS BELOW ARE THE CORRECT!

    Action calls[3] = {
		{"ping", "I ping every talker, a named one, or by channel"},
		{"ping_self", "I can even ping myself"},
		{"call", "I can call actions on others [<talker> <action>]"}
    };
    
    const Action* getActionsArray() const override { return calls; }

    // Size methods
    uint8_t actionsCount() const override { return sizeof(calls)/sizeof(Action); }


public:

    // Action implementations - MUST be implemented by derived
    bool actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& json_message) override;
    void echo(JsonTalker& talker, JsonMessage& json_message) override;
    void error(JsonTalker& talker, JsonMessage& json_message) override;

};


#endif // SPY_MANIFESTO_H
