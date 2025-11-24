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

#include "JsonTalker.h"    // Include ArduinoJson Library


// #define MULTI_PLAYER_DEBUG


class MultiPlayer : public JsonTalker {
private:


    
public:
    MultiPlayer(const char* name, const char* desc) 
        : JsonTalker(name, desc) {  // Calls base class constructor
        // MultiPlayer-specific initialization here
    }

private:

    // const JsonTalker::Run runCommands[2] = {
    //     // A list of Run structures
    //     {"on", "Turns led ON", &JsonTalker::led_on},
    //     {"off", "Turns led OFF", &JsonTalker::led_off}
    // };


    // const JsonTalker::Set setCommands[0] = {
    //     // A list of Set structures
    //     // {"bpm_10", "Sets the Tempo in BPM x 10", set_bpm_10}
    // };


    // long get_total_runs(JsonObject json_message) {
    //     (void)json_message; // Silence unused parameter warning
    //     return total_runs;
    // }

    // long get_total_drops(JsonObject json_message);

    // const JsonTalker::Get getCommands[2] = {
    //     // A list of Get structures
    //     {"runs", "Gets total runs", &JsonTalker::get_total_runs},
    //     {"drops", "Gets total drops count", &JsonTalker::get_total_drops}
    // };


public:

};


#endif // MULTI_PLAYER_HPP
