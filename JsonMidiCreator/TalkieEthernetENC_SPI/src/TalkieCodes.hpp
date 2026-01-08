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
 * @brief This sets the multiple codes associated to the json values.
 * 
 * @author Rui Seixas Monteiro
 * @date Created: 2026-01-06
 * @version 1.0.0
 */

#ifndef TALKIE_CODES_HPP
#define TALKIE_CODES_HPP

#include <Arduino.h>        // Needed for Serial given that Arduino IDE only includes Serial in .ino files!


/**
 * @struct TalkieCodes
 * @brief Centralized enumeration repository for the Talker communication system
 * 
 * This struct contains all enumerated types used throughout the Talkie protocol,
 * providing type safety and clear intent for communication operations.
 * By default an inexistent key returns 0 as number, so, any first key bellow is
 * considered the respective default key value.
 * 
 * @see Talker
 * @see Manifesto
 * @see BroadcastSocket
 * @see MessageRepeater
 * @ingroup JsonTalkie
 */
struct TalkieCodes {

    /**
     * @enum ValueType
     * @brief Data type classification for message values
     * 
     * Used to identify the type of data contained in json fields,
     * because these can be strings or numbers.
     */
    enum ValueType : uint8_t {
        TALKIE_VT_VOID,      ///< No value (null/empty)
        TALKIE_VT_OTHER,     ///< Custom or complex data type
        TALKIE_VT_INTEGER,   ///< Numeric integer value
        TALKIE_VT_STRING     ///< Text string value
    };


    /**
     * @enum LinkType
     * @brief Network linkage configuration for Socket and Talker positioning
     * 
     * Defines how a Socket or a Talker is connected to the Message Repeater,
     * affecting message routing to other Sockets and Talkers.
     */
    enum LinkType : uint8_t {
        TALKIE_LT_NONE,         ///< No special linkage (standalone)
        TALKIE_LT_DOWN_LINKED,  ///< Linked as a Local node (LOCAL and REMOTE messages)
        TALKIE_LT_UP_LINKED,    ///< Linked as a remote node associated to REMOTE messages
        TALKIE_LT_UP_BRIDGED    ///< Up-bridged: Can send LOCAL broadcast messages too
    };


    /**
     * @enum TalkerMatch
     * @brief The type of match associated to the Talker
     * 
     * Based on the `to` field of the message a Talker has its handle transmission method
	 * called, the match by which the Talker is select is defined by this enum.
     */
    enum TalkerMatch : uint8_t {
        TALKIE_MATCH_NONE,        ///< No match attempted
        TALKIE_MATCH_ANY,         ///< Matches any Talker (wildcard)
        TALKIE_MATCH_BY_CHANNEL,  ///< Successfully matched by channel number
        TALKIE_MATCH_BY_NAME,     ///< Successfully matched by name string
        TALKIE_MATCH_FAIL         ///< Matching failed (no such Talker)
    };


    /**
     * @enum MessageValue
     * @brief Primary message type classification
     * 
     * Core message types that define the purpose and handling
     * of communication packets in the Talkie protocol.
     */
    enum MessageValue : uint8_t {
        TALKIE_MSG_NOISE,   ///< Invalid, missing or malformed data
        TALKIE_MSG_TALK,    ///< Lists existent devices in the network
        TALKIE_MSG_CHANNEL, ///< Channel management/configuration
        TALKIE_MSG_PING,    ///< Network presence check and latency
        TALKIE_MSG_CALL,    ///< Action Talker invocation
        TALKIE_MSG_LIST,    ///< Lists Talker actions
        TALKIE_MSG_SYSTEM,  ///< System control/status messages
        TALKIE_MSG_ECHO,    ///< Messages Echo returns
        TALKIE_MSG_ERROR    ///< Error notification
    };


    /**
     * @enum BroadcastValue
     * @brief Scope of broadcast message distribution
     * 
     * Specifies how far a broadcast message should propagate
     * through the environment.
     */
    enum BroadcastValue : uint8_t {
        TALKIE_BC_NONE,    ///< No broadcast, the message is dropped
        TALKIE_BC_REMOTE,  ///< Broadcast to remote talkers
        TALKIE_BC_LOCAL,   ///< Broadcast within local network talkers
        TALKIE_BC_SELF     ///< Broadcast to self only (loopback)
    };


    /**
     * @enum SystemValue
     * @brief Associated to the system state and configuration
     */
    enum SystemValue : uint8_t {
        TALKIE_SYS_UNDEFINED, ///< Unspecified system request
        TALKIE_SYS_BOARD,     ///< Board/system information request
        TALKIE_SYS_MUTE,      ///< Returns or sets the mute mode
        TALKIE_SYS_DROPS,     ///< Packet loss statistics
        TALKIE_SYS_DELAY,     ///< Network delay configuration
        TALKIE_SYS_SOCKET,    ///< List Socket class names
        TALKIE_SYS_MANIFESTO  ///< Show the Manifesto class name
    };


    /**
     * @enum RogerValue
     * @brief Standardized response codes (acknowledgments)
     * 
     * Predefined responses following typical radio procedure
     * for clear acknowledgment and status reporting.
     */
    enum RogerValue : uint8_t {
        TALKIE_RGR_ROGER,      ///< Call positively processed
        TALKIE_RGR_NEGATIVE,   ///< Call denied/cannot comply
        TALKIE_RGR_SAY_AGAIN,  ///< No matching Action available for the call
        TALKIE_RGR_NIL,        ///< Empty content, nothing to be processed
        TALKIE_RGR_NO_JOY      ///< No processing due to lack of implementations
    };


    /**
     * @enum ErrorValue
     * @brief Specific error conditions in communication
     * 
     * Detailed error codes for diagnosing communication
     * failures and malformed messages.
     */
    enum ErrorValue : uint8_t {
        TALKIE_ERR_UNDEFINED, ///< Unspecified/generic error
        TALKIE_ERR_CHECKSUM,  ///< Message checksum failure
        TALKIE_ERR_MESSAGE,   ///< Malformed message structure
        TALKIE_ERR_IDENTITY,  ///< Invalid sender/receiver identity
        TALKIE_ERR_FIELD,     ///< Missing or invalid field
        TALKIE_ERR_FROM,      ///< Invalid source specification
        TALKIE_ERR_TO,        ///< Invalid destination specification
        TALKIE_ERR_DELAY,     ///< Timing/delay violation
        TALKIE_ERR_KEY,       ///< Invalid message key
        TALKIE_ERR_VALUE      ///< Invalid message value
    };
};


#endif // TALKIE_CODES_HPP
