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
#ifndef MULTI_PLAYER_HPP
#define MULTI_PLAYER_HPP

#include "JsonTalker.h"         // Includes the ArduinoJson Library

// #define MULTI_PLAYER_DEBUG


class MultiPlayer : public JsonTalker {
public:

    const char* class_name() const override { return "MultiPlayer"; }

    MultiPlayer(const char* name, const char* desc)
        : JsonTalker(name, desc) {}


protected:

    // Can't use method reference because these type of references are class designation dependent,
    // so, they will prevent any possibility of inheritance, this way the only alternative is to
    // do an indirect reference based on the command position with only a pair of strings and
    // a following switch case sequence that picks the respective method and calls it directly.

    // Virtual method that returns static manifesto (can't override class member variables)
    const Manifesto& get_manifesto() const override {

        static const Manifesto _manifesto = {
            (const Command[]){  // runs
                {"on", "Turns led ON"},
                {"off", "Turns led OFF"}
            },
            (const Command[]){  // sets 
                {"delay", "Sets the socket max delay"},
                {"bpm_10", "Sets the Tempo in BPM x 10"}
            },
            (const Command[]){  // gets
                {"delay", "Gets the socket max delay"},
                {"drops", "Gets total drops count"},
                {"runs", "Gets total runs"},
                {"bpm_10", "Gets the Tempo in BPM x 10"}
            },
            2,
            2,
            4
        };

        return _manifesto;
    }
    

    uint16_t _bpm_10 = 1200;

    bool command_run(const uint8_t command_index, JsonObject json_message) override {
        (void)json_message; // Silence unused parameter warning
        switch (command_index)
        {
        
        default: return JsonTalker::command_run(command_index, json_message);
        }
    }

    
    bool command_set(const uint8_t command_index, JsonObject json_message) override {
        uint32_t json_value = json_message["v"].as<uint32_t>();
        switch (command_index)
        {
        case 1:
            {
                _bpm_10 = json_value;
                return true;
            }
            break;
        
        default: return JsonTalker::command_set(command_index, json_message);
        }
    }

    
    uint32_t command_get(const uint8_t command_index, JsonObject json_message) override {
        (void)json_message; // Silence unused parameter warning
        switch (command_index)
        {
        case 3:
            {
                return _bpm_10;
            }
            break;
        
        default: return JsonTalker::command_get(command_index, json_message);
        }
    }

};


#endif // MULTI_PLAYER_HPP
