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
#ifndef BROADCAST_SOCKET_SERIAL_HPP
#define BROADCAST_SOCKET_SERIAL_HPP

#include "../BroadcastSocket.hpp"


// #define BROADCAST_SERIAL_DEBUG

class BroadcastSocket_Serial : public BroadcastSocket {
public:
    // Singleton accessor
    static BroadcastSocket_Serial& instance() {
        static BroadcastSocket_Serial instance;
        return instance;
    }

    
    bool send(const char* data, size_t len, bool as_reply = false) override {
        return Serial.write(data, len) == len;
    }


    size_t receive(char* buffer, size_t size) override {
        return static_cast<size_t>(Serial.readBytes(buffer, size));
    }
};

#endif // BROADCAST_SOCKET_SERIAL_HPP
