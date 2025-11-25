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
#ifndef MULTI_PLAYER_HPP
#define MULTI_PLAYER_HPP

#include "JsonTalker.h"         // Includes the ArduinoJson Library

#define MULTI_PLAYER_DEBUG


class MultiPlayer : public JsonTalker {
public:

    MultiPlayer(const char* name, const char* desc)
        : JsonTalker(name, desc) {}


protected:

    // Can't use method reference because these type of references are class designation dependent,
    // so, they will prevent any possibility of inheritance, this way the only alternative is to
    // do an indirect reference based on the command position with only a pair of strings and
    // a following switch case sequence that picks the respective method and calls it directly.

    // const Manifesto _manifesto = {
    //     (const Command[]){  // runs
    //         {"on", "Turns led ON"},
    //         {"off", "Turns led OFF"}
    //     },
    //     (const Command[]){  // sets 
    //         {"delay", "Sets the socket max delay"},
    //         {"bpm_10", "Sets the Tempo in BPM x 10"}
    //     },
    //     (const Command[]){  // gets
    //         {"delay", "Gets the socket max delay"},
    //         {"drops", "Gets total drops count"},
    //         {"runs", "Gets total runs"}
    //     },
    //     2,
    //     2,
    //     3
    // };

    long _bpm_10 = 1200;

    // virtual bool command_run(const uint8_t command_index, JsonObject json_message) {
    //     (void)json_message; // Silence unused parameter warning
    //     switch (command_index)
    //     {
    //     case 0:
    //         {
    //             #ifdef MULTI_PLAYER_DEBUG
    //             Serial.println(F("Case 0 - Turning LED ON"));
    //             #endif
        
    //             if (!_is_led_on) {
    //             #ifdef LED_BUILTIN
    //                 #ifdef MULTI_PLAYER_DEBUG
    //                     Serial.print(F("LED_BUILTIN IS DEFINED as: "));
    //                     Serial.println(LED_BUILTIN);
    //                 #endif
    //                 digitalWrite(LED_BUILTIN, HIGH);
    //             #else
    //                 #ifdef MULTI_PLAYER_DEBUG
    //                     Serial.println(F("LED_BUILTIN IS NOT DEFINED in this context!"));
    //                 #endif
    //             #endif
    //                 _is_led_on = true;
    //                 _total_runs++;
    //             } else {
    //                 json_message["r"] = "Already On!";
    //                 if (_socket != nullptr)
    //                     this->sendMessage(json_message);
    //                 return false;
    //             }
    //             return true;
    //         }
    //         break;
        
    //     case 1:
    //         {
    //             #ifdef MULTI_PLAYER_DEBUG
    //             Serial.println(F("Case 1 - Turning LED OFF"));
    //             #endif
        
    //             if (_is_led_on) {
    //             #ifdef LED_BUILTIN
    //                 digitalWrite(LED_BUILTIN, LOW);
    //             #endif
    //                 _is_led_on = false;
    //                 _total_runs++;
    //             } else {
    //                 json_message["r"] = "Already Off!";
    //                 if (_socket != nullptr)
    //                     this->sendMessage(json_message);
    //                 return false;
    //             }
    //             return true;
    //         }
    //         break;
        
    //     default: return false;  // Nothing done
    //     }
    // }

    
    // bool command_set(const uint8_t command_index, JsonObject json_message) {
    //     long json_value = json_message["v"].as<long>();
    //     switch (command_index)
    //     {
    //     case 0:
    //         {
    //             return JsonTalker::command_set(command_index, json_message);
    //         }
    //         break;
            
    //     case 1:
    //         {
    //             _bpm_10 = json_value;
    //             return true;
    //         }
    //         break;
        
    //     default: return false;
    //     }
    // }

    
    // virtual long command_get(const uint8_t command_index, JsonObject json_message) {
    //     (void)json_message; // Silence unused parameter warning
    //     switch (command_index)
    //     {
    //     case 0:
    //         {
    //             return static_cast<long>(this->get_delay());
    //         }
    //         break;

    //     case 1:
    //         {
    //             return this->get_total_drops();
    //         }
    //         break;

    //     case 2:
    //         {
    //             return _total_runs;
    //         }
    //         break;
        
    //     default: return 0;  // Has to return something
    //     }
    // }


};


#endif // MULTI_PLAYER_HPP
