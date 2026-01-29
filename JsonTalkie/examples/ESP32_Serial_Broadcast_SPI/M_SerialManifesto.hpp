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
#ifndef SERIAL_MANIFESTO_HPP
#define SERIAL_MANIFESTO_HPP

#include <TalkerManifesto.hpp>

// #define GREEN_TALKER_DEBUG


class M_SerialManifesto : public TalkerManifesto {
public:

	// The Manifesto class description shouldn't be greater than 42 chars
	// {"m":7,"f":"","s":1,"b":1,"t":"","i":58485,"0":"","1":1,"c":11266} <-- 128 - (66 + 2*10) = 42
    const char* class_description() const override { return "SerialManifesto"; }

    M_SerialManifesto() : TalkerManifesto() {}	// Constructor


protected:

    bool _is_led_on = false;  // keep track of state yourself, by default it's off

    Action calls[3] = {
		{"on", "Turns led ON"},
		{"off", "Turns led OFF"},
		{"state", "The actual state of the led"}
    };
    
public:
    
    const Action* _getActionsArray() const override { return calls; }

    // Size methods
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
				#ifdef GREEN_MANIFESTO_DEBUG
				Serial.println(F("\tCase 0 - Turning LED ON"));
				#endif
		
				if (!_is_led_on) {
				#ifdef LED_BUILTIN
					#ifdef GREEN_MANIFESTO_DEBUG
						Serial.print(F("\tLED_BUILTIN IS DEFINED as: "));
						Serial.println(LED_BUILTIN);
					#endif
					digitalWrite(LED_BUILTIN, HIGH);
				#else
					#ifdef GREEN_MANIFESTO_DEBUG
						Serial.println(F("\tLED_BUILTIN IS NOT DEFINED in this context!"));
					#endif
				#endif
					_is_led_on = true;
					return true;
				} else {
					json_message.set_nth_value_string(0, "Already On!");
					return false;
				}
			}
			break;

			case 1:
			{
				#ifdef GREEN_MANIFESTO_DEBUG
				Serial.println(F("\tCase 1 - Turning LED OFF"));
				#endif
		
				if (_is_led_on) {
				#ifdef LED_BUILTIN
					digitalWrite(LED_BUILTIN, LOW);
				#endif
					_is_led_on = false;
				} else {
					json_message.set_nth_value_string(0, "Already Off!");
					return false;
				}
				return true;
			}
			break;
			
            case 2:
				json_message.set_nth_value_number(0, (uint32_t)_is_led_on);
                return true;
            break;
				
            default: return false;
		}
		return false;
	}
    
};


#endif // SERIAL_MANIFESTO_HPP
