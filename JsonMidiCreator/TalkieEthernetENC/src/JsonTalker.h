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


// #define JSON_TALKER_DEBUG

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

    static uint16_t setChecksum(JsonObject message);
    
    static uint32_t generateMessageId() {
        // Generates a 32-bit wrapped timestamp ID using overflow.
        return (uint32_t)millis();  // millis() is already an unit32_t (unsigned long int) data return
    }


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

    BroadcastSocket* _socket = nullptr;
    const char* _name;      // Name of the Talker
    const char* _desc;      // Description of the Device
    uint8_t _channel = 0;

    
    long total_runs = 0;
    bool is_led_on = false;  // keep track of state yourself, by default it's off

    bool led_on(JsonObject json_message) {
        if (!is_led_on) {
        #ifdef LED_BUILTIN
            digitalWrite(LED_BUILTIN, HIGH);
        #endif
            is_led_on = true;
            total_runs++;
        } else {
            json_message["r"] = "Already On!";
            if (_socket != nullptr)
                this->sendMessage(json_message);
            return false;
        }
        return true;
    }

    bool led_off(JsonObject json_message) {
        if (is_led_on) {
        #ifdef LED_BUILTIN
            digitalWrite(LED_BUILTIN, LOW);
        #endif
            is_led_on = false;
            total_runs++;
        } else {
            json_message["r"] = "Already Off!";
            if (_socket != nullptr)
                this->sendMessage(json_message);
            return false;
        }
        return true;
    }

    const JsonTalker::Run runCommands[2] = {
        // A list of Run structures
        {"on", "Turns led ON", &JsonTalker::led_on},
        {"off", "Turns led OFF", &JsonTalker::led_off}
    };


    const JsonTalker::Set setCommands[0] = {
        // A list of Set structures
        // {"bpm_10", "Sets the Tempo in BPM x 10", set_bpm_10}
    };


    long get_total_runs(JsonObject json_message) {
        (void)json_message; // Silence unused parameter warning
        return total_runs;
    }

    const JsonTalker::Get getCommands[1] = {
        // A list of Get structures
        {"runs", "Gets total runs", &JsonTalker::get_total_runs}
    };


public:

    // Explicit default constructor
    JsonTalker() = delete;
        
    JsonTalker(const char* name, const char* desc)
        : _name(name), _desc(desc) {}

    void setSocket(BroadcastSocket* socket) {
        _socket = socket;
    }

    bool echo(JsonObject json_message) {
        Serial.print(json_message["f"].as<String>());
        Serial.print(" - ");
        if (json_message["r"].is<String>()) {
            Serial.println(json_message["r"].as<String>());
        } else if (json_message["d"].is<String>()) {
            Serial.println(json_message["d"].as<String>());
        } else {
            Serial.println(F("Empty echo received!"));
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
            Serial.println(F("Empty error received!"));
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
        return _name;
    }
    void set_channel(uint8_t channel) { _channel = channel; }
    uint8_t get_channel() { return _channel; }
    
    bool sendMessage(JsonObject message, bool as_reply = false);
    bool processData(const char* received_data, const size_t data_len, bool pre_validated = false);

};


#endif // DEVICE_TALKER_HPP
