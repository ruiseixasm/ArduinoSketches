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
	FROM        = 'f',
	TO          = 't',
	SYSTEM      = 's',
	ERROR       = 'e',
	VALUE       = 'v',
	REPLY       = 'r',
	ROGER       = 'g',
	NAME        = 'n',
	DESCRIPTION = 'd'
};


inline const char* key_string(JsonKey json_key, bool upper_case = false) {
	static char json_key_string[2] = {'\0'};
	json_key_string[0] = static_cast<char>(json_key);
	if (upper_case) {
		json_key_string[0] += 'A' - 'a';
	}
	return json_key_string;
}


#endif // TALKIE_CODES_HPP
