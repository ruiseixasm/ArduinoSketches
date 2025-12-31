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

	enum LinkType : uint8_t {
		TALKIE_DOWN_LINKED,
		TALKIE_UP_LINKED,
		TALKIE_UP_BRIDGED	// TALKIE_UP_BRIDGED allows the sending of LOCAL broadcasted messages
	};

	enum TalkerMatch : uint8_t {
		TALKIE_MATCH_NONE,
		TALKIE_MATCH_ANY,
		TALKIE_MATCH_BY_CHANNEL,
		TALKIE_MATCH_BY_NAME,
		TALKIE_MATCH_FAIL
	};

	enum BroadcastValue : uint8_t {
		TALKIE_BC_REMOTE,
		TALKIE_BC_LOCAL,
		TALKIE_BC_SELF,
		TALKIE_BC_NONE
	};


    // PREFIXED VERSION - 100% safe across all boards
    enum class MessageValue : uint8_t {
        MSG_TALK,
        MSG_CHANNEL,
        MSG_PING,
        MSG_CALL,
        MSG_LIST,
        MSG_INFO,
        MSG_ECHO,
        MSG_ERROR,
        MSG_NOISE
    };

    enum class InfoValue : uint8_t {
        INFO_BOARD,
        INFO_DROPS,
        INFO_DELAY,
        INFO_MUTE,
        INFO_SOCKET,
        INFO_TALKER,
        INFO_MANIFESTO,
        INFO_UNDEFINED
    };

    enum class RogerValue : uint8_t {
        ROGER,
		NEGATIVE,
		SAY_AGAIN,
		NIL
    };

    enum class ErrorValue : uint8_t {
        ERR_FROM,
        ERR_FIELD,
        ERR_CHECKSUM,
        ERR_MESSAGE,
        ERR_IDENTITY,
        ERR_DELAY,
        ERR_KEY,
        ERR_DATA
    };

};



#endif // TALKIE_CODES_HPP
