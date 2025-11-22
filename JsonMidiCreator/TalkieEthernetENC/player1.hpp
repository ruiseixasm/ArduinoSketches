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

    // Tempo data
    int _bpm_numerator = 120;   // bpm_n
    int _bpm_denominator = 1;   // bpm_d
    JsonTalkie json_talkie;

    JsonTalkie::Device device = {
        "player1", "First player that processes JsonMidiCreator data"
    };


    bool set_bpm_n(JsonObject json_message, long bpm_n);
    bool set_bpm_d(JsonObject json_message, long bpm_d);
    JsonTalkie::Set setCommands[] = {
        {"bpm_n", "Sets the Tempo numerator (BPM)", set_bpm_n},
        {"bpm_d", "Sets the Tempo denominator (BPM)", set_bpm_d}
    };


    long get_bpm_n(JsonObject json_message);
    long get_bpm_d(JsonObject json_message);
    JsonTalkie::Get getCommands[] = {
        {"bpm_n", "Gets the Tempo numerator (BPM)", get_bpm_n},
        {"bpm_d", "Gets the Tempo denominator (BPM)", get_bpm_d}
    };


    JsonTalkie::Manifesto manifesto(
        &device,
        setCommands, sizeof(setCommands)/sizeof(JsonTalkie::Set),
        getCommands, sizeof(getCommands)/sizeof(JsonTalkie::Get)
    );



    bool set_bpm_n(JsonObject json_message, long bpm_n) {
        (void)json_message; // Silence unused parameter warning
        _bpm_numerator = static_cast<int>(bpm_n);
        return true;
    }

    bool set_bpm_d(JsonObject json_message, long bpm_d) {
        (void)json_message; // Silence unused parameter warning
        _bpm_denominator = static_cast<int>(bpm_d);
        return true;
    }


public:
    Player1() {
        
        json_talkie.set_manifesto(&manifesto);
    }

    void plug_socket(BroadcastSocket *socket) {
        json_talkie.plug_socket(socket);
    }

    void listen(bool receive = false) {
        json_talkie.listen(receive);
    }

};


#endif // PLAYER_1_HPP
