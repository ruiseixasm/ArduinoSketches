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
private:
    long _bpm_10 = 1200; // 120bpm x 10

    
public:
    MultiPlayer(const char* name, const char* desc) 
        : JsonTalker(name, desc) {  // Calls base class constructor
        // MultiPlayer-specific initialization here
    }

private:

    // const MultiPlayer::Run runCommands[2] = {
    //     // A list of Run structures
    //     {"on", "Turns led ON", &MultiPlayer::led_on},
    //     {"off", "Turns led OFF", &MultiPlayer::led_off}
    // };


    bool set_bpm_10(JsonObject json_message, long bpm_10) {
        (void)json_message; // Silence unused parameter warning
        _bpm_10 = bpm_10;
        return true;
    }

    // const MultiPlayer::Set _setCommands[1] = {
    //     // A list of Set structures
    //     {"bpm_10", "Sets the Tempo in BPM x 10", set_bpm_10}
    // };


    // long get_bpm_10(JsonObject json_message) {
    //     (void)json_message; // Silence unused parameter warning
    //     return _bpm_10;
    // }

    // const MultiPlayer::Get getCommands[3] = {
    //     // A list of Get structures
    //     {"runs", "Gets total runs", &MultiPlayer::get_total_runs},
    //     {"drops", "Gets total drops count", &MultiPlayer::get_total_drops},
    //     {"bpm_10", "Gets the Tempo in BPM x 10", &MultiPlayer::get_bpm_10}
    // };


public:

};


#endif // MULTI_PLAYER_HPP
