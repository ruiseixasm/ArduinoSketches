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
#ifndef BROADCAST_SOCKET_H
#define BROADCAST_SOCKET_H

#include <Arduino.h>    // Needed for Serial given that Arduino IDE only includes Serial in .ino files!
#include "JsonTalker.h"


#define BROADCASTSOCKET_DEBUG

// Readjust if absolutely necessary
#define BROADCAST_SOCKET_BUFFER_SIZE 128
#define MAX_NETWORK_PACKET_LIFETIME_MS 256UL    // 256 milliseconds

class BroadcastSocket {
private:


    // Pointer PRESERVE the polymorphism while objects don't!
    JsonTalker** _json_talkers = nullptr;   // It's a singleton, so, no need to be static
    uint8_t _talker_count = 0;
    bool _control_timing = false;
    uint32_t _last_local_time = 0;
    uint32_t _last_remote_time = 0;
    uint16_t _drops_count = 0;


    static uint16_t generateChecksum(const char* net_data, const size_t len) {
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


    // ASCII byte values:
    // 	'c' = 99
    // 	':' = 58
    // 	'"' = 34
    // 	'0' = 48
    // 	'9' = 57


    static uint16_t extractChecksum(size_t* source_len, int* message_code_int, uint32_t* remote_time) {
        
        uint16_t data_checksum = 0;
        // Has to be pre processed (linearly)
        bool at_m = false;
        bool at_c = false;
        bool at_i = false;
        size_t data_i = 3;
        for (size_t i = data_i; i < *source_len; ++i) {
            if (_receiving_buffer[i] == ':') {
                if (_receiving_buffer[i - 2] == 'c' && _receiving_buffer[i - 3] == '"' && _receiving_buffer[i - 1] == '"') {
                    at_c = true;
                } else if (_receiving_buffer[i - 2] == 'i' && _receiving_buffer[i - 3] == '"' && _receiving_buffer[i - 1] == '"') {
                    at_i = true;
                } else if (_receiving_buffer[i - 2] == 'm' && _receiving_buffer[i - 3] == '"' && _receiving_buffer[i - 1] == '"') {
                    at_m = true;
                }
            } else {
                if (at_i) {
                    if (_receiving_buffer[i] < '0' || _receiving_buffer[i] > '9') {
                        at_i = false;
                    } else {
                        *remote_time *= 10;
                        *remote_time += _receiving_buffer[i] - '0';
                    }
                } else if (at_c) {
                    if (_receiving_buffer[i] < '0' || _receiving_buffer[i] > '9') {
                        at_c = false;
                    } else if (_receiving_buffer[i - 1] == ':') { // First number in the row
                        data_checksum = _receiving_buffer[i] - '0';
                        _receiving_buffer[i] = '0';
                    } else {
                        data_checksum *= 10;
                        data_checksum += _receiving_buffer[i] - '0';
                        continue;   // Avoids the copy of the char
                    }
                } else if (at_m) {
                    if (_receiving_buffer[i] < '0' || _receiving_buffer[i] > '9') {
                        at_m = false;
                    } else if (_receiving_buffer[i - 1] == ':') { // First number in the row
                        *message_code_int = _receiving_buffer[i] - '0';   // Message code found and it's a number
                    } else {
                        *message_code_int *= 10;
                        *message_code_int += _receiving_buffer[i] - '0';
                    }
                }
            }
            _receiving_buffer[data_i++] = _receiving_buffer[i]; // Does a left offset
        }
        *source_len = data_i;
        return data_checksum;
    }


    static size_t insertChecksum(size_t length) {

        uint16_t checksum = generateChecksum(_sending_buffer, length);

        #ifdef BROADCASTSOCKET_DEBUG
        Serial.print(F("I: Checksum is: "));
        Serial.println(checksum);
        #endif

        if (checksum > 0) { // It's already 0

            // First, find how many digits
            uint16_t temp = checksum;
            uint8_t num_digits = 0;
            while (temp > 0) {
                temp /= 10;
                num_digits++;
            }
            size_t data_i = length - 1;    // Old length (shorter)
            length += num_digits - 1;      // Discount the digit '0' already placed
            
            if (length > BROADCAST_SOCKET_BUFFER_SIZE)
                return length;  // buffer overflow

            bool at_c = false;
            for (size_t i = length - 1; data_i > 5; --i) {
                
                if (_sending_buffer[data_i - 2] == ':') {
                    if (_sending_buffer[data_i - 4] == 'c' && _sending_buffer[data_i - 5] == '"' && _sending_buffer[data_i - 3] == '"') {
                        at_c = true;
                    }
                } else if (at_c) {
                    if (checksum == 0) {
                        return length;
                    } else {
                        _sending_buffer[i] = '0' + checksum % 10;
                        checksum /= 10; // Truncates the number (does a floor)
                        continue;       // Avoids the copy of the char
                    }
                }
                _sending_buffer[i] = _sending_buffer[data_i--]; // Does an offset
            }
        }
        return length;
    }

    
protected:

    static char _receiving_buffer[BROADCAST_SOCKET_BUFFER_SIZE];
    static char _sending_buffer[BROADCAST_SOCKET_BUFFER_SIZE];
    static uint8_t _max_delay_ms;


    size_t triggerTalkers(size_t length) {

		#ifdef BROADCASTSOCKET_DEBUG
		Serial.print(class_name());
		Serial.print(F(" has a Talkers count of: "));
		Serial.println(_talker_count);
		#endif

        #ifdef BROADCASTSOCKET_DEBUG
        Serial.print(F("T: "));
        Serial.write(_receiving_buffer, length);
        Serial.println();
        #endif

        if (length > 3*4 + 2) {
            
            int message_code_int = 1000;    // There is no 1000 message code, meaning, it has none!
            uint32_t remote_time = 0;
            uint16_t received_checksum = extractChecksum(&length, &message_code_int, &remote_time);
            uint16_t checksum = generateChecksum(_receiving_buffer, length);
            
            #ifdef BROADCASTSOCKET_DEBUG
            Serial.print(F("C: Remote time: "));
            Serial.println(remote_time);
            #endif

            if (received_checksum == checksum) {
                #ifdef BROADCASTSOCKET_DEBUG
                Serial.print(F("C: Validated Checksum of "));
                Serial.println(checksum);
                #endif

                if (message_code_int == 1000) { // Found no message code!
                    #ifdef BROADCASTSOCKET_DEBUG
                    Serial.println(F("C: No message code!"));
                    #endif

                    return length;
                }
                
                if (_max_delay_ms > 0) {

                    JsonTalker::MessageCode message_code = static_cast<JsonTalker::MessageCode>(message_code_int);

                    if (!(message_code < JsonTalker::MessageCode::RUN || message_code > JsonTalker::MessageCode::GET)) {

                        #ifdef BROADCASTSOCKET_DEBUG
                        Serial.print(F("C: Message code requires delay check: "));
                        Serial.println(message_code_int);
                        #endif

                        const uint32_t local_time = millis();
                        
                        if (_control_timing) {
                            
                            const uint32_t remote_delay = _last_remote_time - remote_time;  // Package received after

                            if (remote_delay > 0 && remote_delay < MAX_NETWORK_PACKET_LIFETIME_MS) {    // Out of order package
                                const uint32_t allowed_delay = static_cast<uint32_t>(_max_delay_ms);
                                const uint32_t local_delay = local_time - _last_local_time;
                                #ifdef BROADCASTSOCKET_DEBUG
                                Serial.print(F("C: Local delay: "));
                                Serial.println(local_delay);
                                #endif
                                if (remote_delay > allowed_delay || local_delay > allowed_delay) {
                                    #ifdef BROADCASTSOCKET_DEBUG
                                    Serial.print(F("C: Out of time package (remote delay): "));
                                    Serial.println(remote_delay);
                                    #endif
                                    _drops_count++;
                                    return length;  // Out of time package (too late)
                                }
                            }
                        }
                        _last_local_time = local_time;
                        _last_remote_time = remote_time;
                        _control_timing = true;
                    }
                }

                // Triggers all Talkers to processes the received data
                bool pre_validated = false;
                for (uint8_t talker_i = 0; talker_i < _talker_count; ++talker_i) {

                    #ifdef BROADCASTSOCKET_DEBUG
                    Serial.print(F("Creating new JsonObject for talker: "));
                    Serial.println(_json_talkers[talker_i]->get_name());
                    #endif
                    
                    // JsonDocument in the stack makes sure its memory is released (NOT GLOBAL)
                    #if ARDUINOJSON_VERSION_MAJOR >= 7
                    JsonDocument message_doc;
                    #else
                    StaticJsonDocument<BROADCAST_SOCKET_BUFFER_SIZE> message_doc;
                    #endif

                    DeserializationError error = deserializeJson(message_doc, _receiving_buffer, length);
                    if (error) {
                        #ifdef BROADCASTSOCKET_DEBUG
                        Serial.println(F("Failed to deserialize received data"));
                        #endif
                        return 0;
                    }
                    JsonObject json_message = message_doc.as<JsonObject>();

					#ifdef BROADCASTSOCKET_DEBUG
					Serial.print(F("Triggering the talker: "));
					Serial.println(_json_talkers[talker_i]->get_name());
					#endif

					// A non static method
                    pre_validated = _json_talkers[talker_i]->processData(json_message, pre_validated);
                    if (!pre_validated) break;
                }
                
            } else {
                #ifdef BROADCASTSOCKET_DEBUG
                Serial.print(F("C: Validation of Checksum FAILED!!"));
                Serial.println(checksum);
                #endif
            }
        }
        return length;
    }


    BroadcastSocket(JsonTalker** json_talkers, uint8_t talker_count) {
			_json_talkers = json_talkers;
			_talker_count = talker_count;
            // Each talker has its remote connections, ONLY local connections are static
            for (uint8_t talker_i = 0; talker_i < _talker_count; ++talker_i) {
                _json_talkers[talker_i]->setSocket(this);
            }
        }


	// CAN'T BE STATIC
    // NOT Pure virtual methods anymores (= 0;)
    virtual size_t send(size_t length, bool as_reply = false, uint8_t target_index = 255) {
        (void)as_reply; 	// Silence unused parameter warning
        (void)target_index; // Silence unused parameter warning

        if (length < 3*4 + 2) {

            #ifdef BROADCASTSOCKET_DEBUG
            Serial.println(F("Error: Serialization failed"));
            #endif

            return 0;
        }

        #ifdef BROADCASTSOCKET_DEBUG
        Serial.print(F("S: "));
        Serial.write(_sending_buffer, length);
        Serial.println();
        #endif

        length = insertChecksum(length);
        
        if (length > BROADCAST_SOCKET_BUFFER_SIZE) {

            #ifdef BROADCASTSOCKET_DEBUG
            Serial.println(F("Error: Message too big"));
            #endif

            return 0;
        }

        #ifdef BROADCASTSOCKET_DEBUG
        Serial.print(F("T: "));
        Serial.write(_sending_buffer, length);
        Serial.println();
        #endif
        

        return length;
    }


public:
    // Delete copy/move operations
    BroadcastSocket(const BroadcastSocket&) = delete;
    BroadcastSocket& operator=(const BroadcastSocket&) = delete;
    BroadcastSocket(BroadcastSocket&&) = delete;
    BroadcastSocket& operator=(BroadcastSocket&&) = delete;

    virtual const char* class_name() const { return "BroadcastSocket"; }


	// CAN'T BE STATIC
    virtual size_t receive() {
        // In theory, a UDP packet on a local area network (LAN) could survive
        // for about 4.25 minutes (255 seconds).
        // BUT in practice it won't more that 256 milliseconds given that is a Ethernet LAN
        if (_control_timing && millis() - _last_local_time > MAX_NETWORK_PACKET_LIFETIME_MS) {
            _control_timing = false;
        }
        return 0;
    }

    
    bool remoteSend(JsonObject& json_message, bool as_reply = false, uint8_t target_index = 255) {

        JsonTalker::MessageCode message_code = static_cast<JsonTalker::MessageCode>(json_message["m"].as<int>());
        if (message_code != JsonTalker::MessageCode::ECHO && message_code != JsonTalker::MessageCode::ERROR) {
            json_message["i"] = (uint32_t)millis();

        } else if (!json_message["i"].is<uint32_t>()) { // Makes sure response messages have an "i" (identifier)

            #ifdef BROADCASTSOCKET_DEBUG
            Serial.print(F("R: Response message without an identifier (i)"));
            serializeJson(json_message, Serial);
            Serial.println();  // optional: just to add a newline after the JSON
            #endif

            return false;
        }

        size_t length = serializeJson(json_message, _sending_buffer, BROADCAST_SOCKET_BUFFER_SIZE);

        #ifdef BROADCASTSOCKET_DEBUG
        Serial.print(F("R: "));
        serializeJson(json_message, Serial);
        Serial.println();  // optional: just to add a newline after the JSON
        #endif

        return send(length, as_reply, target_index);	// send is internally triggered, so, this method can hardly be static
    }
    

    void set_max_delay(uint8_t max_delay_ms = 5) { _max_delay_ms = max_delay_ms; }
    uint8_t get_max_delay() { return _max_delay_ms; }
    uint16_t get_drops_count() { return _drops_count; }

};




#endif // BROADCAST_SOCKET_H
