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
#include "TalkieCodes.hpp"
#include "JsonMessage.hpp"

using LinkType			= TalkieCodes::LinkType;
using TalkerMatch 		= TalkieCodes::TalkerMatch;
using BroadcastValue 	= TalkieCodes::BroadcastValue;
using MessageValue 		= TalkieCodes::MessageValue;
using SystemValue 		= TalkieCodes::SystemValue;
using RogerValue 		= TalkieCodes::RogerValue;
using ErrorValue 		= TalkieCodes::ErrorValue;
using ValueType 		= TalkieCodes::ValueType;
using Original 			= JsonMessage::Original;

class JsonTalker;

class TalkerManifesto {

public:

	// The subclass must have the class name defined (pure virtual)
    virtual const char* class_name() const = 0;

    TalkerManifesto(const TalkerManifesto&) = delete;
    TalkerManifesto& operator=(const TalkerManifesto&) = delete;
    TalkerManifesto(TalkerManifesto&&) = delete;
    TalkerManifesto& operator=(TalkerManifesto&&) = delete;
    
    TalkerManifesto() = default;
    virtual ~TalkerManifesto() = default;


    struct Action {
        const char* name;
        const char* desc;
    };


protected:
	
    // Iterator states
    uint8_t actionsIterIdx = 0;

    virtual const Action* _getActionsArray() const = 0;
    // Size methods
    virtual uint8_t _actionsCount() const = 0;


public:

	/**
     * @brief Method intended to be called by the Repeater class by its public loop method
     * @param talker Allows the access by the Manifesto to its owner Talker class
	 * 
     * @note This method being underscored means to be called internally only.
     */
    virtual void _loop(JsonTalker* talker) {
        (void)talker;		// Silence unused parameter warning
	}


	/** @brief Resets the index of the actions list */
	virtual void _iterateActionsReset() {
		actionsIterIdx = 0;
	}


    /**
     * @brief Returns each Action for each while iteration
     * @return A pointer to an Action of the Manifesto
     */
    virtual const Action* _iterateActionNext() {
        if (actionsIterIdx < _actionsCount()) {
            return &_getActionsArray()[actionsIterIdx++];
        }
        return nullptr;
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

	
    // Action implementations - MUST be implemented by derived
    virtual bool _actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) {
        (void)index;		// Silence unused parameter warning
        (void)talker;		// Silence unused parameter warning
        (void)json_message;	// Silence unused parameter warning
        (void)talker_match;	// Silence unused parameter warning
        return false;
	}


    virtual void _echo(JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) {
        (void)talker;		// Silence unused parameter warning
        (void)json_message;	// Silence unused parameter warning
        (void)talker_match;	// Silence unused parameter warning
    }


    virtual void _error(JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) {
        (void)talker;		// Silence unused parameter warning
        (void)json_message;	// Silence unused parameter warning
        (void)talker_match;	// Silence unused parameter warning
    }

    virtual void _noise(JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) {
        (void)talker;		// Silence unused parameter warning
        (void)json_message;	// Silence unused parameter warning
        (void)talker_match;	// Silence unused parameter warning
    }

};


#endif // I_MANIFESTO_HPP
