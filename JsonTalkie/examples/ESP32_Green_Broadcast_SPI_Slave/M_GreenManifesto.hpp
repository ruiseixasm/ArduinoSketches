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
#ifndef GREEN_MANIFESTO_HPP
#define GREEN_MANIFESTO_HPP

#include <TalkerManifesto.hpp>

// #define GREEN_TALKER_DEBUG


class M_GreenManifesto : public TalkerManifesto {
public:

	// The Manifesto class description shouldn't be greater than 42 chars
	// {"m":7,"f":"","s":1,"b":1,"t":"","i":58485,"0":"","1":1,"c":11266} <-- 128 - (66 + 2*10) = 42
    const char* class_description() const override { return "GreenManifesto"; }

    M_GreenManifesto() : TalkerManifesto() {}	// Constructor

protected:

	// The Action pair name and description shouldn't be greater than 32 chars
	// {"m":7,"b":1,"i":6442,"f":"","t":"","0":255,"1":"","2":"","c":25870} <-- 128 - (68 + 2*15) = 30

    Action calls[6] = {
		{"on", "Turns led ON"},
		{"off", "Turns led OFF"},
		{"state", "The actual state of the led"},
		{"bpm_10", "Sets the Tempo in BPM x 10"},
		{"bpm_10", "Gets the Tempo in BPM x 10"},
		{"toggle", "Toggles 'blue' led on and off"}
    };
    
    bool _is_led_on = false;  // keep track of state yourself, by default it's off
    uint16_t _bpm_10 = 1200;
    uint16_t _total_calls = 0;
	uint8_t _blue_led_on = 0;


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
				#ifdef GREEN_LED
					#ifdef GREEN_MANIFESTO_DEBUG
						Serial.print(F("\tGREEN_LED IS DEFINED as: "));
						Serial.println(GREEN_LED);
					#endif
					digitalWrite(GREEN_LED, HIGH);
				#else
					#ifdef GREEN_MANIFESTO_DEBUG
						Serial.println(F("\tGREEN_LED IS NOT DEFINED in this context!"));
					#endif
				#endif
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
				#ifdef GREEN_MANIFESTO_DEBUG
				Serial.println(F("\tCase 1 - Turning LED OFF"));
				#endif
		
				if (_is_led_on) {
				#ifdef GREEN_LED
					digitalWrite(GREEN_LED, LOW);
				#endif
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
				json_message.set_nth_value_number(0, (uint32_t)_is_led_on);
                return true;
            break;
			
            case 3:
                _bpm_10 = json_message.get_nth_value_number(0);
                return true;
                break;
				
            case 4:
				json_message.set_nth_value_number(0, _bpm_10);
				return true;

            case 5:
			{
				JsonMessage toggle_blue_on_off(MessageValue::TALKIE_MSG_CALL, BroadcastValue::TALKIE_BC_LOCAL);
				toggle_blue_on_off.set_to_name("blue");
				if (_blue_led_on++ % 2) {
					toggle_blue_on_off.set_action_name("off");
				} else {
					toggle_blue_on_off.set_action_name("on");
				}
                return talker.transmitToRepeater(toggle_blue_on_off);
			}
            break;
			
			default: break;
		}
		return false;
	}
    
};


#endif // GREEN_MANIFESTO_HPP
