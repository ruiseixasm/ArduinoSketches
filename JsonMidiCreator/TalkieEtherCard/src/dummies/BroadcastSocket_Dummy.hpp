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
#ifndef BROADCAST_SOCKET_DUMMY_HPP
#define BROADCAST_SOCKET_DUMMY_HPP

#include "../BroadcastSocket.hpp"
#include <ArduinoJson.h>


#define BROADCAST_DUMMY_DEBUG


class BroadcastSocket_Dummy : public BroadcastSocket {
private:
    static unsigned long _lastTime;

    // Helper function to safely create char* from buffer
    static char* decode(const uint8_t* data, const size_t length, char* talk) {
        memcpy(talk, data, length);
        talk[length] = '\0';
        return talk;
    }


    uint16_t getChecksum(const char* net_data, const size_t len) {
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


    uint16_t setChecksum(JsonObject message) {
        message["c"] = 0;   // makes _buffer a net_data buffer
        size_t len = serializeJson(message, _buffer, BROADCAST_SOCKET_BUFFER_SIZE);
        uint16_t checksum = getChecksum(_buffer, len);
        message["c"] = checksum;
        return checksum;
    }
    

public:
    // Singleton accessor
    static BroadcastSocket_Dummy& instance() {
        static BroadcastSocket_Dummy instance;
        return instance;
    }


    bool send(const char* data, size_t len, bool as_reply = false) override {
        #ifdef BROADCAST_DUMMY_DEBUG
        Serial.print(F("DUMMY SENT: "));
        char talk[len + 1];
        Serial.println(decode(data, len, talk));
        #endif
        return true;
    }

    
    size_t receive(char* buffer, size_t size) override {
        if (millis() - _lastTime > 1000) {
            _lastTime = millis();
            if (random(1000) < 100) { // 10% chance
                // 2. Message Selection
                // ALWAYS VALIDATE THE MESSAGES FOR BAD FORMATING !!
                const char* PROGMEM messages[] = {
                    R"({"m":0,"f":"Dummy","i":3003412860})",
                    R"({"m":2,"f":"Dummy","t":"Buzzer","n":"buzz","i":3003412861})",
                    R"({"m":2,"f":"Dummy","t":"Buzzer","n":"on","i":3003412862})",
                    R"({"m":2,"f":"Dummy","t":"Buzzer","n":"off","i":3003412863})",
                    R"({"m":6,"f":"Dummy","r":"Dummy echo","i":3003412864})",
                    R"({"m":6,"f":"Dummy","r":"Broadcasted echo","i":3003412865})",
                    R"({"m":6,"f":"Dummy","r":"Direct echo","i":3003412866})"
                };
                const size_t num_messages = sizeof(messages)/sizeof(char*);
                
                // 3. Safer Random Selection
                const char* message_char = messages[random(num_messages)];
                size_t message_size = strlen(message_char);
                
                // 5. JSON Handling with Memory Checks

                // JsonDocument in the stack makes sure its memory is released (NOT GLOBAL)
                #if ARDUINOJSON_VERSION_MAJOR >= 7
                JsonDocument message_doc;
                if (message_doc.overflowed()) {
                    Serial.println(F("Failed to allocate JSON message_doc"));
                    return;
                }
                #else
                DynamicJsonDocument message_doc(size);
                if (message_doc.capacity() == 0) {
                    Serial.println(F("Failed to allocate JSON message_doc"));
                    return;
                }
                #endif
                
                // Message needs to be '\0' terminated and thus buffer is used instead
                // it's possible to serialize from a JsonObject but it isn't to deserialize into a JsonObject!
                DeserializationError error = deserializeJson(message_doc, message_char, message_size);
                if (error) {
                    Serial.println(F("Failed to deserialize message"));
                    return;
                }
                JsonObject message = message_doc.as<JsonObject>();
                setChecksum(message);

                size_t message_len = serializeJson(message, buffer, size);
                if (message_len == 0 || message_len >= size) {
                    Serial.println(F("Serialization failed/buffer overflow"));
                    return;
                }

                #ifdef BROADCAST_DUMMY_DEBUG
                Serial.print("DUMMY RECEIVED: ");
                serializeJson(message, Serial);
                Serial.println();  // optional: just to add a newline after the JSON
                #endif

                return message_len;
            }
        }
        return 0;
    }
};

static unsigned long BroadcastSocket_Dummy::_lastTime = 0;

#endif // BROADCAST_SOCKET_DUMMY_HPP
