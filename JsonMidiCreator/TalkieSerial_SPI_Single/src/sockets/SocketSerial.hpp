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
#ifndef SOCKET_SERIAL_HPP
#define SOCKET_SERIAL_HPP

#include "../BroadcastSocket.h"


// #define SOCKET_SERIAL_DEBUG
// #define SOCKET_SERIAL_DEBUG_TIMING

class SocketSerial : public BroadcastSocket {
public:

    const char* class_name() const override { return "SocketSerial"; }

	#ifdef SOCKET_SERIAL_DEBUG_TIMING
	unsigned long _reference_time = millis();
	#endif


protected:

    // Singleton accessor
    SocketSerial() : BroadcastSocket() {}

	bool _reading_serial = false;
    
    void _send(const JsonMessage& json_message) override {
        (void)json_message;	// Silence unused parameter warning

		#ifdef SOCKET_SERIAL_DEBUG_TIMING
		Serial.print(" | ");
		Serial.print(millis() - _reference_time);
		#endif

		if (_sending_length) {
			
			#ifdef SOCKET_SERIAL_DEBUG_TIMING
			Serial.print(" | ");
			Serial.print(millis() - _reference_time);
			#endif

			if (Serial.write(_sending_buffer, _sending_length) == _sending_length) {
				
				#ifdef SOCKET_SERIAL_DEBUG_TIMING
				Serial.print(" | ");
				Serial.print(millis() - _reference_time);
				#endif

			}
			_sending_length = 0;
		}
    }


    void _receive() override {
    
		#ifdef SOCKET_SERIAL_DEBUG_TIMING
		_reference_time = millis();
		#endif

		while (Serial.available()) {
			char c = Serial.read();

			if (_reading_serial) {
				if (_received_length < TALKIE_BUFFER_SIZE) {
					if (c == '}' && _received_length && _received_buffer[_received_length - 1] != '\\') {
						_reading_serial = false;

						#ifdef SOCKET_SERIAL_DEBUG_TIMING
						Serial.print(millis() - _reference_time);
						#endif

						_received_buffer[_received_length++] = '}';
						_startTransmission();
						return;
					} else {
						_received_buffer[_received_length++] = c;
					}
				} else {
					_reading_serial = false;
					_received_length = 0; // Reset to start writing
				}
			} else if (c == '{') {
				_reading_serial = true;

				#ifdef SOCKET_SERIAL_DEBUG_TIMING
				Serial.print("\n");
				Serial.print(class_name());
				Serial.print(": ");
				#endif

				_received_length = 0; // Reset to start writing
				_received_buffer[_received_length++] = '{';
			}
		}
    }


public:
    // Move ONLY the singleton instance method to subclass
    static SocketSerial& instance() {

        static SocketSerial instance;
        return instance;
    }

};

#endif // SOCKET_SERIAL_HPP
