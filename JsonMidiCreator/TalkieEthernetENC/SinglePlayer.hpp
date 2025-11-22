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
#ifndef SINGLE_PLAYER_HPP
#define SINGLE_PLAYER_HPP

// 1. THE PURPOSE OF THE PLAYER HEADER LIBRARY IS TO MANAGE ITS OWN JSON_TALKIE INSTANTIATION
// 2. THE SOCKET IS THE SOLE RESPONSIBILITY OF THE `MAIN` SKETCH THAT IS SHARED AMONG ALL PLAYERS
// 3. THE PRESENT LIBRARY SHALL BE ABLE TO KEEP AND INSTANTIATE THE OTHER PARAMETERS NEEDED FOR
//     ITS OWN JSON_TALKIE INSTANTIATION


#define SOURCE_LIBRARY_MODE 1           // 1. JSON_TALKIE INSTANTIATION
//      0 - Arduino Library
//      1 - Project Library
//      2 - Arduino Copy Library


#if SOURCE_LIBRARY_MODE == 0
#include <JsonTalkie.hpp>

#elif SOURCE_LIBRARY_MODE == 1
#include "src/JsonTalkie.hpp"

#elif SOURCE_LIBRARY_MODE == 2
#include <Copy_JsonTalkie.hpp>
#endif

#include "src/BroadcastSocket.hpp"      // 2. THE SOCKET IS THE SOLE RESPONSIBILITY OF THE `MAIN` SKETCH


class SinglePlayer {
private:

    // 1. JSON_TALKIE INSTANTIATION
    JsonTalkie json_talkie;

    // 3. SELF KEPT PARAMETERS FOR JSON_TALKIE (NEED TO BE STATIC)
    static long bpm;

    static JsonTalkie::Device device;

    static bool set_bpm(JsonObject json_message, long bpm) {
        (void)json_message; // Silence unused parameter warning
        SinglePlayer::bpm = bpm;
        return true;
    }
    
    static long get_bpm(JsonObject json_message) {
        (void)json_message; // Silence unused parameter warning
        return bpm;
    }
    
    // Static command arrays (the main reason for being static, due to dynamic size)
    static JsonTalkie::Run runCommands[];
    static JsonTalkie::Set setCommands[];
    static JsonTalkie::Get getCommands[];

    // The final manifesto
    static JsonTalkie::Manifesto manifesto;
    
public:

    SinglePlayer(BroadcastSocket* socket) {
        // 2. THE SOCKET IS THE SOLE RESPONSIBILITY OF THE `MAIN` SKETCH
        json_talkie.plug_socket(socket);
        // 3. SELF KEPT PARAMETERS FOR JSON_TALKIE (NEED TO BE STATIC)
        json_talkie.set_manifesto(&manifesto);
    }

    void listen(bool receive = false) {
        json_talkie.listen(receive);
    }
    
};

// Static definitions (required for C++)

long SinglePlayer::bpm = 120.0;

JsonTalkie::Device SinglePlayer::device = {
    "sp1", "Single Player receiving commands from JsonTalkiePlayer"
};

JsonTalkie::Run SinglePlayer::runCommands[] = {
    // Mandatory parameter
};

JsonTalkie::Set SinglePlayer::setCommands[] = {
    {"bpm", "Sets the Tempo in BPM", &SinglePlayer::set_bpm}
};

JsonTalkie::Get SinglePlayer::getCommands[] = {
    {"bpm", "Gets the Tempo in BPM", &SinglePlayer::get_bpm}
};

JsonTalkie::Manifesto SinglePlayer::manifesto(
    &device,
    runCommands, sizeof(runCommands)/sizeof(JsonTalkie::Run),
    setCommands, sizeof(setCommands)/sizeof(JsonTalkie::Set),
    getCommands, sizeof(getCommands)/sizeof(JsonTalkie::Get),
    nullptr, nullptr
);



#endif // SINGLE_PLAYER_HPP
