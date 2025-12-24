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
#ifndef I_MANIFESTO_HPP
#define I_MANIFESTO_HPP

#include <Arduino.h>
#include <ArduinoJson.h>    // Include ArduinoJson Library
#include "TalkieCodes.hpp"
#include "JsonMessage.hpp"

using MessageValue = TalkieCodes::MessageValue;
using ValueType = JsonMessage::ValueType;

class JsonTalker;

class TalkerManifesto {

public:

    virtual const char* class_name() const { return "TalkerManifesto"; }

    TalkerManifesto(const TalkerManifesto&) = delete;
    TalkerManifesto& operator=(const TalkerManifesto&) = delete;
    TalkerManifesto(TalkerManifesto&&) = delete;
    TalkerManifesto& operator=(TalkerManifesto&&) = delete;
    
    TalkerManifesto() = default;
    virtual ~TalkerManifesto() = default;


    struct Action {
        const char* name;
        const char* desc;
    };

	struct Original {
		uint16_t identity;
		MessageValue message_data;
	};

	static const char* valueKey(size_t nth = 0) {
		return TalkieCodes::valueKey(nth);
	}


protected:
	
    // Iterator states
    uint8_t actionsIterIdx = 0;

    virtual const Action* getActionsArray() const = 0;

    // Size methods
    virtual uint8_t actionsCount() const = 0;


public:

	virtual uint8_t getChannel(uint8_t channel, JsonTalker* talker) {
        (void)talker;		// Silence unused parameter warning
		return channel;
	}


    virtual void loop(JsonTalker* talker) {	// Also defined, not a pure virtual one
        (void)talker;		// Silence unused parameter warning
	}

	virtual void iterateActionsReset() {
		actionsIterIdx = 0;
	}


    // Iterator next methods - IMPLEMENTED in base class
    virtual const Action* iterateActionNext() {
        if (actionsIterIdx < actionsCount()) {
            return &getActionsArray()[actionsIterIdx++];
        }
        return nullptr;
    }
    
    // Name-based index search - IMPLEMENTED in base class
    virtual uint8_t actionIndex(const char* name) const {
        for (uint8_t i = 0; i < actionsCount(); i++) {
            if (strcmp(getActionsArray()[i].name, name) == 0) {
                return i;
            }
        }
        return 255;
    }
    
    // Numeric index validation
    virtual uint8_t actionIndex(uint8_t index) const {
        return (index < actionsCount()) ? index : 255;
    }
    
    // Action implementations - MUST be implemented by derived
    virtual bool actionByIndex(uint8_t index, JsonTalker& talker, JsonObject& old_json_message, JsonMessage& new_json_message) {
        (void)index;		// Silence unused parameter warning
        (void)talker;		// Silence unused parameter warning
        (void)old_json_message;	// Silence unused parameter warning
        (void)new_json_message;	// Silence unused parameter warning
        return false;
	}


    virtual void echo(JsonTalker& talker, JsonObject& old_json_message, JsonMessage& new_json_message) {
        (void)old_json_message;	// Silence unused parameter warning
        (void)talker;		// Silence unused parameter warning
        (void)new_json_message;	// Silence unused parameter warning
    }


    virtual void error(JsonTalker& talker, JsonObject& old_json_message, JsonMessage& new_json_message) {
        (void)old_json_message;	// Silence unused parameter warning
        (void)talker;		// Silence unused parameter warning
        (void)new_json_message;	// Silence unused parameter warning
    }

};


#endif // I_MANIFESTO_HPP
