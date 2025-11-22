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
#ifndef BROADCAST_SOCKET_HPP
#define BROADCAST_SOCKET_HPP

#include <Arduino.h>    // Needed for Serial given that Arduino IDE only includes Serial in .ino files!


// #define BROADCASTSOCKET_DEBUG

class BroadcastSocket {
protected:
    static uint16_t _port;

    // Private constructor
    BroadcastSocket() = default;
    virtual ~BroadcastSocket() = default;

    static size_t jsonStrip(char* buffer, size_t length) {

        // Find the first '{' (start of JSON)
        size_t json_start = 0;
        while (buffer[json_start] != '{' && json_start < length) {
            
            #ifdef BROADCASTSOCKET_DEBUG
            if (json_start == 0)
                Serial.println(F("Json LEFT strip"));
            #endif
            
            json_start++;
        }

        // If no '{', discard
        if (json_start == length) {

            #ifdef BROADCASTSOCKET_DEBUG
            Serial.println(F("Json '{' NOT found"));
            #endif
            
            return 0;
        }

        // Find the first '}' (finish of JSON)
        size_t json_finish = length - 1;  // json_start and json_finish are indexes, NOT sizes
        while (buffer[json_finish] != '}' && json_finish > json_start) {

            #ifdef BROADCASTSOCKET_DEBUG
            if (json_finish == length - 1)
                Serial.println(F("Json RIGHT strip"));
            #endif
            
            json_finish--;
        }

        // If no '}', discard
        if (json_finish == json_start) {
            
            #ifdef BROADCASTSOCKET_DEBUG
            Serial.println(F("Json '}' NOT found"));
            #endif
            
            return 0;
        }

        // Shift JSON to start of buffer if needed
        if (json_start > 0) {
            // Copies "numBytes" bytes from address "from" to address "to"
            // void * memmove(void *to, const void *from, size_t numBytes);
            memmove(buffer, buffer + json_start, json_finish - json_start + 1);
        }

        // Return actual JSON length (including both braces)
        return json_finish - json_start + 1;
    }

public:
    // Delete copy/move operations
    BroadcastSocket(const BroadcastSocket&) = delete;
    BroadcastSocket& operator=(const BroadcastSocket&) = delete;
    BroadcastSocket(BroadcastSocket&&) = delete;
    BroadcastSocket& operator=(BroadcastSocket&&) = delete;

    // Pure virtual methods remain unchanged
    virtual bool send(const char* data, size_t len, bool as_reply = false) = 0;
    virtual size_t receive(char* buffer, size_t size) = 0;
    
    virtual void set_port(uint16_t port) { _port = port; }
    virtual uint16_t get_port() { return _port; }
};

// Static member initialization
uint16_t BroadcastSocket::_port = 5005; // The default port


#endif // BROADCAST_SOCKET_HPP
