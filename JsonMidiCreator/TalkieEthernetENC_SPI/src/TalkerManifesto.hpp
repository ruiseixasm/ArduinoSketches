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

using MessageData = TalkieCodes::MessageData;

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


    struct Call {
        const char* name;
        const char* desc;
    };

	struct Original {
		uint16_t identity;
		MessageData message_data;
	};


protected:
	
    // Iterator states
    uint8_t callsIterIdx = 0;

    virtual const Call* getCallsArray() const = 0;

    // Size methods
    virtual uint8_t callsCount() const = 0;


public:

	virtual uint8_t getChannel(uint8_t channel, JsonTalker* talker) {
        (void)talker;		// Silence unused parameter warning
		return channel;
	}


    virtual void loop(JsonTalker* talker) {	// Also defined, not a pure virtual one
        (void)talker;		// Silence unused parameter warning
	}

	virtual void iterateCallsReset() {
		callsIterIdx = 0;
	}


    // Iterator next methods - IMPLEMENTED in base class
    virtual const Call* iterateCallsNext() {
        if (callsIterIdx < callsCount()) {
            return &getCallsArray()[callsIterIdx++];
        }
        return nullptr;
    }
    
    // Name-based index search - IMPLEMENTED in base class
    virtual uint8_t callIndex(const char* name) const {
        for (uint8_t i = 0; i < callsCount(); i++) {
            if (strcmp(getCallsArray()[i].name, name) == 0) {
                return i;
            }
        }
        return 255;
    }
    
    // Numeric index validation
    virtual uint8_t callIndex(uint8_t index) const {
        return (index < callsCount()) ? index : 255;
    }
    
    // Call implementations - MUST be implemented by derived
    virtual bool callByIndex(uint8_t index, JsonObject& json_message, JsonTalker* talker) {
        (void)index;		// Silence unused parameter warning
        (void)json_message;	// Silence unused parameter warning
        (void)talker;		// Silence unused parameter warning
        return false;
	}


    virtual void echo(JsonObject& json_message, JsonTalker* talker) {
        (void)json_message;	// Silence unused parameter warning
        (void)talker;		// Silence unused parameter warning
    }


    virtual void error(JsonObject& json_message, JsonTalker* talker) {
        (void)json_message;	// Silence unused parameter warning
        (void)talker;		// Silence unused parameter warning
    }

};


#endif // I_MANIFESTO_HPP
