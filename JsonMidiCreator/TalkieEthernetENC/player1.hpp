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
#ifndef PLAYER_1_HPP
#define PLAYER_1_HPP

#include <JsonTalkie.hpp>


class Player1 {
private:
    // Static command arrays
    static JsonTalkie::Set setCommands[];
    static JsonTalkie::Get getCommands[];
    
    // Static member variables (instead of instance variables)
    static long bpm_n;
    static long bpm_d;
    // ... other static state

public:
    // Static JsonTalkie instance
    static JsonTalkie json_talkie;
    
    // Static initialization method
    static void begin() {
        json_talkie = JsonTalkie(
            &device,
            setCommands, sizeof(setCommands)/sizeof(JsonTalkie::Set),
            getCommands, sizeof(getCommands)/sizeof(JsonTalkie::Get)
        );
        
        // Initialize static state
        bpm_n = 120;
        bpm_d = 4;
        // ... other initialization
    }
    
    // Static methods (no 'this' pointer needed)
    static bool set_bpm_n(JsonObject json_message, long value) {
        bpm_n = value;
        // Add any validation logic here
        return true;
    }
    
    static bool set_bpm_d(JsonObject json_message, long value) {
        bpm_d = value;
        // Add any validation logic here
        return true;
    }
    
    static bool get_bpm_n(JsonObject json_message, long value) {
        // Write current value back to JSON
        json_message["value"] = bpm_n;
        return true;
    }
    
    static bool get_bpm_d(JsonObject json_message, long value) {
        // Write current value back to JSON
        json_message["value"] = bpm_d;
        return true;
    }
    
    static void listen() {
        json_talkie.listen();
    }
    
    // Static getters for other parts of your code
    static long get_current_bpm_n() { return bpm_n; }
    static long get_current_bpm_d() { return bpm_d; }
};

// Static definitions (required for C++)
JsonTalkie Player1::json_talkie;

long Player1::bpm_n = 120;
long Player1::bpm_d = 4;

JsonTalkie::Set Player1::setCommands[] = {
    {"bpm_n", &Player1::set_bpm_n},
    {"bpm_d", &Player1::set_bpm_d}
    // Add more commands as needed
};

JsonTalkie::Get Player1::getCommands[] = {
    {"bpm_n", &Player1::get_bpm_n},
    {"bpm_d", &Player1::get_bpm_d}
    // Add more commands as needed
};


#endif // PLAYER_1_HPP
