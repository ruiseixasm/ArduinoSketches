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
 * @file MessageRepeater.hpp
 * @brief This is the central class by which all JsonMessages are routed
 *        between Talkers and Sockets.
 * 
 * @author Rui Seixas Monteiro
 * @date Created: 2026-01-03
 * @version 1.0.0
 */

#ifndef MESSAGE_REPEATER_HPP
#define MESSAGE_REPEATER_HPP

#include <Arduino.h>        // Needed for Serial given that Arduino IDE only includes Serial in .ino files!
#include "BroadcastSocket.h"
#include "JsonTalker.h"

// #define MESSAGE_REPEATER_DEBUG

using LinkType			= TalkieCodes::LinkType;
using TalkerMatch 		= TalkieCodes::TalkerMatch;
using BroadcastValue 	= TalkieCodes::BroadcastValue;
using MessageValue 		= TalkieCodes::MessageValue;
using SystemValue 		= TalkieCodes::SystemValue;
using RogerValue 		= TalkieCodes::RogerValue;
using ErrorValue 		= TalkieCodes::ErrorValue;
using ValueType 		= TalkieCodes::ValueType;
using Original 			= JsonMessage::Original;


/**
 * @class MessageRepeater
 * @brief An Interface to be implemented as a Manifesto to define the Talker actions
 * 
 * The Repeater works in similar fashion as an HAM radio repeater on the top of a mountain,
 * with a clear distinction of Uplinked and Downlinked communications, where the Uplinked nodes
 * are considered remote nodes and the downlinked nodes are considered local nodes.
 */
class MessageRepeater {
private:

	BroadcastSocket* const* const _uplinked_sockets;
	const uint8_t _uplinked_sockets_count;
	JsonTalker* const* const _downlinked_talkers;
	const uint8_t _downlinked_talkers_count;
	BroadcastSocket* const* const _downlinked_sockets;
	const uint8_t _downlinked_sockets_count;
	JsonTalker* const* const _uplinked_talkers;
	const uint8_t _uplinked_talkers_count;
	

public:

    // Constructor
    MessageRepeater(
			BroadcastSocket* const* const uplinked_sockets, uint8_t uplinked_sockets_count,
			JsonTalker* const* const downlinked_talkers, uint8_t downlinked_talkers_count,
			BroadcastSocket* const* const downlinked_sockets = nullptr, uint8_t downlinked_sockets_count = 0,
			JsonTalker* const* const uplinked_talkers = nullptr, uint8_t uplinked_talkers_count = 0
		)
        : _uplinked_sockets(uplinked_sockets), _uplinked_sockets_count(uplinked_sockets_count),
        _downlinked_talkers(downlinked_talkers), _downlinked_talkers_count(downlinked_talkers_count),
        _downlinked_sockets(downlinked_sockets), _downlinked_sockets_count(downlinked_sockets_count),
        _uplinked_talkers(uplinked_talkers), _uplinked_talkers_count(uplinked_talkers_count)
    {
		for (uint8_t socket_j = 0; socket_j < _uplinked_sockets_count; ++socket_j) {
			_uplinked_sockets[socket_j]->_setLink(this, LinkType::TALKIE_LT_UP_LINKED);
		}
		for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
			_downlinked_talkers[talker_i]->_setLink(this, LinkType::TALKIE_LT_DOWN_LINKED);
		}
		for (uint8_t socket_j = 0; socket_j < _downlinked_sockets_count; ++socket_j) {
			_downlinked_sockets[socket_j]->_setLink(this, LinkType::TALKIE_LT_DOWN_LINKED);
		}
		for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count; ++talker_i) {
			_uplinked_talkers[talker_i]->_setLink(this, LinkType::TALKIE_LT_UP_LINKED);
		}
	}

	~MessageRepeater() {
		// Does nothing
	}


	/**
	 * @brief Method intended to be called from the Arduino sketch `loop()` function.
	 *
	 * @note This method should be called regularly from the sketch `loop()` function.
	 * Example:
	 * ```
	 * void loop() {
	 *     // Other needed calls here
	 *     message_repeater.loop();
	 * }
	 * ```
	 */
    void loop() {
		for (uint8_t socket_j = 0; socket_j < _uplinked_sockets_count; ++socket_j) {
			_uplinked_sockets[socket_j]->_loop();
		}
		for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
			_downlinked_talkers[talker_i]->_loop();
		}
		for (uint8_t socket_j = 0; socket_j < _downlinked_sockets_count; ++socket_j) {
			_downlinked_sockets[socket_j]->_loop();
		}
		for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count; ++talker_i) {
			_uplinked_talkers[talker_i]->_loop();
		}
    }


	/**
     * @brief Method intended to be called directly inside a sketch
	 *        without the need of using a Talker
     * @param message A json message to be transmitted
	 * 
     * @note Transmits a downlink message to the Repeater.
     */
	bool downlinkMessage(const JsonMessage &message) {
		JsonTalker dummy_talker = JsonTalker("", "", nullptr);
		JsonMessage message_copy(message);
		message_copy.set_from_name("");
		message_copy.set_identity();
		message_copy.set_no_reply();
		return _talkerDownlink(dummy_talker, message_copy);
	}


	/**
     * @brief Method intended to be called directly inside a sketch
	 *        without the need of using a Talker
     * @param message A json message to be transmitted
	 * 
     * @note Transmits a uplink message to the Repeater.
     */
	bool uplinkMessage(const JsonMessage &message) {
		JsonTalker dummy_talker = JsonTalker("", "", nullptr);
		JsonMessage message_copy(message);
		message_copy.set_from_name("");
		message_copy.set_identity();
		message_copy.set_no_reply();
		return _talkerUplink(dummy_talker, message_copy);
	}


	/**
     * @brief Returns the amount of uplinked sockets
	 * 
     * @note This is intended to be called internally and not by the user code.
     */
	uint8_t _uplinkedSocketsCount() const {
		return _uplinked_sockets_count;
	}


	/**
     * @brief Returns the amount of downlinked sockets
	 * 
     * @note This is intended to be called internally and not by the user code.
     */
	uint8_t _downlinkedSocketsCount() const {
		return _downlinked_sockets_count;
	}

	
	/**
     * @brief Returns the uplinked socked selected via its index
     * @param socket_index The index of the socket
	 * 
     * @note This is intended to be called internally and not by the user code.
     */
	BroadcastSocket* _getUplinkedSocket(uint8_t socket_index) const {
        if (socket_index < _uplinked_sockets_count) {
            return _uplinked_sockets[socket_index];
        }
		return nullptr;
	}
	

	/**
     * @brief Returns the downlinked socked selected via its index
     * @param socket_index The index of the socket
	 * 
     * @note This is intended to be called internally and not by the user code.
     */
	BroadcastSocket* _getDownlinkedSocket(uint8_t socket_index) const {
        if (socket_index < _downlinked_sockets_count) {
            return _downlinked_sockets[socket_index];
        }
		return nullptr;
	}


	/**
     * @brief Transmits to the Repeater downlink a json message
     * @param socket The socket that is calling the method
     * @param message A json message to be transmitted
	 * 
     * @note This is intended to be called internally and not by the user code.
     */
	void _socketDownlink(BroadcastSocket &socket, JsonMessage &message) {
		BroadcastValue broadcast = message.get_broadcast_value();
		TalkerMatch talker_match = message.get_talker_match();

		#ifdef MESSAGE_REPEATER_DEBUG
		Serial.print(F("\t\t_socketDownlink1: "));
		message.write_to(Serial);
		Serial.print(" | ");
		Serial.print((int)broadcast);
		Serial.print(" | ");
		Serial.println((int)talker_match);
		#endif

		// To downlinked nodes (BRIDGED uplinks process LOCAL messages too)
		if (broadcast == BroadcastValue::TALKIE_BC_REMOTE || (broadcast == BroadcastValue::TALKIE_BC_LOCAL && socket.getLinkType() == LinkType::TALKIE_LT_UP_BRIDGED)) {
			switch (talker_match) {

				case TalkerMatch::TALKIE_MATCH_ANY:
				{
					for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count;) {
						JsonMessage message_copy(message);
						_downlinked_talkers[talker_i++]->_handleTransmission(message_copy, talker_match);
					}
				}
				break;
				
				case TalkerMatch::TALKIE_MATCH_BY_CHANNEL:
				{
					uint8_t message_channel = message.get_to_channel();
					for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
						uint8_t talker_channel = _downlinked_talkers[talker_i]->get_channel();
						if (talker_channel == message_channel) {
							JsonMessage message_copy(message);
							_downlinked_talkers[talker_i]->_handleTransmission(message_copy, talker_match);
						}
					}
				}
				break;
				
				case TalkerMatch::TALKIE_MATCH_BY_NAME:
				
				#ifdef MESSAGE_DEBUG_TIMING
				Serial.print(" | ");
				Serial.print(millis() - message._reference_time);
				#endif
				
				{
					char message_to_name[TALKIE_NAME_LEN];
					strcpy(message_to_name, message.get_to_name());
					for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
						const char* talker_name = _downlinked_talkers[talker_i]->get_name();
						if (strcmp(talker_name, message_to_name) == 0) {
							_downlinked_talkers[talker_i]->_handleTransmission(message, talker_match);
							return;
						}
					}
				}
				break;

				default: return;
			}
			
			#ifdef MESSAGE_DEBUG_TIMING
			Serial.print(" | ");
			Serial.print(millis() - message._reference_time);
			#endif
				
			for (uint8_t socket_j = 0; socket_j < _downlinked_sockets_count; ++socket_j) {
				// Sockets ONLY manipulate the checksum ('c')
				_downlinked_sockets[socket_j]->_finishTransmission(message);
			}
			
			#ifdef MESSAGE_DEBUG_TIMING
			Serial.print(" | ");
			Serial.print(millis() - message._reference_time);
			#endif
		}
	}

	
	/**
     * @brief Transmits to the Repeater uplink a json message
     * @param talker The talker that is calling the method
     * @param message A json message to be transmitted
	 * 
     * @note This is intended to be called internally and not by the user code.
     */
	bool _talkerUplink(JsonTalker &talker, JsonMessage &message) {

		BroadcastValue broadcast = message.get_broadcast_value();

		#ifdef MESSAGE_REPEATER_DEBUG
		Serial.print(F("\t\t_talkerUplink1: "));
		message.write_to(Serial);
		Serial.print(" | ");
		Serial.println((int)broadcast);
		#endif

		switch (broadcast) {

			case BroadcastValue::TALKIE_BC_REMOTE:		// To uplinked nodes
			{
				TalkerMatch talker_match = message.get_talker_match();

				switch (talker_match) {

					case TalkerMatch::TALKIE_MATCH_ANY:
					{
						for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count;) {
							JsonMessage message_copy(message);
							_uplinked_talkers[talker_i++]->_handleTransmission(message_copy, talker_match);
						}
					}
					break;
					
					case TalkerMatch::TALKIE_MATCH_BY_CHANNEL:
					{
						uint8_t message_channel = message.get_to_channel();
						for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count; ++talker_i) {
							uint8_t talker_channel = _uplinked_talkers[talker_i]->get_channel();
							if (talker_channel == message_channel) {
								JsonMessage message_copy(message);
								_uplinked_talkers[talker_i]->_handleTransmission(message_copy, talker_match);
							}
						}
					}
					break;
					
					case TalkerMatch::TALKIE_MATCH_BY_NAME:
					{
						char message_to_name[TALKIE_NAME_LEN];
						strcpy(message_to_name, message.get_to_name());
						for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count; ++talker_i) {
							const char* talker_name = _uplinked_talkers[talker_i]->get_name();
							if (strcmp(talker_name, message_to_name) == 0) {
								_uplinked_talkers[talker_i]->_handleTransmission(message, talker_match);
								return true;
							}
						}
					}
					break;

					case TalkerMatch::TALKIE_MATCH_NONE: return true;
					default: return false;
				}
				bool no_fails = true;
				for (uint8_t socket_j = 0; socket_j < _uplinked_sockets_count; ++socket_j) {
					// Sockets ONLY manipulate the checksum ('c')
					if (!_uplinked_sockets[socket_j]->_finishTransmission(message)) {
						no_fails = false;
					}
				}
				return no_fails;
			}
			break;
			
			case BroadcastValue::TALKIE_BC_LOCAL:		// To downlinked nodes
			{
				if (_downlinked_talkers_count) {
					TalkerMatch talker_match = message.get_talker_match();

					switch (talker_match) {

						case TalkerMatch::TALKIE_MATCH_ANY:
						{
							for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
								if (_downlinked_talkers[talker_i] != &talker) {
									JsonMessage message_copy(message);
									_downlinked_talkers[talker_i]->_handleTransmission(message_copy, talker_match);
								}
							}
						}
						break;
						
						case TalkerMatch::TALKIE_MATCH_BY_CHANNEL:
						{
							uint8_t message_channel = message.get_to_channel();
							for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
								if (_downlinked_talkers[talker_i] != &talker) {
									uint8_t talker_channel = _downlinked_talkers[talker_i]->get_channel();
									if (talker_channel == message_channel) {
										JsonMessage message_copy(message);
										_downlinked_talkers[talker_i]->_handleTransmission(message_copy, talker_match);
									}
								}
							}
						}
						break;
						
						case TalkerMatch::TALKIE_MATCH_BY_NAME:
						{
							char message_to_name[TALKIE_NAME_LEN];
							strcpy(message_to_name, message.get_to_name());
							for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
								if (_downlinked_talkers[talker_i] != &talker) {
									const char* talker_name = _downlinked_talkers[talker_i]->get_name();
									if (strcmp(talker_name, message_to_name) == 0) {
										_downlinked_talkers[talker_i]->_handleTransmission(message, talker_match);
										return true;
									}
								}
							}
						}
						break;
						
						case TalkerMatch::TALKIE_MATCH_NONE: return true;
						default: return false;
					}
					bool no_fails = true;
					for (uint8_t socket_j = 0; socket_j < _downlinked_sockets_count; ++socket_j) {
						// Sockets ONLY manipulate the checksum ('c')
						if (!_downlinked_sockets[socket_j]->_finishTransmission(message)) {
							no_fails = false;
						}
					}
					for (uint8_t socket_j = 0; socket_j < _uplinked_sockets_count; ++socket_j) {
						if (_uplinked_sockets[socket_j]->getLinkType() == LinkType::TALKIE_LT_UP_BRIDGED) {
							// Sockets ONLY manipulate the checksum ('c')
							if (!_uplinked_sockets[socket_j]->_finishTransmission(message)) {
								no_fails = false;
							}
						}
					}
					return no_fails;
				} else {
					bool no_fails = true;
					for (uint8_t socket_j = 0; socket_j < _downlinked_sockets_count; ++socket_j) {
						// Sockets ONLY manipulate the checksum ('c')
						if (!_downlinked_sockets[socket_j]->_finishTransmission(message)) {
							no_fails = false;
						}
					}
					for (uint8_t socket_j = 0; socket_j < _uplinked_sockets_count; ++socket_j) {
						if (_uplinked_sockets[socket_j]->getLinkType() == LinkType::TALKIE_LT_UP_BRIDGED) {
							// Sockets ONLY manipulate the checksum ('c')
							if (!_uplinked_sockets[socket_j]->_finishTransmission(message)) {
								no_fails = false;
							}
						}
					}
					return no_fails;
				}
			}
			break;
			
			case BroadcastValue::TALKIE_BC_SELF:
			{
				TalkerMatch talker_match = message.get_talker_match();

				switch (talker_match) {

					case TalkerMatch::TALKIE_MATCH_ANY:
					{
						talker._handleTransmission(message, talker_match);
						return true;
					}
					break;
					
					case TalkerMatch::TALKIE_MATCH_BY_CHANNEL:
					{
						uint8_t message_channel = message.get_to_channel();
						uint8_t talker_channel = talker.get_channel();
						if (talker_channel == message_channel) {
							talker._handleTransmission(message, talker_match);
							return true;
						}
					}
					break;
					
					case TalkerMatch::TALKIE_MATCH_BY_NAME:
					{
						char message_to_name[TALKIE_NAME_LEN];
						strcpy(message_to_name, message.get_to_name());
						
						const char* talker_name = talker.get_name();
						if (strcmp(talker_name, message_to_name) == 0) {
							talker._handleTransmission(message, talker_match);
							return true;
						}
					}
					break;
					
					case TalkerMatch::TALKIE_MATCH_NONE: return true;
					default: return false;
				}
			}
			break;
			
			case BroadcastValue::TALKIE_BC_NONE: return true;
			
			default: break;
		}
		return false;
	}


	/**
     * @brief Transmits to the Repeater downlink a json message
     * @param socket The socket that is calling the method
     * @param message A json message to be transmitted
	 * 
     * @note This is intended to be called internally and not by the user code.
     */
	void _socketUplink(BroadcastSocket &socket, JsonMessage &message) {
		BroadcastValue broadcast = message.get_broadcast_value();
		TalkerMatch talker_match = message.get_talker_match();

		#ifdef MESSAGE_REPEATER_DEBUG
		Serial.print(F("\t\t_socketUplink1: "));
		message.write_to(Serial);
		Serial.print(" | ");
		Serial.println((int)broadcast);
		#endif

		switch (broadcast) {

			case BroadcastValue::TALKIE_BC_REMOTE:		// To uplinked nodes
			{
				switch (talker_match) {

					case TalkerMatch::TALKIE_MATCH_ANY:
					{
						for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count;) {
							JsonMessage message_copy(message);
							_uplinked_talkers[talker_i++]->_handleTransmission(message_copy, talker_match);
						}
					}
					break;
					
					case TalkerMatch::TALKIE_MATCH_BY_CHANNEL:
					{
						uint8_t message_channel = message.get_to_channel();
						for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count; ++talker_i) {
							uint8_t talker_channel = _uplinked_talkers[talker_i]->get_channel();
							if (talker_channel == message_channel) {
								JsonMessage message_copy(message);
								_uplinked_talkers[talker_i]->_handleTransmission(message_copy, talker_match);
							}
						}
					}
					break;
					
					case TalkerMatch::TALKIE_MATCH_BY_NAME:
					
					#ifdef MESSAGE_DEBUG_TIMING
					Serial.print(" | ");
					Serial.print(millis() - message._reference_time);
					#endif
				
					{
						char message_to_name[TALKIE_NAME_LEN];
						strcpy(message_to_name, message.get_to_name());
						for (uint8_t talker_i = 0; talker_i < _uplinked_talkers_count; ++talker_i) {
							const char* talker_name = _uplinked_talkers[talker_i]->get_name();
							if (strcmp(talker_name, message_to_name) == 0) {
								_uplinked_talkers[talker_i]->_handleTransmission(message, talker_match);
								return;
							}
						}
					}
					break;
					
					default: return;
				}
				
				#ifdef MESSAGE_DEBUG_TIMING
				Serial.print(" | ");
				Serial.print(millis() - message._reference_time);
				#endif
				
				for (uint8_t socket_j = 0; socket_j < _uplinked_sockets_count; ++socket_j) {
					// Sockets ONLY manipulate the checksum ('c')
					_uplinked_sockets[socket_j]->_finishTransmission(message);
				}
				
				#ifdef MESSAGE_DEBUG_TIMING
				Serial.print(" | ");
				Serial.print(millis() - message._reference_time);
				#endif
			}
			break;
			
			case BroadcastValue::TALKIE_BC_LOCAL:		// To downlinked nodes
			{

				switch (talker_match) {

					case TalkerMatch::TALKIE_MATCH_ANY:
					{
						for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count;) {
							JsonMessage message_copy(message);
							_downlinked_talkers[talker_i++]->_handleTransmission(message_copy, talker_match);
						}
					}
					break;
					
					case TalkerMatch::TALKIE_MATCH_BY_CHANNEL:
					{
						uint8_t message_channel = message.get_to_channel();
						for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
							uint8_t talker_channel = _downlinked_talkers[talker_i]->get_channel();
							if (talker_channel == message_channel) {
								JsonMessage message_copy(message);
								_downlinked_talkers[talker_i]->_handleTransmission(message_copy, talker_match);
							}
						}
					}
					break;
					
					case TalkerMatch::TALKIE_MATCH_BY_NAME:
					{
						char message_to_name[TALKIE_NAME_LEN];
						strcpy(message_to_name, message.get_to_name());
						for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
							const char* talker_name = _downlinked_talkers[talker_i]->get_name();
							if (strcmp(talker_name, message_to_name) == 0) {
								_downlinked_talkers[talker_i]->_handleTransmission(message, talker_match);
								return;
							}
						}
					}
					break;
					
					default: return;
				}
				for (uint8_t socket_j = 0; socket_j < _downlinked_sockets_count; ++socket_j) {
					if (_downlinked_sockets[socket_j] != &socket) {	// Shouldn't locally Uplink to itself
						// Sockets ONLY manipulate the checksum ('c')
						_downlinked_sockets[socket_j]->_finishTransmission(message);
					}
				}
				for (uint8_t socket_j = 0; socket_j < _uplinked_sockets_count; ++socket_j) {
					if (_uplinked_sockets[socket_j]->getLinkType() == LinkType::TALKIE_LT_UP_BRIDGED) {
						// Sockets ONLY manipulate the checksum ('c')
						_uplinked_sockets[socket_j]->_finishTransmission(message);
					}
				}
			}
			break;

			default: break;
		}
	}


	/**
     * @brief Transmits to the Repeater downlink a json message
     * @param talker The talker that is calling the method
     * @param message A json message to be transmitted
	 * 
     * @note This is intended to be called internally and not by the user code.
     */
	bool _talkerDownlink(JsonTalker &talker, JsonMessage &message) {

		BroadcastValue broadcast = message.get_broadcast_value();

		#ifdef MESSAGE_REPEATER_DEBUG
		Serial.print(F("\t\t_talkerDownlink1: "));
		message.write_to(Serial);
		Serial.print(" | ");
		Serial.println((int)broadcast);
		#endif

		switch (broadcast) {
			
			// A Talker is always a local talker, so, it's an implicit bridge
			case BroadcastValue::TALKIE_BC_REMOTE:
			case BroadcastValue::TALKIE_BC_LOCAL:
			{
				TalkerMatch talker_match = message.get_talker_match();

				#ifdef MESSAGE_REPEATER_DEBUG
				Serial.print(F("\t\t\t_talkerDownlink2: "));
				message.write_to(Serial);
				Serial.print(" | ");
				Serial.println((int)talker_match);
				#endif

				switch (talker_match) {

					case TalkerMatch::TALKIE_MATCH_ANY:
					{
						for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count;) {
							JsonMessage message_copy(message);
							_downlinked_talkers[talker_i++]->_handleTransmission(message_copy, talker_match);
						}
					}
					break;
					
					case TalkerMatch::TALKIE_MATCH_BY_CHANNEL:
					{
						uint8_t message_channel = message.get_to_channel();
						for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
							uint8_t talker_channel = _downlinked_talkers[talker_i]->get_channel();
							if (talker_channel == message_channel) {
								JsonMessage message_copy(message);
								_downlinked_talkers[talker_i]->_handleTransmission(message_copy, talker_match);
							}
						}
					}
					break;
					
					case TalkerMatch::TALKIE_MATCH_BY_NAME:
					{
						char message_to_name[TALKIE_NAME_LEN];
						strcpy(message_to_name, message.get_to_name());
						for (uint8_t talker_i = 0; talker_i < _downlinked_talkers_count; ++talker_i) {
							const char* talker_name = _downlinked_talkers[talker_i]->get_name();
							if (strcmp(talker_name, message_to_name) == 0) {
								_downlinked_talkers[talker_i]->_handleTransmission(message, talker_match);
								return true;
							}
						}
					}
					break;
					
					case TalkerMatch::TALKIE_MATCH_NONE: return true;
					default: return false;
				}
				bool no_fails = true;
				for (uint8_t socket_j = 0; socket_j < _downlinked_sockets_count; ++socket_j) {
					// Sockets ONLY manipulate the checksum ('c')
					if (!_downlinked_sockets[socket_j]->_finishTransmission(message)) {
						no_fails = false;
					}
				}
				return no_fails;
			}
			break;
			
			case BroadcastValue::TALKIE_BC_SELF:
			{
				TalkerMatch talker_match = message.get_talker_match();

				switch (talker_match) {

					case TalkerMatch::TALKIE_MATCH_ANY:
					{
						talker._handleTransmission(message, talker_match);
						return true;
					}
					break;
					
					case TalkerMatch::TALKIE_MATCH_BY_CHANNEL:
					{
						uint8_t message_channel = message.get_to_channel();
						uint8_t talker_channel = talker.get_channel();
						if (talker_channel == message_channel) {
							talker._handleTransmission(message, talker_match);
							return true;
						}
					}
					break;
					
					case TalkerMatch::TALKIE_MATCH_BY_NAME:
					{
						char message_to_name[TALKIE_NAME_LEN];
						strcpy(message_to_name, message.get_to_name());
						
						const char* talker_name = talker.get_name();
						if (strcmp(talker_name, message_to_name) == 0) {
							talker._handleTransmission(message, talker_match);
							return true;
						}
					}
					break;
					
					case TalkerMatch::TALKIE_MATCH_NONE: return true;
					default: return false;
				}
			}
			break;
			
			case BroadcastValue::TALKIE_BC_NONE: return true;
			default: break;
		}
		return false;
	}

};


#endif // MESSAGE_REPEATER_HPP
