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

#include <Arduino.h>        // Needed for Serial given that Arduino IDE only includes Serial in .ino files!
#include <ArduinoJson.h>    // Include ArduinoJson Library


// #define JSON_TALKER_DEBUG

// Readjust if absolutely necessary
#define BROADCAST_SOCKET_BUFFER_SIZE 128


class BroadcastSocket;


class JsonTalker {
protected:
    
    // The socket can't be static becaus different talkers may use different sockets (remote)
    BroadcastSocket* _socket = nullptr;
    // Pointer PRESERVE the polymorphism while objects don't!
    static JsonTalker** _json_talkers;  // It's capable of communicate with other talkers (local)
    static uint8_t _talker_count;

public:

    virtual const char* class_name() const { return "JsonTalker"; }

    enum MessageCode : int {
        TALK,
        LIST,
        RUN,
        SET,
        GET,
        SYS,
        ECHO,
        ERROR,
        CHANNEL
    };

    
    // Now without a method reference `bool (JsonTalker::*method)(JsonObject, uint32_t)`
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
    bool _muted = false;


    // Can't use method reference because these type of references are class designation dependent,
    // so, they will prevent any possibility of inheritance, this way the only alternative is to
    // do an indirect reference based on the command position with only a pair of strings and
    // a following switch case sequence that picks the respective method and calls it directly.

    // Virtual method that returns static manifesto (can't override class member variables)
    virtual const Manifesto& get_manifesto() const {

        static const Manifesto _manifesto = {
            (const Command[]){  // runs
                {"mute", "Mutes this talker"},
                {"unmute", "Unmutes this talker"},
                {"on", "Turns led ON"},
                {"off", "Turns led OFF"}
            },
            (const Command[]){  // sets 
                {"delay", "Sets the socket max delay"}
            },
            (const Command[]){  // gets
                {"muted", "Returns 1 if muted and 0 if not"},
                {"delay", "Gets the socket max delay"},
                {"drops", "Gets total drops count"},
                {"runs", "Gets total runs"}
            },
            4,
            1,
            4
        };

        return _manifesto;
    }


    bool remoteSend(JsonObject json_message, bool as_reply = false, uint8_t target_index = 255);


    bool localSend(JsonObject json_message, bool as_reply = false, uint8_t target_index = 255) {
        (void)as_reply; 	// Silence unused parameter warning
        (void)target_index; // Silence unused parameter warning

        json_message["f"] = _name;
        json_message["c"] = 1;  // 'c' = 1 means LOCAL communication
        // Triggers all local Talkers to processes the json_message
        bool sent_message = false;
		if (target_index < _talker_count) {
			if (_json_talkers[target_index] != this) {  // Can't send to myself
				_json_talkers[target_index]->processData(json_message);
				sent_message = true;
			}
		} else {
			bool pre_validated = false;
			for (uint8_t talker_i = 0; talker_i < _talker_count; ++talker_i) {
				if (_json_talkers[talker_i] != this) {  // Can't send to myself
					pre_validated = _json_talkers[talker_i]->processData(json_message, pre_validated);
					sent_message = true;
					if (!pre_validated) break;
				}
			}
		}
        return sent_message;
    }


    bool replyMessage(JsonObject json_message, bool as_reply = true) {
        if (json_message["c"].is<uint16_t>()) {
            uint16_t c = json_message["c"].as<uint16_t>();
            if (c == 1) {   // c == 1 means a local message while 0 means a remote one
                return localSend(json_message, as_reply);
            }
        }
        return remoteSend(json_message, as_reply);
    }

    
    uint16_t _total_runs = 0;
    // static becaus it's a shared state among all other talkers, device (board) parameter
    static bool _is_led_on;  // keep track of state yourself, by default it's off


    uint8_t command_index(const MessageCode message_code, JsonObject json_message) {
        const char* command_name = json_message["n"].as<const char*>();
        const Command* command = nullptr;
        uint8_t count = 0;
        const Manifesto& my_manifesto = get_manifesto();
        switch (message_code)
        {
        case MessageCode::RUN:
            command = my_manifesto.runs;
            count = my_manifesto.runs_count;
            break;
        case MessageCode::SET:
            command = my_manifesto.sets;
            count = my_manifesto.sets_count;
            break;
        case MessageCode::GET:
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
            _muted = true;
            break;
        case 1:
            _muted = false;
            break;

        case 2:
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
                    if (_socket)
                        this->replyMessage(json_message, false);
                    return false;
                }
                return true;
            }
            break;
        
        case 3:
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
                    if (_socket)
                        this->replyMessage(json_message, false);
                    return false;
                }
                return true;
            }
            break;
        }
		return false;  // Nothing done
    }

    
    virtual bool command_set(const uint8_t command_index, JsonObject json_message) {
        uint32_t json_value = json_message["v"].as<uint32_t>();
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

    
    virtual uint32_t command_get(const uint8_t command_index, JsonObject json_message) {
        (void)json_message; // Silence unused parameter warning
        switch (command_index)
        {
        case 0:
            return _muted;
            break;
        case 1:
            {
                return static_cast<uint32_t>(this->get_delay());
            }
            break;

        case 2:
            {
                return this->get_total_drops();
            }
            break;

        case 3:
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
    uint16_t get_total_drops();
    uint16_t get_total_runs() { return _total_runs; }


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

    static void connectTalkers(JsonTalker** json_talkers, uint8_t talker_count) {
        _json_talkers = json_talkers;
        _talker_count = talker_count;
    }


    const char* get_name() { return _name; }
    void set_channel(uint8_t channel) { _channel = channel; }
    uint8_t get_channel() { return _channel; }
    
    JsonTalker& mute() {    // It does NOT make a copy!
        _muted = true;
        return *this;
    }

    JsonTalker& unmute() {
        _muted = false;
        return *this;
    }

    bool muted() { return _muted; }

    
    bool processData(JsonObject json_message, bool pre_validated = false) {

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
                
                replyMessage(json_message, true);
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
        json_message["m"] = MessageCode::ECHO;

        switch (message_code)
        {
        case MessageCode::TALK:
            json_message["d"] = _desc;
            replyMessage(json_message, true);
            break;
        
        case MessageCode::LIST:
            {   // Because of none_list !!!
                bool none_list = true;

                // In your list handler:
                
                #ifdef JSON_TALKER_DEBUG
                Serial.print("=== This object is: ");
                Serial.println(class_name());
                #endif
            
                const Manifesto& my_manifesto = get_manifesto();

                json_message["w"] = MessageCode::RUN;
                for (size_t i = 0; i < my_manifesto.runs_count; ++i) {
                    none_list = false;
                    json_message["n"] = my_manifesto.runs[i].name;
                    json_message["d"] = my_manifesto.runs[i].desc;
                    replyMessage(json_message, true);
                }
                json_message["w"] = MessageCode::SET;
                for (size_t i = 0; i < my_manifesto.sets_count; ++i) {
                    none_list = false;
                    json_message["n"] = my_manifesto.sets[i].name;
                    json_message["d"] = my_manifesto.sets[i].desc;
                    replyMessage(json_message, true);
                }
                json_message["w"] = MessageCode::GET;
                for (size_t i = 0; i < my_manifesto.gets_count; ++i) {
                    none_list = false;
                    json_message["n"] = my_manifesto.gets[i].name;
                    json_message["d"] = my_manifesto.gets[i].desc;
                    replyMessage(json_message, true);
                }
                if(none_list) {
                    json_message["g"] = 2;       // NONE
                }
            }
            break;
        
        case MessageCode::RUN:
            if (json_message["n"].is<String>()) {

                const uint8_t command_found_i = command_index(MessageCode::RUN, json_message);
                if (command_found_i < 255) {

                    #ifdef JSON_TALKER_DEBUG
                    Serial.println(F("RUN found, now being processed..."));
                    #endif
            
                    json_message["g"] = 0;       // ROGER
                    replyMessage(json_message, true);
                    // No memory leaks because message_doc exists in the listen() method stack
                    json_message.remove("g");
                    command_run(command_found_i, json_message);
                } else {
                    json_message["g"] = 1;   // UNKNOWN
                    replyMessage(json_message, true);
                }
            }
            break;
        
        case MessageCode::SET:
            if (json_message["n"].is<String>() && json_message["v"].is<uint32_t>()) {

                const uint8_t command_found_i = command_index(MessageCode::SET, json_message);
                if (command_found_i < 255) {
                    json_message["g"] = 0;       // ROGER
                    replyMessage(json_message, true);
                    // No memory leaks because message_doc exists in the listen() method stack
                    json_message.remove("g");
                    command_set(command_found_i, json_message);
                } else {
                    json_message["g"] = 1;   // UNKNOWN
                    replyMessage(json_message, true);
                }
            }
            break;
        
        case MessageCode::GET:
            if (json_message["n"].is<String>()) {

                const uint8_t command_found_i = command_index(MessageCode::GET, json_message);
                if (command_found_i < 255) {
                    // No memory leaks because message_doc exists in the listen() method stack
                    // The return of the value works as an implicit ROGER (avoids network flooding)
                    json_message["v"] = command_get(command_found_i, json_message);
                    replyMessage(json_message, true);
                } else {
                    json_message["g"] = 1;   // UNKNOWN
                    replyMessage(json_message, true);
                }
            }
            break;
        
        case MessageCode::SYS:
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

                replyMessage(json_message, true);

                // TO INSERT HERE EXTRA DATA !!
            }
            break;
        
        case MessageCode::ECHO:
            this->echo(json_message);
            break;
        
        case MessageCode::ERROR:
            this->error(json_message);
            break;
        
        case MessageCode::CHANNEL:
            if (json_message["b"].is<uint8_t>()) {

                #ifdef JSON_TALKER_DEBUG
                Serial.print(F("Channel B value is an <uint8_t>: "));
                Serial.println(json_message["b"].is<uint8_t>());
                #endif

                _channel = json_message["b"].as<uint8_t>();
            }
            json_message["b"] = _channel;
            replyMessage(json_message, true);
            break;
        
        default:
            break;
        }

        return dont_interrupt;
    }

};


#endif // JSON_TALKER_H
