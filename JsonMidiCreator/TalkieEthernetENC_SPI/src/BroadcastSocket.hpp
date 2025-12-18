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
#include "TalkieCodes.hpp"


// #define BROADCASTSOCKET_DEBUG

// Readjust if absolutely necessary
#define BROADCAST_SOCKET_BUFFER_SIZE 128
#define MAX_NETWORK_PACKET_LIFETIME_MS 256UL    // 256 milliseconds

class BroadcastSocket {
protected:

    char _receiving_buffer[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};
    char _sending_buffer[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};
	
	uint8_t _received_length = 0;
	uint8_t _sending_length = 0;

    // Pointer PRESERVE the polymorphism while objects don't!
    JsonTalker** _json_talkers = nullptr;   // It's a singleton, so, no need to be static
    uint8_t _talker_count = 0;
    uint8_t _max_delay_ms = 5;
    bool _control_timing = false;
    uint16_t _last_local_time = 0;
    uint16_t _last_remote_time = 0;
    uint16_t _drops_count = 0;

    // JsonDocument intended to be reused
    #if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument _message_doc;
    #else
    StaticJsonDocument<BROADCAST_SOCKET_BUFFER_SIZE> _message_doc;
    #endif


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


    uint16_t extractChecksum(uint8_t* message_code_int, uint16_t* remote_time) {
        
        uint16_t data_checksum = 0;
        // Has to be pre processed (linearly)
        bool at_m = false;
        bool at_c = false;
        bool at_i = false;
        uint8_t data_i = 4;	// Optimized {"c": ...
        for (uint8_t i = data_i; i < _received_length; ++i) {
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
        _received_length = data_i;
        return data_checksum;
    }


    bool insertChecksum() {

        uint16_t checksum = generateChecksum(_sending_buffer, _sending_length);

        #ifdef BROADCASTSOCKET_DEBUG
        Serial.print(F("insertChecksum1: Checksum is: "));
        Serial.println(checksum);
        #endif

        if (checksum > 0) { // It's already 0

			#ifdef BROADCASTSOCKET_DEBUG
			Serial.print(F("insertChecksum2: Initial length: "));
			Serial.println(_sending_length);
			#endif

            // First, find how many digits
            uint16_t temp = checksum;
            uint8_t num_digits = 0;
            while (temp > 0) {
                temp /= 10;
                num_digits++;
            }
            uint8_t data_i = _sending_length - 1;	// Old length (shorter) (binary processing, no '\0' to take into consideration)
			uint8_t new_length = _sending_length + num_digits - 1;	// Discount the digit '0' already placed
            
			#ifdef BROADCASTSOCKET_DEBUG
			Serial.print(F("insertChecksum3: New length: "));
			Serial.println(new_length);
			#endif

            if (new_length > BROADCAST_SOCKET_BUFFER_SIZE)
                return false;  // buffer overflow

			_sending_length = new_length;

            bool at_c = false;
            for (uint8_t i = new_length - 1; data_i > 4; i--) {	// 'i = new_length - 1' because it's a binary processing, no '\0' to take into consideration
                
                if (_sending_buffer[data_i - 2] == ':') {	// Must find it at 5 the least (> 4)
                    if (_sending_buffer[data_i - 4] == 'c' && _sending_buffer[data_i - 5] == '"' && _sending_buffer[data_i - 3] == '"') {
                        at_c = true;
                    }
                } else if (at_c) {
                    if (checksum == 0) {
                        return true;
                    } else {
                        _sending_buffer[i] = '0' + checksum % 10;
                        checksum /= 10; // Truncates the number (does a floor)
                        continue;       // Avoids the copy of the char
                    }
                }
                _sending_buffer[i] = _sending_buffer[data_i--]; // Does an offset (NOTE the continue above)
            }
        }
        return true;
    }


	// Allows the overriding class to peek at the received JSON message
	virtual bool checkJsonMessage(const JsonObject& json_message) {
		if (!json_message[ JsonKey::FROM ].is<String>()) {
			#ifdef JSON_TALKER_DEBUG
			Serial.println(F("ERROR: From key 'f' is missing"));
			#endif
			return false;
		}
		return true;
	}

    
    uint8_t triggerTalkers() {

		#ifdef BROADCASTSOCKET_DEBUG
		Serial.print(class_name());
		Serial.print(F(": triggerTalkers1: has a Talkers count of: "));
		Serial.println(_talker_count);
        Serial.print(F("triggerTalkers2: "));
        Serial.write(_receiving_buffer, _received_length);
        Serial.println();
        #endif

        if (_received_length > 3*4 + 2) {
            
            uint8_t message_code_int = 255;    // There is no 255 message code, meaning, it has none!
            uint16_t remote_time = 0;
            uint16_t received_checksum = extractChecksum(&message_code_int, &remote_time);
            uint16_t checksum = generateChecksum(_receiving_buffer, _received_length);
            
            #ifdef BROADCASTSOCKET_DEBUG
            Serial.print(F("triggerTalkers3: Remote time: "));
            Serial.println(remote_time);
            #endif

            if (received_checksum == checksum) {
                #ifdef BROADCASTSOCKET_DEBUG
                Serial.print(F("triggerTalkers4: Validated Checksum of "));
                Serial.println(checksum);
                #endif

                if (message_code_int == 255) { // Found no message code!
                    #ifdef BROADCASTSOCKET_DEBUG
                    Serial.println(F("triggerTalkers5: No message code!"));
                    #endif

                    return _received_length;
                }
                
                if (_max_delay_ms > 0) {

                    MessageData message_code = static_cast<MessageData>(message_code_int);

                    if (message_code < MessageData::LIST) {	// Only does time control on Actions (drops) (< LIST)

                        #ifdef BROADCASTSOCKET_DEBUG
                        Serial.print(F("triggerTalkers6: Message code requires delay check: "));
                        Serial.println(message_code_int);
                        #endif

                        const uint16_t local_time = (uint16_t)millis();
                        
                        if (_control_timing) {
                            
                            const uint16_t remote_delay = _last_remote_time - remote_time;  // Package received after

                            if (remote_delay > 0 && remote_delay < MAX_NETWORK_PACKET_LIFETIME_MS) {    // Out of order package
                                const uint16_t allowed_delay = static_cast<uint16_t>(_max_delay_ms);
                                const uint16_t local_delay = local_time - _last_local_time;
                                #ifdef BROADCASTSOCKET_DEBUG
                                Serial.print(F("triggerTalkers7: Local delay: "));
                                Serial.println(local_delay);
                                #endif
                                if (remote_delay > allowed_delay || local_delay > allowed_delay) {
                                    #ifdef BROADCASTSOCKET_DEBUG
                                    Serial.print(F("triggerTalkers8: Out of time package (remote delay): "));
                                    Serial.println(remote_delay);
                                    #endif
                                    _drops_count++;
                                    return _received_length;  // Out of time package (too late)
                                }
                            }
                        }
                        _last_local_time = local_time;
                        _last_remote_time = remote_time;
                        _control_timing = true;
                    }
                }

				// Gives a chance to show it one time
				DeserializationError error = deserializeJson(_message_doc, _receiving_buffer, _received_length);
				if (error) {
					#ifdef BROADCASTSOCKET_DEBUG
					Serial.println(F("ERROR: Failed to deserialize received data"));
					#endif
					return 0;
				}
				JsonObject json_message = _message_doc.as<JsonObject>();

				if (!checkJsonMessage(json_message)) return 0;

				if (!json_message[ JsonKey::IDENTITY ].is<uint16_t>()) {
					#ifdef JSON_TALKER_DEBUG
					Serial.println(4);
					#endif
					json_message[ JsonKey::ORIGINAL ] = json_message[ JsonKey::MESSAGE ];
					json_message[ JsonKey::MESSAGE ] = static_cast<int>(MessageData::ERROR);
					json_message[ JsonKey::ERROR ] = static_cast<int>(ErrorData::IDENTITY);
					// Wrong type of identifier or no identifier, so, it has to insert new identifier
					json_message[ JsonKey::IDENTITY ] = (uint16_t)millis();
					// From one to many, starts to set the returning target in this single place only
					json_message[ JsonKey::TO ] = json_message[ JsonKey::FROM ];

					remoteSend(json_message);	// Includes reply swap
					return 0;
				}
				
                // Triggers all Talkers to processes the received data
                bool pre_validated = false;
                for (uint8_t talker_i = 0; talker_i < _talker_count; ++talker_i) {	// _talker_count makes the code safe

                    #ifdef BROADCASTSOCKET_DEBUG
                    Serial.print(F("triggerTalkers9: Creating new JsonObject for talker: "));
                    Serial.println(_json_talkers[talker_i]->get_name());
                    #endif

					if (talker_i > 0) {
						DeserializationError error = deserializeJson(_message_doc, _receiving_buffer, _received_length);
						if (error) {
							#ifdef BROADCASTSOCKET_DEBUG
							Serial.println(F("ERROR: Failed to deserialize received data"));
							#endif
							return 0;
						}
						json_message = _message_doc.as<JsonObject>();
					}
                    
					#ifdef BROADCASTSOCKET_DEBUG
					Serial.print(F("triggerTalkers10: Triggering the talker: "));
					Serial.println(_json_talkers[talker_i]->get_name());
					#endif

					// A non static method
                    pre_validated = _json_talkers[talker_i]->processMessage(json_message);
                    if (!pre_validated) return 0;
                }
                
            } else {
                #ifdef BROADCASTSOCKET_DEBUG
                Serial.print(F("triggerTalkers9: Validation of Checksum FAILED: "));
                Serial.println(checksum);
                #endif
            }
        }
        return _received_length;
    }


    BroadcastSocket(JsonTalker** json_talkers, uint8_t talker_count) {
			_json_talkers = json_talkers;
			_talker_count = talker_count;
            // Each talker has its remote connections, ONLY local connections are static
            for (uint8_t talker_i = 0; talker_i < _talker_count; ++talker_i) {
                _json_talkers[talker_i]->setSocket(this);
            }
        }


	virtual bool availableReceivingBuffer(uint8_t wait_seconds = 3) {
		uint16_t start_waiting = (uint16_t)millis();
		while (_received_length) {
			if ((uint16_t)millis() - start_waiting > 1000 * wait_seconds) {
				return false;
			}
		}
		return true;
	}

	virtual bool availableSendingBuffer(uint8_t wait_seconds = 3) {
		uint16_t start_waiting = (uint16_t)millis();
		while (_sending_length) {
			if ((uint16_t)millis() - start_waiting > 1000 * wait_seconds) {

				#ifdef BROADCASTSOCKET_DEBUG
				Serial.println(F("\tavailableSendingBuffer: NOT available sending buffer"));
				#endif

				return false;
			}
		}
		
		#ifdef BROADCASTSOCKET_DEBUG
		Serial.println(F("\tavailableSendingBuffer: Available sending buffer"));
		#endif

		return true;
	}


    virtual bool send(const JsonObject& json_message) {
        (void)json_message; // Silence unused parameter warning

        if (_sending_length < 3*4 + 2) {

            #ifdef BROADCASTSOCKET_DEBUG
            Serial.println(F("Error: Serialization failed"));
            #endif

            return false;
        }

        #ifdef BROADCASTSOCKET_DEBUG
        Serial.print(F("send1: "));
        Serial.write(_sending_buffer, _sending_length);
        Serial.println();
        #endif

        if (insertChecksum()) {	// Where the CHECKSUM is set

			if (_sending_length > BROADCAST_SOCKET_BUFFER_SIZE) {

				#ifdef BROADCASTSOCKET_DEBUG
				Serial.println(F("ERROR: Message too big"));
				#endif

				return false;
			}

			#ifdef BROADCASTSOCKET_DEBUG
			Serial.print(F("send2: "));
			Serial.write(_sending_buffer, _sending_length);
			Serial.println();
			#endif

		} else {
			
			#ifdef BROADCASTSOCKET_DEBUG
			Serial.println(F("ERROR: Couldn't insert Checksum"));
			#endif
        	return false;
		}
        return true;
    }


    virtual uint8_t receive() {
        // In theory, a UDP packet on a local area network (LAN) could survive
        // for about 4.25 minutes (255 seconds).
        // BUT in practice it won't more that 256 milliseconds given that is a Ethernet LAN
        if (_control_timing && (uint16_t)millis() - _last_local_time > MAX_NETWORK_PACKET_LIFETIME_MS) {
            _control_timing = false;
        }
        return 0;
    }


public:
    // Delete copy/move operations
    BroadcastSocket(const BroadcastSocket&) = delete;
    BroadcastSocket& operator=(const BroadcastSocket&) = delete;
    BroadcastSocket(BroadcastSocket&&) = delete;
    BroadcastSocket& operator=(BroadcastSocket&&) = delete;

    virtual const char* class_name() const { return "BroadcastSocket"; }


    virtual void loop() {
        receive();
        for (uint8_t talker_i = 0; talker_i < _talker_count; ++talker_i) {
            _json_talkers[talker_i]->loop();
        }
    }


	bool hasTalker(JsonTalker* json_talker) {

        for (uint8_t talker_i = 0; talker_i < _talker_count; ++talker_i) {
			if (_json_talkers[talker_i] == json_talker) return true;
		}
		return false;
	}
    

    bool remoteSend(JsonObject& json_message) {

		// Makes sure 'c' is correctly set as 0, BroadcastSocket responsibility
		json_message[ JsonKey::CHECKSUM ] = 0;

		#ifdef BROADCASTSOCKET_DEBUG
		Serial.print(F("remoteSend1: "));
		serializeJson(json_message, Serial);
		Serial.println();  // optional: just to add a newline after the JSON
		#endif

		// Before writing on the _sending_buffer it needs the wait for its availability

		if (availableSendingBuffer()) {

			// This length excludes the '\0' char
			// serializeJson() returns length without \0, but adds \0 to the buffer. Your SPI code should send until it finds \0.
			_sending_length = serializeJson(json_message, _sending_buffer, BROADCAST_SOCKET_BUFFER_SIZE);

			#ifdef BROADCASTSOCKET_DEBUG
			Serial.print(F("remoteSend3: "));
			Serial.write(_sending_buffer, _sending_length);
			Serial.println();
			Serial.print(F("remoteSend4: JSON length: "));
			Serial.println(_sending_length);
			#endif

			return send(json_message);
		}

		return false;
    }
    

    void set_max_delay(uint8_t max_delay_ms = 5) { _max_delay_ms = max_delay_ms; }
    uint8_t get_max_delay() { return _max_delay_ms; }
    uint16_t get_drops_count() { return _drops_count; }

};




#endif // BROADCAST_SOCKET_H
