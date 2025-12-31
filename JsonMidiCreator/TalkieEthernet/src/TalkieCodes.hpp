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
    enum MessageValue : uint8_t {
        TALKIE_MSG_TALK,
        TALKIE_MSG_CHANNEL,
        TALKIE_MSG_PING,
        TALKIE_MSG_CALL,
        TALKIE_MSG_LIST,
        TALKIE_MSG_INFO,
        TALKIE_MSG_ECHO,
        TALKIE_MSG_ERROR,
        TALKIE_MSG_NOISE
    };

    enum InfoValue : uint8_t {
        TALKIE_INFO_BOARD,
        TALKIE_INFO_DROPS,
        TALKIE_INFO_DELAY,
        TALKIE_INFO_MUTE,
        TALKIE_INFO_SOCKET,
        TALKIE_INFO_TALKER,
        TALKIE_INFO_MANIFESTO,
        TALKIE_INFO_UNDEFINED
    };

    enum RogerValue : uint8_t {
        TALKIE_ROGER_ROGER,
		TALKIE_ROGER_NEGATIVE,
		TALKIE_ROGER_SAY_AGAIN,
		TALKIE_ROGER_NIL
    };

    enum ErrorValue : uint8_t {
        TALKIE_ERR_FROM,
        TALKIE_ERR_FIELD,
        TALKIE_ERR_CHECKSUM,
        TALKIE_ERR_MESSAGE,
        TALKIE_ERR_IDENTITY,
        TALKIE_ERR_ERR_DELAY,
        TALKIE_ERR_ERR_KEY,
        TALKIE_ERR_ERR_DATA
    };

};



#endif // TALKIE_CODES_HPP
