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


// #define JSON_TALKER_DEBUG

// Readjust if absolutely necessary
#define BROADCAST_SOCKET_BUFFER_SIZE 128


class BroadcastSocket;


class JsonTalker {
private:
    
    // Shared processing data buffer Not reentrant, received data unaffected
    static char _buffer[BROADCAST_SOCKET_BUFFER_SIZE];
    static BroadcastSocket* _socket;

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

    static uint16_t setChecksum(JsonObject json_message) {
        json_message["c"] = 0;   // makes _buffer a net_data buffer
        size_t len = serializeJson(json_message, _buffer, BROADCAST_SOCKET_BUFFER_SIZE);
        uint16_t checksum = getChecksum(_buffer, len);
        json_message["c"] = checksum;
        return checksum;
    }

    
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

    const char* _name;      // Name of the Talker
    const char* _desc;      // Description of the Device
    uint8_t _channel = 0;


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

    
    long _total_runs = 0;
    // static becaus it's a shared state among all other talkers, device (board) parameter
    static bool _is_led_on;  // keep track of state yourself, by default it's off


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

    const char* get_name() { return _name; }
    void set_channel(uint8_t channel) { _channel = channel; }
    uint8_t get_channel() { return _channel; }
    




    bool sendMessage(JsonObject json_message, bool as_reply = false);

    
    bool processData(JsonObject json_message, bool pre_validated) {

        #ifdef JSON_TALKER_DEBUG
        Serial.println(F("Processing..."));
        #endif
        
        // Error types:
        //     0 - Unknown sender   // Removed, useless kind of error report that results in UDP flooding
        //     1 - Message missing the checksum
        //     2 - Message corrupted
        //     3 - Wrong message code
        //     4 - Message NOT identified
        //     5 - Set command arrived too late

        if (!pre_validated) {

            if (!json_message["f"].is<String>()) {
                #ifdef JSON_TALKER_DEBUG
                Serial.println(0);
                #endif
                return false;
            }
            if (!json_message["i"].is<uint32_t>()) {
                #ifdef JSON_TALKER_DEBUG
                Serial.println(4);
                #endif
                json_message["m"] = 7;   // error
                json_message["t"] = json_message["f"];
                json_message["e"] = 4;
                
                sendMessage(json_message, true);
                return false;
            }
        }

        
        bool dont_interrupt = true;   // Doesn't interrupt next talkers process

        // Is it for me?
        if (json_message["t"].is<uint8_t>()) {
            if (json_message["t"].as<uint8_t>() != _channel)
                return true;    // It's still validated (just not for me as a target)
        } else if (json_message["t"].is<String>()) {
            if (json_message["t"] != _name) {
                #ifdef JSON_TALKER_DEBUG
                Serial.println(F("Message NOT for me!"));
                #endif
                return true;    // It's still validated (just not for me as a target)
            } else {
                dont_interrupt = false; // Found by name, interrupts next Talkers process
            }
        }   // else: If it has no "t" it means for every Talkers


        // Echo codes:
        //     0 - ROGER
        //     1 - UNKNOWN
        //     2 - NONE

        #ifdef JSON_TALKER_DEBUG
        Serial.print(F("Process: "));
        serializeJson(json_message, Serial);
        Serial.println();  // optional: just to add a newline after the JSON
        #endif


        MessageCode message_code = static_cast<MessageCode>(json_message["m"].as<int>());
        json_message["w"] = json_message["m"].as<int>();
        json_message["t"] = json_message["f"];
        json_message["m"] = static_cast<int>(MessageCode::echo);

        switch (message_code)
        {
        case MessageCode::talk:
            json_message["d"] = _desc;
            sendMessage(json_message, true);
            break;
        
        case MessageCode::list:
            {   // Because of none_list !!!
                bool none_list = true;

                // In your list handler:
                
                #ifdef JSON_TALKER_DEBUG
                Serial.print("=== This object is: ");
                Serial.println(class_name());
                #endif
            
                const Manifesto& my_manifesto = get_manifesto();

                json_message["w"] = static_cast<int>(MessageCode::run);
                for (size_t i = 0; i < my_manifesto.runs_count; ++i) {
                    none_list = false;
                    json_message["n"] = my_manifesto.runs[i].name;
                    json_message["d"] = my_manifesto.runs[i].desc;
                    sendMessage(json_message, true);
                }
                json_message["w"] = static_cast<int>(MessageCode::set);
                for (size_t i = 0; i < my_manifesto.sets_count; ++i) {
                    none_list = false;
                    json_message["n"] = my_manifesto.sets[i].name;
                    json_message["d"] = my_manifesto.sets[i].desc;
                    sendMessage(json_message, true);
                }
                json_message["w"] = static_cast<int>(MessageCode::get);
                for (size_t i = 0; i < my_manifesto.gets_count; ++i) {
                    none_list = false;
                    json_message["n"] = my_manifesto.gets[i].name;
                    json_message["d"] = my_manifesto.gets[i].desc;
                    sendMessage(json_message, true);
                }
                if(none_list) {
                    json_message["g"] = 2;       // NONE
                }
            }
            break;
        
        case MessageCode::run:
            if (json_message["n"].is<String>()) {

                const uint8_t command_found_i = command_index(MessageCode::run, json_message);
                if (command_found_i < 255) {

                    #ifdef JSON_TALKER_DEBUG
                    Serial.println(F("RUN found, now being processed..."));
                    #endif
            
                    json_message["g"] = 0;       // ROGER
                    sendMessage(json_message, true);
                    // No memory leaks because message_doc exists in the listen() method stack
                    json_message.remove("g");
                    command_run(command_found_i, json_message);
                } else {
                    json_message["g"] = 1;   // UNKNOWN
                    sendMessage(json_message, true);
                }
            }
            break;
        
        case MessageCode::set:
            if (json_message["n"].is<String>() && json_message["v"].is<long>()) {

                const uint8_t command_found_i = command_index(MessageCode::set, json_message);
                if (command_found_i < 255) {
                    json_message["g"] = 0;       // ROGER
                    sendMessage(json_message, true);
                    // No memory leaks because message_doc exists in the listen() method stack
                    json_message.remove("g");
                    command_set(command_found_i, json_message);
                } else {
                    json_message["g"] = 1;   // UNKNOWN
                    sendMessage(json_message, true);
                }
            }
            break;
        
        case MessageCode::get:
            if (json_message["n"].is<String>()) {

                const uint8_t command_found_i = command_index(MessageCode::get, json_message);
                if (command_found_i < 255) {
                    json_message["g"] = 0;       // ROGER
                    sendMessage(json_message, true);
                    // No memory leaks because message_doc exists in the listen() method stack
                    json_message.remove("g");
                    json_message["v"] = command_get(command_found_i, json_message);
                    sendMessage(json_message, true);
                } else {
                    json_message["g"] = 1;   // UNKNOWN
                    sendMessage(json_message, true);
                }
            }
            break;
        
        case MessageCode::sys:
            {
            // AVR Boards (Uno, Nano, Mega) - Check RAM size
            #ifdef __AVR__
                uint16_t ramSize = RAMEND - RAMSTART + 1;
                if (ramSize == 2048)
                    json_message["d"] = F("Arduino Uno/Nano (ATmega328P)");
                else if (ramSize == 8192)
                    json_message["d"] = F("Arduino Mega (ATmega2560)");
                else
                    json_message["d"] = F("Unknown AVR Board");
                
            // ESP8266
            #elif defined(ESP8266)
                json_message["d"] = "ESP8266 (Chip ID: " + String(ESP.getChipId()) + ")";
                
            // ESP32
            #elif defined(ESP32)
                json_message["d"] = "ESP32 (Rev: " + String(ESP.getChipRevision()) + ")";
                
            // Teensy Boards
            #elif defined(TEENSYDUINO)
                #if defined(__IMXRT1062__)
                    json_message["d"] = F("Teensy 4.0/4.1 (i.MX RT1062)");
                #elif defined(__MK66FX1M0__)
                    json_message["d"] = F("Teensy 3.6 (MK66FX1M0)");
                #elif defined(__MK64FX512__)
                    json_message["d"] = F("Teensy 3.5 (MK64FX512)");
                #elif defined(__MK20DX256__)
                    json_message["d"] = F("Teensy 3.2/3.1 (MK20DX256)");
                #elif defined(__MKL26Z64__)
                    json_message["d"] = F("Teensy LC (MKL26Z64)");
                #else
                    json_message["d"] = F("Unknown Teensy Board");
                #endif

            // ARM (Due, Zero, etc.)
            #elif defined(__arm__)
                json_message["d"] = F("ARM-based Board");

            // Unknown Board
            #else
                json_message["d"] = F("Unknown Board");

            #endif

                sendMessage(json_message, true);

                // TO INSERT HERE EXTRA DATA !!
            }
            break;
        
        case MessageCode::echo:
            this->echo(json_message);
            break;
        
        case MessageCode::error:
            this->error(json_message);
            break;
        
        case MessageCode::channel:
            if (json_message["b"].is<uint8_t>()) {

                #ifdef JSON_TALKER_DEBUG
                Serial.print(F("Channel B value is an <uint8_t>: "));
                Serial.println(json_message["b"].is<uint8_t>());
                #endif

                _channel = json_message["b"].as<uint8_t>();
            }
            json_message["b"] = _channel;
            sendMessage(json_message, true);
            break;
        
        default:
            break;
        }

        return dont_interrupt;
    }

};


#endif // JSON_TALKER_H
