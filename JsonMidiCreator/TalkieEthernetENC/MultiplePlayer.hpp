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
#ifndef MULTIPLE_PLAYER_HPP
#define MULTIPLE_PLAYER_HPP

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


class MultiplePlayer {
private:

    // 1. JSON_TALKIE INSTANTIATION
    JsonTalkie json_talkie;

    // 3. SELF KEPT PARAMETERS FOR JSON_TALKIE (NEED TO BE STATIC)
    static long bpm_10;

    static JsonTalkie::Talker talker;

    static bool set_bpm_10(JsonObject json_message, long bpm_10) {
        (void)json_message; // Silence unused parameter warning
        MultiplePlayer::bpm_10 = bpm_10;
        return true;
    }
    
    static long get_bpm_10(JsonObject json_message) {
        (void)json_message; // Silence unused parameter warning
        return bpm_10;
    }
    
    // Static command arrays (the main reason for being static, due to dynamic size)
    static JsonTalkie::Run runCommands[];
    static JsonTalkie::Set setCommands[];
    static JsonTalkie::Get getCommands[];

    // The final manifesto
    static JsonTalkie::Manifesto manifesto;
    
public:

    MultiplePlayer(BroadcastSocket* socket) {
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

long MultiplePlayer::bpm_10 = 1200;

JsonTalkie::Talker MultiplePlayer::talker = {
    "mp1", "Multiple Player receiving commands from JsonTalkiePlayer"
};

JsonTalkie::Run MultiplePlayer::runCommands[] = {
    // Mandatory parameter
};

JsonTalkie::Set MultiplePlayer::setCommands[] = {
    {"bpm_10", "Sets the Tempo in BPM x 10", &MultiplePlayer::set_bpm_10}
};

JsonTalkie::Get MultiplePlayer::getCommands[] = {
    {"bpm_10", "Gets the Tempo in BPM x 10", &MultiplePlayer::get_bpm_10}
};

JsonTalkie::Manifesto MultiplePlayer::manifesto(
    &talker,
    runCommands, sizeof(runCommands)/sizeof(JsonTalkie::Run),
    setCommands, sizeof(setCommands)/sizeof(JsonTalkie::Set),
    getCommands, sizeof(getCommands)/sizeof(JsonTalkie::Get),
    nullptr, nullptr
);



#endif // MULTIPLE_PLAYER_HPP
