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
#ifndef DEVICE_TALKER_HPP
#define DEVICE_TALKER_HPP

#include "BroadcastSocket.hpp"
#include <ArduinoJson.h>    // Include ArduinoJson Library

class DeviceTalker {
public:

    struct Talk {
        const char* name;      // Name of the Talker
        const char* desc;      // Description of the Device
    };

    struct Run {
        const char* name;      // "buzz", "print", etc.
        const char* desc;      // Description
        bool (DeviceTalker::*method)(JsonObject);
    };

    struct Set {
        const char* name;      // "buzz", "print", etc.
        const char* desc;      // Description
        bool (DeviceTalker::*method)(JsonObject, long);
    };

    struct Get {
        const char* name;      // "buzz", "print", etc.
        const char* desc;      // Description
        long (DeviceTalker::*method)(JsonObject);
    };

private:

    Talk* talk = nullptr;
    Run* runCommands = nullptr;
    size_t runSize = 0;
    Set* setCommands = nullptr;
    size_t setSize = 0;
    Get* getCommands = nullptr;
    size_t getSize = 0;
    bool (DeviceTalker::*echo)(JsonObject) = nullptr;
    bool (DeviceTalker::*error)(JsonObject) = nullptr;

public:

    Run* get_run(const char* cmd) {
        for (size_t index = 0; index < runSize; ++index) {
            if (strcmp(cmd, runCommands[index].name) == 0) {
                return &runCommands[index];  // Returns the function
            }
        }
        return nullptr;
    }

    Set* get_set(const char* cmd) {
        for (size_t index = 0; index < setSize; ++index) {
            if (strcmp(cmd, setCommands[index].name) == 0) {
                return &setCommands[index];  // Returns the function
            }
        }
        return nullptr;
    }

    Get* get_get(const char* cmd) {
        for (size_t index = 0; index < getSize; ++index) {
            if (strcmp(cmd, getCommands[index].name) == 0) {
                return &getCommands[index];  // Returns the function
            }
        }
        return nullptr;
    }



};


#endif // DEVICE_TALKER_HPP
