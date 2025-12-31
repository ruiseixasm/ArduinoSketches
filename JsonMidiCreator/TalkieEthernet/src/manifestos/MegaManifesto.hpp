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
#ifndef MEGA_MANIFESTO_HPP
#define MEGA_MANIFESTO_HPP

#include "../TalkerManifesto.hpp"

// #define MEGA_MANIFESTO_DEBUG


class MegaManifesto : public TalkerManifesto {
public:

    const char* class_name() const override { return "MegaManifesto"; }

    MegaManifesto() : TalkerManifesto() {}	// Constructor


protected:

    bool _is_led_on = false;  // keep track of state yourself, by default it's off
    uint16_t _bpm_10 = 1200;
    uint16_t _total_calls = 0;


    Action calls[3] = {
		{"on", "Turns led ON"},
		{"off", "Turns led OFF"},
		{"bpm_10", "Sets the Tempo in BPM x 10"}
    };
    
    const Action* getActionsArray() const override { return calls; }

    // Size methods
    uint8_t actionsCount() const override { return sizeof(calls)/sizeof(Action); }


public:
    
    // Index-based operations (simplified examples)
    bool actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& json_message) override {
        (void)talker;		// Silence unused parameter warning
		
		if (index >= sizeof(calls)/sizeof(Action)) return false;
		
		// Actual implementation would do something based on index
		switch(index) {

			case 0:
			{
				#ifdef MEGA_MANIFESTO_DEBUG
				Serial.println(F("\tCase 0 - Turning LED ON"));
				#endif
		
				if (!_is_led_on) {
				#ifdef LED_BUILTIN
					#ifdef MEGA_MANIFESTO_DEBUG
						Serial.print(F("\tLED_BUILTIN IS DEFINED as: "));
						Serial.println(LED_BUILTIN);
					#endif
					digitalWrite(LED_BUILTIN, HIGH);
				#else
					#ifdef MEGA_MANIFESTO_DEBUG
						Serial.println(F("\tLED_BUILTIN IS NOT DEFINED in this context!"));
					#endif
				#endif
					_is_led_on = true;
					_total_calls++;
					return true;
				} else {
					// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
					json_message.set_nth_value_string(0, "Already On!");
					return false;
				}
			}
			break;

			case 1:
			{
				#ifdef MEGA_MANIFESTO_DEBUG
				Serial.println(F("\tCase 1 - Turning LED OFF"));
				#endif
		
				if (_is_led_on) {
				#ifdef LED_BUILTIN
					digitalWrite(LED_BUILTIN, LOW);
				#endif
					_is_led_on = false;
					_total_calls++;
				} else {
					// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
					json_message.set_nth_value_string(0, "Already Off!");
					return false;
				}
				return true;
			}
			break;
			
            case 2:
				// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
                _bpm_10 = json_message.get_nth_value_number(0);
                return true;
                break;
				
            // case 3:
			// 	return _bpm_10;
		}
		return false;
	}
    
};


#endif // MEGA_MANIFESTO_HPP
