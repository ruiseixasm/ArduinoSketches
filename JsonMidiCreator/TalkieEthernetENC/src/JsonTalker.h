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

#include <ArduinoJson.h>    // Include ArduinoJson Library


// #define DEVICE_TALKER_DEBUG

// Readjust if absolutely necessary
#define BROADCAST_SOCKET_BUFFER_SIZE 128


class BroadcastSocket;


class JsonTalker {
private:
    
    // Shared processing data buffer Not reentrant, received data unaffected
    static char _buffer[BROADCAST_SOCKET_BUFFER_SIZE];


public:

    enum class MessageCode {
        talk,
        list,
        run,
        set,
        get,
        sys,
        echo,
        error,
        channel
    };


    static uint16_t getChecksum(const char* net_data, const size_t len) {
        // 16-bit word and XORing
        uint16_t checksum = 0;
        for (size_t i = 0; i < len; i += 2) {
            uint16_t chunk = net_data[i] << 8;
            if (i + 1 < len) {
                chunk |= net_data[i + 1];
            }
            checksum ^= chunk;
        }
        return checksum;
    }


    static uint16_t setChecksum(JsonObject message) {
        message["c"] = 0;   // makes _buffer a net_data buffer
        size_t len = serializeJson(message, _buffer, BROADCAST_SOCKET_BUFFER_SIZE);
        uint16_t checksum = getChecksum(_buffer, len);
        message["c"] = checksum;
        return checksum;
    }
    

    static uint32_t generateMessageId() {
        // Generates a 32-bit wrapped timestamp ID using overflow.
        return (uint32_t)millis();  // millis() is already an unit32_t (unsigned long int) data return
    }




    struct Talk {
        const char* name;      // Name of the Talker
        const char* desc;      // Description of the Device
    };

    struct Run {
        const char* name;      // "buzz", "print", etc.
        const char* desc;      // Description
        bool (JsonTalker::*method)(JsonObject);
    };

    struct Set {
        const char* name;      // "buzz", "print", etc.
        const char* desc;      // Description
        bool (JsonTalker::*method)(JsonObject, long);
    };

    struct Get {
        const char* name;      // "buzz", "print", etc.
        const char* desc;      // Description
        long (JsonTalker::*method)(JsonObject);
    };

private:

    Talk* _talk = nullptr;
    uint8_t _channel = 0;
    uint32_t _sent_set_time[2] = {0};   // Keeps two time stamp
    String _set_name = "";              // Keeps the talker name
    bool _check_set_time = false;

public:

    const JsonTalker::Run runCommands[0] = {
        // A list of Run structures
        // {"buzz", "Triggers buzzing", buzz}
    };

    const JsonTalker::Set setCommands[0] = {
        // A list of Set structures
        // {"bpm_10", "Sets the Tempo in BPM x 10", set_bpm_10}
    };

    const JsonTalker::Get getCommands[0] = {
        // A list of Get structures
        // {"bpm_10", "Gets the Tempo in BPM x 10", get_bpm_10}
    };





    // Explicit default constructor
    JsonTalker() = default;
    


    bool echo(JsonObject json_message) {
        Serial.print(json_message["f"].as<String>());
        Serial.print(" - ");
        if (json_message["r"].is<String>()) {
            Serial.println(json_message["r"].as<String>());
        } else if (json_message["d"].is<String>()) {
            Serial.println(json_message["d"].as<String>());
        } else {
            Serial.println("Empty echo received!");
        }
        return false;
    }

    bool error(JsonObject json_message) {
        Serial.print(json_message["f"].as<String>());
        Serial.print(" - ");
        if (json_message["r"].is<String>()) {
            Serial.println(json_message["r"].as<String>());
        } else if (json_message["d"].is<String>()) {
            Serial.println(json_message["d"].as<String>());
        } else {
            Serial.println("Empty error received!");
        }
        return false;
    }



    size_t runs_count() { return sizeof(runCommands)/sizeof(JsonTalker::Run); }
    const Run* run(const char* cmd) {
        for (size_t index = 0; index < runs_count(); ++index) {
            if (strcmp(cmd, runCommands[index].name) == 0) {
                return &runCommands[index];  // Returns the function
            }
        }
        return nullptr;
    }

    size_t sets_count() { return sizeof(setCommands)/sizeof(JsonTalker::Set); }
    const Set* set(const char* cmd) {
        for (size_t index = 0; index < sets_count(); ++index) {
            if (strcmp(cmd, setCommands[index].name) == 0) {
                return &setCommands[index];  // Returns the function
            }
        }
        return nullptr;
    }

    size_t gets_count() { return sizeof(getCommands)/sizeof(JsonTalker::Get); }
    const Get* get(const char* cmd) {
        for (size_t index = 0; index < gets_count(); ++index) {
            if (strcmp(cmd, getCommands[index].name) == 0) {
                return &getCommands[index];  // Returns the function
            }
        }
        return nullptr;
    }

    
    const char* get_name() {
        if (_talk != nullptr) return _talk->name;
        return nullptr;
    }
    void set_channel(uint8_t channel) { _channel = channel; }
    uint8_t get_channel() { return _channel; }


    
    bool sendMessage(BroadcastSocket* socket, JsonObject message, bool as_reply = false);
    void receiveData(BroadcastSocket* socket, const char* received_data, const size_t data_len);




};


char JsonTalker::_buffer[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};



#endif // DEVICE_TALKER_HPP
