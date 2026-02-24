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
#ifndef CYCLER_MANIFESTO_HPP
#define CYCLER_MANIFESTO_HPP

#include <TalkerManifesto.hpp>

// #define CYCLER_MANIFESTO_DEBUG


// LED_BUILTIN is already defined by ESP32 platform
// Typically GPIO2 for most ESP32 boards
#ifndef LED_BUILTIN
  #define LED_BUILTIN 2  // Fallback definition if not already defined
#endif


class M_CyclerManifesto : public TalkerManifesto {
public:

	// The Manifesto class description shouldn't be greater than 42 chars
	// {"m":7,"f":"","s":1,"b":1,"t":"","i":58485,"0":"","1":1,"c":11266} <-- 128 - (66 + 2*10) = 42
    const char* class_description() const override { return "CyclerManifesto"; }

    M_CyclerManifesto() : TalkerManifesto() {

		_toggle_yellow_on_off.set_to_name("slave");
	}	// Constructor

protected:

	uint16_t _self_blink_time = 0;

	uint32_t _last_blink = 0;
	uint8_t _blue_led_on = 0;
	uint32_t _cyclic_period_ms = 1000;
	bool _cyclic_transmission = true;	// true by default

	JsonMessage _toggle_yellow_on_off{
		MessageValue::TALKIE_MSG_CALL,
		BroadcastValue::TALKIE_BC_LOCAL
	};

    uint16_t _total_calls = 0;
    uint16_t _total_echoes = 0;

	uint8_t _burst_toggles = 0;
	uint32_t _burst_spacing_us = 0;
	uint32_t _last_burst_us = 0;

	// ALWAYS MAKE SURE THE DIMENSIONS OF THE ARRAYS BELOW ARE THE CORRECT!

    Action calls[7] = {
		{"period", "Sets cycle period milliseconds"},
		{"enable", "Enables 1sec cyclic transmission"},
		{"disable", "Disables 1sec cyclic transmission"},
		{"calls", "Gets total calls and their echoes"},
		{"reset", "Resets the totals counter"},
		{"burst", "Sends many messages at once"},
		{"spacing", "Burst spacing in microseconds"}
    };
    
public:

    const Action* _getActionsArray() const override { return calls; }

    // Size methods
    uint8_t _actionsCount() const override { return sizeof(calls)/sizeof(Action); }


	void _loop(JsonTalker& talker) override {
		
		if ((uint16_t)millis() - _self_blink_time > 50) {	// turns off for 50 milliseconds
			
			digitalWrite(LED_BUILTIN, HIGH);
		}

		if (millis() - _last_blink > _cyclic_period_ms) {
			_last_blink = millis();

			if (_blue_led_on++ % 2) {
				_toggle_yellow_on_off.set_action_name("off");
			} else {
				_toggle_yellow_on_off.set_action_name("on");
			}
			if (_cyclic_transmission) {
				talker.transmitToRepeater(_toggle_yellow_on_off);
				_total_calls++;
			}
		}

		if (_burst_toggles > 0 && micros() - _last_burst_us > _burst_spacing_us) {
			_burst_toggles--;
			
			if (_blue_led_on++ % 2) {
				_toggle_yellow_on_off.set_action_name("off");
			} else {
				_toggle_yellow_on_off.set_action_name("on");
			}
			talker.transmitToRepeater(_toggle_yellow_on_off);
			_total_calls++;
			_last_burst_us = micros();
		}
	}

    
    // Index-based operations (simplified examples)
    bool _actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) override {
        (void)talker;		// Silence unused parameter warning
    	(void)talker_match;	// Silence unused parameter warning

		// Actual implementation would do something based on index
		switch(index) {

			case 0:
				_cyclic_period_ms = json_message.get_nth_value_number(0);
				return true;
			break;
				
			case 1:
				_cyclic_transmission = true;
				return true;
			break;
				
			case 2:
				_cyclic_transmission = false;
				return true;
			break;
				
			case 3:
				json_message.set_nth_value_number(0, _total_calls);
				json_message.set_nth_value_number(1, _total_echoes);
				return true;
			break;
			
			case 4:
				_total_calls = 0;
				_total_echoes = 0;
				return true;
			break;
			
			case 5:
			{
				_burst_toggles = 20;
				return true;
			}
			break;
			
			case 6:
				_burst_spacing_us = json_message.get_nth_value_number(0);;
				return true;
			break;
				
			default: break;
		}
		return false;
	}


    void _echo(JsonTalker& talker, JsonMessage& json_message, MessageValue message_value, TalkerMatch talker_match) override {
		(void)talker;		// Silence unused parameter warning
        (void)message_value;	// Silence unused parameter warning
		(void)talker_match;	// Silence unused parameter warning

		digitalWrite(LED_BUILTIN, LOW);
		_self_blink_time = millis();
		_total_echoes++;
    }
};


#endif // CYCLER_MANIFESTO_HPP
