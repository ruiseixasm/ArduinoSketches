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
#ifndef JSON_TALKER_H
#define JSON_TALKER_H

#include <ArduinoJson.h>    // Include ArduinoJson Library


#define JSON_TALKER_DEBUG

// Readjust if absolutely necessary
#define BROADCAST_SOCKET_BUFFER_SIZE 128


class BroadcastSocket;


class JsonTalker {
private:
    
    // Shared processing data buffer Not reentrant, received data unaffected
    static char _buffer[BROADCAST_SOCKET_BUFFER_SIZE];


public:

    virtual const char* class_name() const { return "JsonTalker"; }

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

    // Now without a method reference `bool (JsonTalker::*method)(JsonObject, long)`
    struct Command {
        const char* name;
        const char* desc;
    };

    struct Manifesto {
        const Command* runs;
        const Command* sets;
        const Command* gets;
        const uint8_t runs_count;
        const uint8_t sets_count;
        const uint8_t gets_count;
    };

protected:

    BroadcastSocket* _socket = nullptr;
    const char* _name;      // Name of the Talker
    const char* _desc;      // Description of the Device
    uint8_t _channel = 0;

    
    long _total_runs = 0;
    // static becaus it's a shared state among all other talkers, device (board) parameter
    static bool _is_led_on;  // keep track of state yourself, by default it's off


    // Can't use method reference because these type of references are class designation dependent,
    // so, they will prevent any possibility of inheritance, this way the only alternative is to
    // do an indirect reference based on the command position with only a pair of strings and
    // a following switch case sequence that picks the respective method and calls it directly.

    // Virtual method that returns static manifesto (can't override class member variables)
    virtual const Manifesto& get_manifesto() const {

        static const Manifesto _manifesto = {
            (const Command[]){  // runs
                {"on", "Turns led ON"},
                {"off", "Turns led OFF"}
            },
            (const Command[]){  // sets 
                {"delay", "Sets the socket max delay"}
            },
            (const Command[]){  // gets
                {"delay", "Gets the socket max delay"},
                {"drops", "Gets total drops count"},
                {"runs", "Gets total runs"}
            },
            2,
            1,
            3
        };

        return _manifesto;
    }


    uint8_t command_index(const MessageCode message_code, JsonObject json_message) {
        const char* command_name = json_message["n"].as<const char*>();
        const Command* command = nullptr;
        uint8_t count = 0;
        const Manifesto& my_manifesto = get_manifesto();
        switch (message_code)
        {
        case MessageCode::run:
            command = my_manifesto.runs;
            count = my_manifesto.runs_count;
            break;
        case MessageCode::set:
            command = my_manifesto.sets;
            count = my_manifesto.sets_count;
            break;
        case MessageCode::get:
            command = my_manifesto.gets;
            count = my_manifesto.gets_count;
            break;
        default: return 255;    // 255 means not found!
        }
        for (uint8_t i = 0; i < count; i++) {
            if (strcmp(command_name, command[i].name) == 0)
                return i;
        }
        return 255; // 255 means not found!
    }


    virtual bool command_run(const uint8_t command_index, JsonObject json_message) {
        (void)json_message; // Silence unused parameter warning
        switch (command_index)
        {
        case 0:
            {
                #ifdef JSON_TALKER_DEBUG
                Serial.println(F("Case 0 - Turning LED ON"));
                #endif
        
                if (!_is_led_on) {
                #ifdef LED_BUILTIN
                    #ifdef JSON_TALKER_DEBUG
                        Serial.print(F("LED_BUILTIN IS DEFINED as: "));
                        Serial.println(LED_BUILTIN);
                    #endif
                    digitalWrite(LED_BUILTIN, HIGH);
                #else
                    #ifdef JSON_TALKER_DEBUG
                        Serial.println(F("LED_BUILTIN IS NOT DEFINED in this context!"));
                    #endif
                #endif
                    _is_led_on = true;
                    _total_runs++;
                } else {
                    json_message["r"] = "Already On!";
                    if (_socket != nullptr)
                        this->sendMessage(json_message);
                    return false;
                }
                return true;
            }
            break;
        
        case 1:
            {
                #ifdef JSON_TALKER_DEBUG
                Serial.println(F("Case 1 - Turning LED OFF"));
                #endif
        
                if (_is_led_on) {
                #ifdef LED_BUILTIN
                    digitalWrite(LED_BUILTIN, LOW);
                #endif
                    _is_led_on = false;
                    _total_runs++;
                } else {
                    json_message["r"] = "Already Off!";
                    if (_socket != nullptr)
                        this->sendMessage(json_message);
                    return false;
                }
                return true;
            }
            break;
        
        default: return false;  // Nothing done
        }
    }

    
    virtual bool command_set(const uint8_t command_index, JsonObject json_message) {
        long json_value = json_message["v"].as<long>();
        switch (command_index)
        {
        case 0:
            {
                this->set_delay(static_cast<uint8_t>(json_value));
                return true;
            }
            break;
        
        default: return false;
        }
    }

    
    virtual long command_get(const uint8_t command_index, JsonObject json_message) {
        (void)json_message; // Silence unused parameter warning
        switch (command_index)
        {
        case 0:
            {
                return static_cast<long>(this->get_delay());
            }
            break;

        case 1:
            {
                return this->get_total_drops();
            }
            break;

        case 2:
            {
                return _total_runs;
            }
            break;
        
        default: return 0;  // Has to return something
        }
    }


    void set_delay(uint8_t delay);
    uint8_t get_delay();
    long get_total_drops();
    long get_total_runs() { return _total_runs; }


public:

    // Explicit default constructor
    JsonTalker() = delete;
        
    JsonTalker(const char* name, const char* desc)
        : _name(name), _desc(desc) {
            // Nothing to see here
        }

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


    
    const char* get_name() { return _name; }
    void set_channel(uint8_t channel) { _channel = channel; }
    uint8_t get_channel() { return _channel; }
    
    bool sendMessage(JsonObject message, bool as_reply = false);
    bool processData(const char* received_data, const size_t data_len, bool pre_validated = false);

};


#endif // JSON_TALKER_H
