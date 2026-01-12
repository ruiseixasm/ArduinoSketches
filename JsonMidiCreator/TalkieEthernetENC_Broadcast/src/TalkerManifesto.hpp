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
 * @file TalkerManifesto.hpp
 * @brief This is an Interface for the class that defines exactly the
 *        activity of the Talker, namely to its named and numbered actions.
 * 
 * @author Rui Seixas Monteiro
 * @date Created: 2026-01-03
 * @version 1.0.0
 */

#ifndef I_MANIFESTO_HPP
#define I_MANIFESTO_HPP

#include <Arduino.h>
#include "JsonTalker.h"

using LinkType			= TalkieCodes::LinkType;
using TalkerMatch 		= TalkieCodes::TalkerMatch;
using BroadcastValue 	= TalkieCodes::BroadcastValue;
using MessageValue 		= TalkieCodes::MessageValue;
using SystemValue 		= TalkieCodes::SystemValue;
using RogerValue 		= TalkieCodes::RogerValue;
using ErrorValue 		= TalkieCodes::ErrorValue;
using ValueType 		= TalkieCodes::ValueType;
using Original 			= JsonMessage::Original;
using Action 			= JsonTalker::Action;

class JsonTalker;


/**
 * @class TalkerManifesto
 * @brief An Interface to be implemented as a Manifesto to define the Talker's actions
 *
 * The implementation of this class requires de definition of a list of actions like so:
 *     `Action calls[1] = {{"on", "Turns led ON"}};`
 *
 * @note Find `TalkerManifesto` implementation in https://github.com/ruiseixasm/JsonTalkie/tree/main/src/manifestos.
 */
class TalkerManifesto {

public:

	/** @brief A getter for the class name to be returned for the `system` command */
    virtual const char* class_name() const = 0;

    TalkerManifesto(const TalkerManifesto&) = delete;
    TalkerManifesto& operator=(const TalkerManifesto&) = delete;
    TalkerManifesto(TalkerManifesto&&) = delete;
    TalkerManifesto& operator=(TalkerManifesto&&) = delete;
    
    TalkerManifesto() = default;
    virtual ~TalkerManifesto() = default;


	/**
     * @brief Returns the total number of actions available to call
	 * 
	 * The typical method is:
	 * `uint8_t _actionsCount() const override { return sizeof(calls)/sizeof(Action); }`
     */
    virtual uint8_t _actionsCount() const = 0;


	/**
     * @brief A getter to the actions array defined in the interface implementation
	 * 
	 * The typical method is:
	 * `const Action* _getActionsArray() const override { return calls; }`
     */
    virtual const Action* _getActionsArray() const = 0;

	
	/**
     * @brief Method intended to be called by the Repeater class by its public loop method
     * @param talker Allows the access by the Manifesto to its owner Talker class
	 * 
     * @note This method being underscored means to be called internally only.
     */
    virtual void _loop(JsonTalker& talker) {
        (void)talker;		// Silence unused parameter warning
	}

	
    /**
     * @brief Returns the index Action for a given Action name
     * @param name The name of the Action
     * @return The index number of the action or 255 if none was found
     */
    virtual uint8_t _actionIndex(const char* name) const {
        for (uint8_t i = 0; i < _actionsCount(); i++) {
            if (strcmp(_getActionsArray()[i].name, name) == 0) {
                return i;
            }
        }
        return 255;
    }
    

    /**
     * @brief Confirms the index Action for a given index Action
     * @param index The index of the Action to be confirmed
     * @return The index number of the action or 255 if none exists
     */
    virtual uint8_t _actionIndex(uint8_t index) const {
        return (index < _actionsCount()) ? index : 255;
    }

	
    /**
     * @brief Calls a given Action by it's index number
     * @param index The index of the Action being called
     * @param talker Allows the access by the Manifesto to its owner Talker class
     * @param json_message The json message made available for manipulation
     * @param talker_match The type of matching concerning the Talker call
     * @return Returns true if the call if successful (roger) or false if not (negative)
     */
    virtual bool _actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) {
        (void)index;		// Silence unused parameter warning
        (void)talker;		// Silence unused parameter warning
        (void)json_message;	// Silence unused parameter warning
        (void)talker_match;	// Silence unused parameter warning
        return false;
	}

	
    /**
     * @brief The method that processes the received echoes of the messages sent
     * @param talker Allows the access by the Manifesto to its owner Talker class
     * @param json_message The json message made available for manipulation
     * @param talker_match The type of matching concerning the Talker call
	 * 
	 * This method is intended to process the echoes from the talker sent messages.
     */
    virtual void _echo(JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) {
        (void)talker;		// Silence unused parameter warning
        (void)json_message;	// Silence unused parameter warning
        (void)talker_match;	// Silence unused parameter warning
    }

	
    /**
     * @brief The method that processes the received errors of the messages sent
     * @param talker Allows the access by the Manifesto to its owner Talker class
     * @param json_message The json message made available for manipulation
     * @param talker_match The type of matching concerning the Talker call
	 * 
	 * This method is intended to process the errors from the talker sent messages.
     */
	virtual void _error(JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) {
        (void)talker;		// Silence unused parameter warning
        (void)json_message;	// Silence unused parameter warning
        (void)talker_match;	// Silence unused parameter warning
    }

	
    /**
     * @brief The method that processes the noisy messages received
     * @param talker Allows the access by the Manifesto to its owner Talker class
     * @param json_message The json message made available for manipulation
     * @param talker_match The type of matching concerning the Talker call
	 * 
     * @note This method excludes noisy messages associated to errors (with error field).
     */
    virtual void _noise(JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) {
        (void)talker;		// Silence unused parameter warning
        (void)json_message;	// Silence unused parameter warning
        (void)talker_match;	// Silence unused parameter warning
    }

};


#endif // I_MANIFESTO_HPP
