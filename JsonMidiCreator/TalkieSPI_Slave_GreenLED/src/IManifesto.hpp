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

    enum EchoCode : int {
        ROGER,
        SAY_AGAIN,
        NEGATIVE
    };

    IManifesto(const IManifesto&) = delete;
    IManifesto& operator=(const IManifesto&) = delete;
    IManifesto(IManifesto&&) = delete;
    IManifesto& operator=(IManifesto&&) = delete;
    
    IManifesto() = default;
    virtual ~IManifesto() = default;

    struct Action {
        const char* name;
        const char* desc;
    };

    virtual const char* class_name() const { return "IManifesto"; }


    // Size methods
    virtual uint8_t runsCount() const = 0;
    virtual uint8_t setsCount() const = 0;
    virtual uint8_t getsCount() const = 0;


    // These methods are intended to be used in a for loop where they return an Action at each iteration
    virtual bool iterateRunsReset() = 0;
    virtual Action* iterateRunsNext() = 0;  // Returns nullptr when done

    virtual bool iterateSetsReset() = 0;
    virtual Action* iterateSetsNext() = 0;
    
    virtual bool iterateGetsReset() = 0;
    virtual Action* iterateGetsNext() = 0;


    // These methods are intended to call the respective ByIndex when it's find based on a name
    virtual uint8_t runIndex(const char* name) const = 0;
    virtual uint8_t setIndex(const char* name) const = 0;
    virtual uint8_t getIndex(const char* name) const = 0;

    // These methods are intended to call the respective ByIndex number and return it if in range or 255 if not
    virtual uint8_t runIndex(uint8_t index) const = 0;
    virtual uint8_t setIndex(uint8_t index) const = 0;
    virtual uint8_t getIndex(uint8_t index) const = 0;

    // These methods use a switch that based on the index pick the respective action to be done
    virtual bool runByIndex(uint8_t index, JsonObject& json_message, JsonTalker* talker) = 0;
    virtual bool setByIndex(uint8_t index, uint32_t value, JsonObject& json_message, JsonTalker* talker) = 0;
    virtual uint32_t getByIndex(uint8_t index, JsonObject& json_message, JsonTalker* talker) const = 0;
};


#endif // I_MANIFESTO_HPP
