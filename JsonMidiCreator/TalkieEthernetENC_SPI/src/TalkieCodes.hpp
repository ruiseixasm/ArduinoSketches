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


/**
 * @file TalkieCodes.hpp
 * @brief All JSON message field codes including Linkage ones
 * 
 * @author Rui Seixas Monteiro
 * @date Created: 2026-01-03
 * @version 1.0.0
 */

#ifndef TALKIE_CODES_HPP
#define TALKIE_CODES_HPP

#include <Arduino.h>        // Needed for Serial given that Arduino IDE only includes Serial in .ino files!


/**
 * @file TalkieCodes.hpp
 * @brief This sets the multiple codes associated to the json values.
 * 
 * By default an inexistent key returns 0 as number, so, any first key
 * bellow is considered the respective default key value.
 * 
 * @author Rui Seixas Monteiro
 * @date Created: 2026-01-06
 * @version 1.0.0
 */

// PREFIXED VERSION - 100% safe across all boards
struct TalkieCodes {

	enum ValueType : uint8_t {
		TALKIE_VT_VOID,
		TALKIE_VT_OTHER,
		TALKIE_VT_INTEGER,
		TALKIE_VT_STRING
	};

	
	enum LinkType : uint8_t {
		TALKIE_LT_NONE,
		TALKIE_LT_DOWN_LINKED,
		TALKIE_LT_UP_LINKED,
		TALKIE_LT_UP_BRIDGED	// TALKIE_LT_UP_BRIDGED allows the sending of LOCAL broadcasted messages
	};


	enum TalkerMatch : uint8_t {
		TALKIE_MATCH_NONE,
		TALKIE_MATCH_ANY,
		TALKIE_MATCH_BY_CHANNEL,
		TALKIE_MATCH_BY_NAME,
		TALKIE_MATCH_FAIL
	};


	enum BroadcastValue : uint8_t {
		TALKIE_BC_NONE,
		TALKIE_BC_REMOTE,
		TALKIE_BC_LOCAL,
		TALKIE_BC_SELF
	};


    enum MessageValue : uint8_t {
        TALKIE_MSG_TALK,
        TALKIE_MSG_CHANNEL,
        TALKIE_MSG_PING,
        TALKIE_MSG_CALL,
        TALKIE_MSG_LIST,
        TALKIE_MSG_SYSTEM,
        TALKIE_MSG_ECHO,
        TALKIE_MSG_ERROR,
        TALKIE_MSG_NOISE
    };


    enum SystemValue : uint8_t {
        TALKIE_SYS_UNDEFINED,
        TALKIE_SYS_BOARD,
        TALKIE_SYS_MUTE,
        TALKIE_SYS_DROPS,
        TALKIE_SYS_DELAY,
        TALKIE_SYS_SOCKET,
        TALKIE_SYS_MANIFESTO
    };


    enum RogerValue : uint8_t {
        TALKIE_RGR_ROGER,
		TALKIE_RGR_NEGATIVE,
		TALKIE_RGR_SAY_AGAIN,
		TALKIE_RGR_NIL,
		TALKIE_RGR_NO_JOY
    };


    enum ErrorValue : uint8_t {
        TALKIE_ERR_UNDEFINED,
        TALKIE_ERR_CHECKSUM,
        TALKIE_ERR_MESSAGE,
        TALKIE_ERR_IDENTITY,
        TALKIE_ERR_FIELD,
        TALKIE_ERR_FROM,
        TALKIE_ERR_TO,
        TALKIE_ERR_DELAY,
        TALKIE_ERR_KEY,
        TALKIE_ERR_VALUE
    };
};


#endif // TALKIE_CODES_HPP
