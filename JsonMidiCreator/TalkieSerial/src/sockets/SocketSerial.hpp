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

class SocketSerial : public BroadcastSocket {
public:

    const char* class_name() const override { return "SocketSerial"; }


protected:

    // Singleton accessor
    SocketSerial() : BroadcastSocket() {}

    
    bool send(const JsonMessage& json_message) override {
        (void)json_message;	// Silence unused parameter warning

		if (_sending_length) {
			if (Serial.write(_sending_buffer, _sending_length) == _sending_length) {
				_sending_length = 0;
				return true;
			}
			_sending_length = 0;
		}
		return false;
    }


    size_t receive() override {
		_received_length =
			static_cast<size_t>(Serial.readBytes(_received_buffer, BROADCAST_SOCKET_BUFFER_SIZE));
		if (_received_length) {
			BroadcastSocket::startTransmission();
		}
        return _received_length;
    }


public:
    // Move ONLY the singleton instance method to subclass
    static SocketSerial& instance() {

        static SocketSerial instance;
        return instance;
    }

};

#endif // SOCKET_SERIAL_HPP
