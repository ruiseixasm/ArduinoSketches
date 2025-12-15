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


enum class MessageCode : int {
	TALK,
	LIST,
	RUN,
	SET,
	GET,
	SYS,
	ECHO,
	ERROR,
	CHANNEL
};


enum class SystemCode : int {
	MUTE, UNMUTE, MUTED, BOARD, PING, DROPS, DELAY
};


enum class EchoCode : int {
	ROGER,
	SAY_AGAIN,
	NEGATIVE
};


enum class JsonKey : char {
	CHECKSUM    = 'c',
	IDENTITY    = 'i',
	MESSAGE     = 'm',
	ORIGINAL	= 'o',
	FROM        = 'f',
	TO          = 't',
	SYSTEM      = 's',
	ERROR       = 'e',
	VALUE       = 'v',
	REPLY       = 'r',
	ROGER       = 'g',
	ACTION      = 'a',
	NAME        = 'n',
	INDEX		= 'x',
	DESCRIPTION = 'd'
};


inline const char* key_str(JsonKey json_key) {
	// The flip_flop avoids the usage of the sam string in a single line double operations
	static uint8_t flip_flop = 0;
	static char json_key_string[2][2] = {{'\0'}, {'\0'}};
	flip_flop = 1 - flip_flop;
	json_key_string[flip_flop][0] = static_cast<char>(json_key);
	return json_key_string[flip_flop];
}


#endif // TALKIE_CODES_HPP
