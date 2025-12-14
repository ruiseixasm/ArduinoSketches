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


class JsonTalker;

class IManifesto {

public:

    virtual const char* class_name() const { return "IManifesto"; }

    IManifesto(const IManifesto&) = delete;
    IManifesto& operator=(const IManifesto&) = delete;
    IManifesto(IManifesto&&) = delete;
    IManifesto& operator=(IManifesto&&) = delete;
    
    IManifesto() = default;
    virtual ~IManifesto() = default;

    enum EchoCode : int {
        ROGER,
        SAY_AGAIN,
        NEGATIVE
    };

    struct Action {
        const char* name;
        const char* desc;
    };


protected:
	
    // Iterator states
    uint8_t runsIterIdx = 0;
    uint8_t setsIterIdx = 0;
    uint8_t getsIterIdx = 0;

    virtual const Action* getRunsArray() const = 0;
    virtual const Action* getSetsArray() const = 0;
    virtual const Action* getGetsArray() const = 0;

    // Size methods
    virtual uint8_t runsCount() const = 0;
    virtual uint8_t setsCount() const = 0;
    virtual uint8_t getsCount() const = 0;


public:

    virtual void loop() {}	// Also defined, not a pure virtual one

	virtual void iterateRunsReset() {
		runsIterIdx = 0;
	}

	virtual void iterateSetsReset() {
		setsIterIdx = 0;
	}

	virtual void iterateGetsReset() {
		getsIterIdx = 0;
	}


    // Iterator next methods - IMPLEMENTED in base class
    virtual const Action* iterateRunsNext() {
        if (runsIterIdx < runsCount()) {
            return &getRunsArray()[runsIterIdx++];
        }
        return nullptr;
    }
    
    virtual const Action* iterateSetsNext() {
        if (setsIterIdx < setsCount()) {
            return &getSetsArray()[setsIterIdx++];
        }
        return nullptr;
    }
    
    virtual const Action* iterateGetsNext() {
        if (getsIterIdx < getsCount()) {
            return &getGetsArray()[getsIterIdx++];
        }
        return nullptr;
    }

    // Name-based index search - IMPLEMENTED in base class
    virtual uint8_t runIndex(const char* name) const {
        for (uint8_t i = 0; i < runsCount(); i++) {
            if (strcmp(getRunsArray()[i].name, name) == 0) {
                return i;
            }
        }
        return 255;
    }
    
    virtual uint8_t setIndex(const char* name) const {
        for (uint8_t i = 0; i < setsCount(); i++) {
            if (strcmp(getSetsArray()[i].name, name) == 0) {
                return i;
            }
        }
        return 255;
    }
    
    virtual uint8_t getIndex(const char* name) const {
        for (uint8_t i = 0; i < getsCount(); i++) {
            if (strcmp(getGetsArray()[i].name, name) == 0) {
                return i;
            }
        }
        return 255;
    }

    // Numeric index validation
    virtual uint8_t runIndex(uint8_t index) const {
        return (index < runsCount()) ? index : 255;
    }
    
    virtual uint8_t setIndex(uint8_t index) const {
        return (index < setsCount()) ? index : 255;
    }
    
    virtual uint8_t getIndex(uint8_t index) const {
        return (index < getsCount()) ? index : 255;
    }

    // Action implementations - MUST be implemented by derived
    virtual bool runByIndex(uint8_t index, JsonObject& json_message, JsonTalker* talker) = 0;
    virtual bool setByIndex(uint8_t index, uint32_t value, JsonObject& json_message, JsonTalker* talker) = 0;
    virtual uint32_t getByIndex(uint8_t index, JsonObject& json_message, JsonTalker* talker) const = 0;

};


#endif // I_MANIFESTO_HPP
