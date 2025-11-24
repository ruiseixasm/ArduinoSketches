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

#include "JsonTalker.h"    // Includes the ArduinoJson Library


// #define MULTI_PLAYER_DEBUG


class MultiPlayer : public JsonTalker {
public:
    MultiPlayer(const char* name, const char* desc) 
        : JsonTalker(name, desc) {  // Calls base class constructor
        // MultiPlayer-specific initialization here
    }

protected:

    long _bpm_10 = 1200; // 120bpm x 10

    // Let subclasses override these
    virtual const Set* get_set_commands() { return _setCommands; }
    virtual size_t get_set_commands_count() { return sizeof(_setCommands)/sizeof(Set); }


    bool set_bpm_10(JsonObject json_message, long bpm_10) {
        (void)json_message; // Silence unused parameter warning
        _bpm_10 = bpm_10;
        return true;
    }

    // const MultiPlayer::Set _setCommands[1] = {
    //     // A list of Set structures
    //     {"bpm_10", "Sets the Tempo in BPM x 10", &MultiPlayer::set_bpm_10}
    // };
    
};


#endif // MULTI_PLAYER_HPP
