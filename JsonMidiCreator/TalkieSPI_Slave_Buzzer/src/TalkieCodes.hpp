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
#ifndef TALKIE_CODES_HPP
#define TALKIE_CODES_HPP

#include <Arduino.h>        // Needed for Serial given that Arduino IDE only includes Serial in .ino files!


struct TalkieCodes {

	enum class LinkType : uint8_t {
		DOWN_LINKED, UP_LINKED
	};


	// Add a char "enum" struct
    struct MessageKey {
        static constexpr const char BROADCAST		= 'b';
        static constexpr const char TIMESTAMP		= 'i';
        static constexpr const char IDENTITY 		= 'i';
        static constexpr const char MESSAGE 		= 'm';
        static constexpr const char FROM 			= 'f';
        static constexpr const char TO 				= 't';
        static constexpr const char INFO 			= 's';
        static constexpr const char ACTION			= 'a';
        static constexpr const char ROGER 			= 'r';
    };


	enum class BroadcastValue : uint8_t {
		REMOTE, LOCAL, SELF, NONE
	};


	enum class MessageValue : uint8_t {
		TALK, CHANNEL, PING, CALL, LIST, INFO, ECHO, ERROR, NOISE
	};


	enum class InfoValue : uint8_t {
		BOARD, DROPS, DELAY, MUTE, SOCKET, TALKER, MANIFESTO, UNDEFINED
	};


	// Only applicable to CALL messages
	enum class RogerValue : uint8_t {
		ROGER, NEGATIVE, SAY_AGAIN, NIL
	};


	enum class ErrorValue : uint8_t {
		FROM, FIELD, CHECKSUM, MESSAGE, IDENTITY, DELAY, KEY, DATA
	};

};



#endif // TALKIE_CODES_HPP
