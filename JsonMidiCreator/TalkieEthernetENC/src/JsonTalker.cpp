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

#include "BroadcastSocket.h"  // MUST include the full definition!
#include "JsonTalker.h"    // Includes the ArduinoJson Library


char JsonTalker::_buffer[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};

bool JsonTalker::_is_led_on = false;


void JsonTalker::set_delay(uint8_t delay) {
    return _socket->set_max_delay(delay);
}

uint8_t JsonTalker::get_delay() {
    return _socket->get_max_delay();
}

long JsonTalker::get_total_drops() {
    return _socket->get_drops_count();
}




uint16_t JsonTalker::setChecksum(JsonObject message) {
    message["c"] = 0;   // makes _buffer a net_data buffer
    size_t len = serializeJson(message, _buffer, BROADCAST_SOCKET_BUFFER_SIZE);
    uint16_t checksum = BroadcastSocket::getChecksum(_buffer, len);
    message["c"] = checksum;
    return checksum;
}


bool JsonTalker::sendMessage(JsonObject message, bool as_reply) {
    if (_socket == nullptr) return false;
    
    // Directly nest the editable message under "m"
    if (message.isNull()) {
        #ifdef JSON_TALKER_DEBUG
        Serial.println(F("Error: Null message received"));
        #endif
        return false;
    }

    // Set default 'id' field if missing
    if (!message["i"].is<uint32_t>()) {
        message["i"] = generateMessageId();
    }

    message["f"] = _name;

    JsonTalker::setChecksum(message);

    size_t len = serializeJson(message, _buffer, BROADCAST_SOCKET_BUFFER_SIZE);
    if (len == 0) {
        #ifdef JSON_TALKER_DEBUG
        Serial.println(F("Error: Serialization failed"));
        #endif
    } else {
        
        #ifdef JSON_TALKER_DEBUG
        Serial.print(F("T: "));
        serializeJson(message, Serial);
        Serial.println();  // optional: just to add a newline after the JSON
        #endif

        return _socket->send(_buffer, len, as_reply);
    }
    return false;
}


    
bool JsonTalker::processData(const char* received_data, const size_t data_len, bool pre_validated) {

    if (_socket == nullptr) return false;
    
    #ifdef JSON_TALKER_DEBUG
    Serial.println(F("Validating..."));
    #endif
    
    // JsonDocument in the stack makes sure its memory is released (NOT GLOBAL)
    #if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument message_doc;
    #else
    StaticJsonDocument<BROADCAST_SOCKET_BUFFER_SIZE> message_doc;
    #endif

    DeserializationError error = deserializeJson(message_doc, received_data, data_len);
    if (error) {
        #ifdef JSON_TALKER_DEBUG
        Serial.println(F("Failed to deserialize received data"));
        #endif
        return false;
    }
    JsonObject message = message_doc.as<JsonObject>();

    // Error types:
    //     0 - Unknown sender   // Removed, useless kind of error report that results in UDP flooding
    //     1 - Message missing the checksum
    //     2 - Message corrupted
    //     3 - Wrong message code
    //     4 - Message NOT identified
    //     5 - Set command arrived too late

    if (!pre_validated) {

        if (!message["m"].is<int>()) {
            #ifdef JSON_TALKER_DEBUG
            Serial.println(F("Message \"m\" is NOT an integer!"));
            #endif
            return false;
        }
        if (!message["f"].is<String>()) {
            #ifdef JSON_TALKER_DEBUG
            Serial.println(0);
            #endif
            return false;
        }
        if (!message["i"].is<uint32_t>()) {
            #ifdef JSON_TALKER_DEBUG
            Serial.println(4);
            #endif
            message["m"] = 7;   // error
            message["t"] = message["f"];
            message["e"] = 4;
            
            sendMessage(message, true);
            return false;
        }
    }

    
    bool dont_interrupt = true;   // Doesn't interrupt next talkers process

    // Is it for me?
    if (message["t"].is<uint8_t>()) {
        if (message["t"].as<uint8_t>() != _channel)
            return true;    // It's still validated (just not for me as a target)
    } else if (message["t"].is<String>()) {
        if (message["t"] != _name) {
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
    serializeJson(message, Serial);
    Serial.println();  // optional: just to add a newline after the JSON
    #endif


    MessageCode message_code = static_cast<MessageCode>(message["m"].as<int>());
    message["w"] = message["m"].as<int>();
    message["t"] = message["f"];
    message["m"] = 6;   // echo

    switch (message_code)
    {
    case MessageCode::talk:
        message["d"] = _desc;
        sendMessage(message, true);
        break;
    
    case MessageCode::list:
        {   // Because of none_list !!!
            bool none_list = true;
            message["w"] = static_cast<int>(MessageCode::run);
            for (size_t i = 0; i < this->runs_count(); ++i) {
                none_list = false;
                message["n"] = this->_runCommands[i].name;
                message["d"] = this->_runCommands[i].desc;
                sendMessage(message, true);
            }
            message["w"] = static_cast<int>(MessageCode::set);
            const Set* commands = get_set_commands();
            size_t count = get_set_commands_count();
            for (size_t i = 0; i < count; ++i) {
                none_list = false;
                message["n"] = commands[i].name;
                message["d"] = commands[i].desc;
                sendMessage(message, true);
            }
            message["w"] = static_cast<int>(MessageCode::get);
            for (size_t i = 0; i < this->gets_count(); ++i) {
                none_list = false;
                message["n"] = this->_getCommands[i].name;
                message["d"] = this->_getCommands[i].desc;
                sendMessage(message, true);
            }
            if(none_list) {
                message["g"] = 2;       // NONE
            }
        }
        break;
    
    case MessageCode::run:
        if (message["n"].is<String>()) {

            const JsonTalker::Run* run = this->run(message["n"]);
            if (run == nullptr) {
                message["g"] = 1;   // UNKNOWN
                sendMessage(message, true);
            } else {
                message["g"] = 0;       // ROGER
                sendMessage(message, true);
                // No memory leaks because message_doc exists in the listen() method stack
                message.remove("g");
                (this->*(run->method))(message);
            }
        }
        break;
    
    case MessageCode::set:
        if (message["n"].is<String>() && message["v"].is<long>()) {

            const JsonTalker::Set* set = this->set(message["n"]);
            if (set == nullptr) {
                message["g"] = 1;   // UNKNOWN
                sendMessage(message, true);
            } else {
                message["g"] = 0;       // ROGER
                sendMessage(message, true);
                // No memory leaks because message_doc exists in the listen() method stack
                message.remove("g");
                (this->*(set->method))(message, message["v"].as<long>());
            }
        }
        break;
    
    case MessageCode::get:
        if (message["n"].is<String>()) {
            const JsonTalker::Get* get = this->get(message["n"]);
            if (get == nullptr) {
                message["g"] = 1;   // UNKNOWN
                sendMessage(message, true);
            } else {
                message["g"] = 0;       // ROGER
                sendMessage(message, true);
                // No memory leaks because message_doc exists in the listen() method stack
                message.remove("g");
                message["v"] = (this->*(get->method))(message);
                sendMessage(message, true);
            }
        }
        break;
    
    case MessageCode::sys:
        {
        // AVR Boards (Uno, Nano, Mega) - Check RAM size
        #ifdef __AVR__
            uint16_t ramSize = RAMEND - RAMSTART + 1;
            if (ramSize == 2048)
                message["d"] = F("Arduino Uno/Nano (ATmega328P)");
            else if (ramSize == 8192)
                message["d"] = F("Arduino Mega (ATmega2560)");
            else
                message["d"] = F("Unknown AVR Board");
            
        // ESP8266
        #elif defined(ESP8266)
            message["d"] = "ESP8266 (Chip ID: " + String(ESP.getChipId()) + ")";
            
        // ESP32
        #elif defined(ESP32)
            message["d"] = "ESP32 (Rev: " + String(ESP.getChipRevision()) + ")";
            
        // Teensy Boards
        #elif defined(TEENSYDUINO)
            #if defined(__IMXRT1062__)
                message["d"] = F("Teensy 4.0/4.1 (i.MX RT1062)");
            #elif defined(__MK66FX1M0__)
                message["d"] = F("Teensy 3.6 (MK66FX1M0)");
            #elif defined(__MK64FX512__)
                message["d"] = F("Teensy 3.5 (MK64FX512)");
            #elif defined(__MK20DX256__)
                message["d"] = F("Teensy 3.2/3.1 (MK20DX256)");
            #elif defined(__MKL26Z64__)
                message["d"] = F("Teensy LC (MKL26Z64)");
            #else
                message["d"] = F("Unknown Teensy Board");
            #endif

        // ARM (Due, Zero, etc.)
        #elif defined(__arm__)
            message["d"] = F("ARM-based Board");

        // Unknown Board
        #else
            message["d"] = F("Unknown Board");

        #endif

            sendMessage(message, true);

            // TO INSERT HERE EXTRA DATA !!
        }
        break;
    
    case MessageCode::echo:
        this->echo(message);
        break;
    
    case MessageCode::error:
        this->error(message);
        break;
    
    case MessageCode::channel:
        if (message["b"].is<uint8_t>()) {

            #ifdef JSON_TALKER_DEBUG
            Serial.print(F("Channel B value is an <uint8_t>: "));
            Serial.println(message["b"].is<uint8_t>());
            #endif

            _channel = message["b"].as<uint8_t>();
        } else {
            return false;
        }
        message["b"] = _channel;
        sendMessage(message, true);
        break;
    
    default:
        break;
    }

    return dont_interrupt;
}



