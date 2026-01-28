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

#include <BroadcastSocket.h>


// #define SOCKET_SERIAL_DEBUG
// #define SOCKET_SERIAL_DEBUG_TIMING

class S_SocketSerial : public BroadcastSocket {
public:

	// The Socket class description shouldn't be greater than 35 chars
	// {"m":7,"f":"","s":3,"b":1,"t":"","i":58485,"0":1,"1":"","2":11,"c":11266} <-- 128 - (73 + 2*10) = 35
    const char* class_description() const override { return "SocketSerial"; }

	#ifdef SOCKET_SERIAL_DEBUG_TIMING
	unsigned long _reference_time = millis();
	#endif


protected:

    // Singleton accessor
    S_SocketSerial() : BroadcastSocket() {}

	JsonMessage _json_message;
	bool _reading_serial = false;


    void _receive() override {
    
		#ifdef SOCKET_SERIAL_DEBUG_TIMING
		_reference_time = millis();
		#endif

		while (Serial.available()) {
			char c = Serial.read();

			char* message_buffer = _json_message._write_buffer();
			if (_reading_serial) {

				size_t message_length = _json_message.get_length();
				if (message_length < TALKIE_BUFFER_SIZE) {
					if (c == '}' && message_length && message_buffer[message_length - 1] != '\\') {

						_reading_serial = false;

						#ifdef SOCKET_SERIAL_DEBUG_TIMING
						Serial.print(millis() - _reference_time);
						#endif

						if (_json_message._append('}')) {
							_startTransmission(_json_message);
						}
						return;
					} else if (!_json_message._append(c)) {
						return;
					}
				} else {
					_reading_serial = false;
					_json_message._set_length(0);	// Reset to start writing
				}
			} else if (c == '{') {
				
				_json_message._set_length(0);
				_reading_serial = true;

				#ifdef SOCKET_SERIAL_DEBUG_TIMING
				Serial.print("\n");
				Serial.print(class_description());
				Serial.print(": ");
				#endif

				_json_message._append('{');
			}
		}
    }


    bool _send(const JsonMessage& json_message) override {

		#ifdef SOCKET_SERIAL_DEBUG_TIMING
		Serial.print(" | ");
		Serial.print(millis() - _reference_time);
		#endif

		const char* message_buffer = json_message._read_buffer();
		size_t message_length = json_message.get_length();
		return Serial.write(message_buffer, message_length) == message_length;
    }


public:
    // Move ONLY the singleton instance method to subclass
    static S_SocketSerial& instance() {

        static S_SocketSerial instance;
        return instance;
    }

};

#endif // SOCKET_SERIAL_HPP
