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

	// Add a string "enum" struct
    struct TalkieKey {
        static constexpr const char* SOURCE			= "c";
        static constexpr const char* CHECKSUM		= "c";
        static constexpr const char* TIMESTAMP		= "i";
        static constexpr const char* IDENTITY 		= "i";
        static constexpr const char* MESSAGE 		= "m";
        static constexpr const char* FROM 			= "f";
        static constexpr const char* TO 			= "t";
        static constexpr const char* SYSTEM 		= "s";
        static constexpr const char* ERROR 			= "e";
        static constexpr const char* ROGER 			= "r";
        static constexpr const char* NAME 			= "n";
        static constexpr const char* INDEX 			= "x";
        static constexpr const char* DESCRIPTION	= "d";
    };


	static const char* dataKey(size_t nth = 0) {
		switch (nth) {
			case 1: 	return "1";
			case 2: 	return "2";
			case 3: 	return "3";
			case 4: 	return "4";
			case 5: 	return "5";
			case 6: 	return "6";
			case 7: 	return "7";
			case 8: 	return "8";
			case 9: 	return "9";
			default: 	return "0";
		}
	}


	enum class SourceValue : int {
		REMOTE, LOCAL, HERE
	};


	enum class MessageValue : int {
		TALK, CHANNEL, PING, CALL, LIST, SYS, ECHO, ERROR, NOISE
	};


	enum class SystemValue : int {
		BOARD, DROPS, DELAY, MUTE, UNMUTE, MUTED, SOCKET, TALKER, MANIFESTO
	};


	enum class EchoValue : int {
		ROGER, SAY_AGAIN, NEGATIVE, NIL
	};


	enum class ErrorValue : int {
		FROM, FIELD, CHECKSUM, MESSAGE, IDENTITY, DELAY, KEY, DATA
	};

};



#endif // TALKIE_CODES_HPP
