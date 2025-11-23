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
#include "JsonTalker.h"    // Include ArduinoJson Library


char JsonTalker::_buffer[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};



uint16_t JsonTalker::setChecksum(JsonObject message) {
    message["c"] = 0;   // makes _buffer a net_data buffer
    size_t len = serializeJson(message, _buffer, BROADCAST_SOCKET_BUFFER_SIZE);
    uint16_t checksum = BroadcastSocket::getChecksum(_buffer, len);
    message["c"] = checksum;
    return checksum;
}
    

bool JsonTalker::sendMessage(BroadcastSocket* socket, JsonObject message, bool as_reply) {
    if (socket == nullptr) return false;
    _socket = socket;
    
    // Directly nest the editable message under "m"
    if (message.isNull()) {
        #ifdef DEVICE_TALKER_DEBUG
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
        #ifdef DEVICE_TALKER_DEBUG
        Serial.println(F("Error: Serialization failed"));
        #endif
    } else {
        
        #ifdef DEVICE_TALKER_DEBUG
        Serial.print(F("T: "));
        serializeJson(message, Serial);
        Serial.println();  // optional: just to add a newline after the JSON
        #endif

        return socket->send(_buffer, len, as_reply);
    }
    return false;
}


    
void JsonTalker::processData(BroadcastSocket* socket, const char* received_data, const size_t data_len) {
    
    if (socket != nullptr) _socket = socket;
    
    #ifdef DEVICE_TALKER_DEBUG
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
        #ifdef DEVICE_TALKER_DEBUG
        Serial.println(F("Failed to deserialize received data"));
        #endif
        return;
    }
    JsonObject message = message_doc.as<JsonObject>();

    // Error types:
    //     0 - Unknown sender   // Removed, useless kind of error report that results in UDP flooding
    //     1 - Message missing the checksum
    //     2 - Message corrupted
    //     3 - Wrong message code
    //     4 - Message NOT identified
    //     5 - Set command arrived too late

    if (!message["m"].is<int>()) {
        #ifdef DEVICE_TALKER_DEBUG
        Serial.println(F("Message \"m\" is NOT an integer!"));
        #endif
        return;
    }
    if (!message["f"].is<String>()) {
        #ifdef DEVICE_TALKER_DEBUG
        Serial.println(0);
        #endif
        return;
    }
    if (!message["i"].is<uint32_t>()) {
        #ifdef DEVICE_TALKER_DEBUG
        Serial.println(4);
        #endif
        message["m"] = 7;   // error
        message["t"] = message["f"];
        message["e"] = 4;
        
        sendMessage(socket, message, true);
        return;
    }

    // In theory, a UDP packet on a local area network (LAN) could survive
    // for about 4.25 minutes (255 seconds).
    if (_check_set_time && millis() - _sent_set_time[1] > 255000UL) {
        _check_set_time = false;
    }



    if (true) {

        #ifdef DEVICE_TALKER_DEBUG
        Serial.print(F("Listened: "));
        serializeJson(message, Serial);
        Serial.println();  // optional: just to add a newline after the JSON
        #endif

        // Only set messages are time checked
        if (message["m"].as<int>() == static_cast<int>(MessageCode::set)) {  // 3 - set
            _sent_set_time[0] = message["i"].as<uint32_t>();
            _sent_set_time[1] = generateMessageId();
            _set_name = message["f"].as<String>(); // Explicit conversion
            _check_set_time = true;
        }
    }


    // Echo codes:
    //     0 - ROGER
    //     1 - UNKNOWN
    //     2 - NONE

    #ifdef DEVICE_TALKER_DEBUG
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
        sendMessage(socket, message, true);
        break;
    
    case MessageCode::list:
        {   // Because of none_list !!!
            bool none_list = true;
            message["w"] = static_cast<int>(MessageCode::run);
            for (size_t run_i = 0; run_i < this->runs_count(); ++run_i) {
                none_list = false;
                message["n"] = this->runCommands[run_i].name;
                message["d"] = this->runCommands[run_i].desc;
                sendMessage(socket, message, true);
            }
            message["w"] = static_cast<int>(MessageCode::set);
            for (size_t set_i = 0; set_i < this->sets_count(); ++set_i) {
                none_list = false;
                message["n"] = this->setCommands[set_i].name;
                message["d"] = this->setCommands[set_i].desc;
                sendMessage(socket, message, true);
            }
            message["w"] = static_cast<int>(MessageCode::get);
            for (size_t get_i = 0; get_i < this->gets_count(); ++get_i) {
                none_list = false;
                message["n"] = this->getCommands[get_i].name;
                message["d"] = this->getCommands[get_i].desc;
                sendMessage(socket, message, true);
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
                sendMessage(socket, message, true);
            } else {
                message["g"] = 0;       // ROGER
                sendMessage(socket, message, true);
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
                sendMessage(socket, message, true);
            } else {
                message["g"] = 0;       // ROGER
                sendMessage(socket, message, true);
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
                sendMessage(socket, message, true);
            } else {
                message["g"] = 0;       // ROGER
                sendMessage(socket, message, true);
                // No memory leaks because message_doc exists in the listen() method stack
                message.remove("g");
                message["v"] = (this->*(get->method))(message);
                sendMessage(socket, message, true);
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

            sendMessage(socket, message, true);
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

            #ifdef DEVICE_TALKER_DEBUG
            Serial.print(F("Channel B value is an <uint8_t>: "));
            Serial.println(message["b"].is<uint8_t>());
            #endif

            _channel = message["b"].as<uint8_t>();
        }
        message["b"] = _channel;
        sendMessage(socket, message, true);
        break;
    
    default:
        break;
    }









}



