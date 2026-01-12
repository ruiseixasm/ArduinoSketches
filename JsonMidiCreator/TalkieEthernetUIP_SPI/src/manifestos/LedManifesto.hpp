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
#ifndef BLUE_MANIFESTO_HPP
#define BLUE_MANIFESTO_HPP

#include "../TalkerManifesto.hpp"

// #define BLUE_MANIFESTO_DEBUG


class LedManifesto : public TalkerManifesto {
public:

    const char* class_name() const override { return "LedManifesto"; }

    LedManifesto(uint8_t led_pin) : TalkerManifesto(), _led_pin(led_pin)
	{
		pinMode(_led_pin, OUTPUT);
	}	// Constructor

    ~LedManifesto()
	{	// ~TalkerManifesto() called automatically here
		digitalWrite(_led_pin, LOW);
		pinMode(_led_pin, INPUT);
	}	// Destructor


protected:

	const uint8_t _led_pin;
    bool _is_led_on = false;	// keep track of the led state, by default it's off
    uint16_t _total_calls = 0;

    Action calls[3] = {
		{"on", "Turns led ON"},
		{"off", "Turns led OFF"},
		{"actions", "Returns the number of triggered Actions"}
    };
    
public:
    
    const Action* _getActionsArray() const override { return calls; }
    uint8_t _actionsCount() const override { return sizeof(calls)/sizeof(Action); }


    // Index-based operations (simplified examples)
    bool _actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) override {
        (void)talker;		// Silence unused parameter warning
    	(void)talker_match;	// Silence unused parameter warning
		
		if (index >= sizeof(calls)/sizeof(Action)) return false;
		
		// Actual implementation would do something based on index
		switch(index) {

			case 0:
			{
				#ifdef BLUE_MANIFESTO_DEBUG
				Serial.println(F("\tCase 0 - Turning LED ON"));
				#endif
		
				if (!_is_led_on) {
					digitalWrite(_led_pin, HIGH);
					_is_led_on = true;
					_total_calls++;
					return true;
				} else {
					json_message.set_nth_value_string(0, "Already On!");
					return false;
				}
			}
			break;

			case 1:
			{
				#ifdef BLUE_MANIFESTO_DEBUG
				Serial.println(F("\tCase 1 - Turning LED OFF"));
				#endif
		
				if (_is_led_on) {
				digitalWrite(_led_pin, LOW);
					_is_led_on = false;
					_total_calls++;
				} else {
					json_message.set_nth_value_string(0, "Already Off!");
					return false;
				}
				return true;
			}
			break;
			
            case 2:
				json_message.set_nth_value_number(0, _total_calls);
                return true;
            break;
				
            default: return false;
		}
		return false;
	}
    
};


#endif // BLUE_MANIFESTO_HPP
