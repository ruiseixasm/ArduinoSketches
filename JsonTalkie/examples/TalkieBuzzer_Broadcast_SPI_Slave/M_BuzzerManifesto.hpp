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
#ifndef BUZZER_MANIFESTO_HPP
#define BUZZER_MANIFESTO_HPP

#include <TalkerManifesto.hpp>

// #define BUZZER_MANIFESTO_DEBUG

#define BUZZ_PIN 2	// External BUZZER pin


class M_BuzzerManifesto : public TalkerManifesto {
public:

	// The Manifesto class description shouldn't be greater than 42 chars
	// {"m":7,"f":"","s":1,"b":1,"t":"","i":58485,"0":"","1":1,"c":11266} <-- 128 - (66 + 2*10) = 42
    const char* class_description() const override { return "BuzzerManifesto"; }

    M_BuzzerManifesto() : TalkerManifesto() {}	// Constructor

protected:

    uint16_t _buzz_duration_ms = 100;
	uint16_t _buzz_start = 0;

	uint32_t _last_blink = 0;
	uint8_t _yellow_led_on = 0;
	bool _cyclic_transmission = true;	// true by default

	// ALWAYS MAKE SURE THE DIMENSIONS OF THE ARRAYS BELOW ARE THE CORRECT!

    Action calls[4] = {
		{"buzz", "Buzz for a while"},
		{"ms", "Gets and sets the buzzing duration"},
		{"enable", "Enables 1sec cyclic transmission"},
		{"disable", "Disables 1sec cyclic transmission"}
    };
    
public:

    const Action* _getActionsArray() const override { return calls; }

    // Size methods
    uint8_t _actionsCount() const override { return sizeof(calls)/sizeof(Action); }


	void _loop(JsonTalker& talker) override {
		
		if ((uint16_t)millis() - _buzz_start > _buzz_duration_ms) {
			#ifdef BUZZ_PIN
			digitalWrite(BUZZ_PIN, LOW);
			#endif
		}

		if (millis() - _last_blink > 1000) {
			_last_blink = millis();

			JsonMessage toggle_yellow_on_off(MessageValue::TALKIE_MSG_CALL, BroadcastValue::TALKIE_BC_LOCAL);
			toggle_yellow_on_off.set_to_name("yellow");
			if (_yellow_led_on++ % 2) {
				toggle_yellow_on_off.set_action_name("off");
			} else {
				toggle_yellow_on_off.set_action_name("on");
			}
			if (_cyclic_transmission) talker.transmitToRepeater(toggle_yellow_on_off);
		}
	}

    
    // Index-based operations (simplified examples)
    bool _actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) override {
        (void)talker;		// Silence unused parameter warning
    	(void)talker_match;	// Silence unused parameter warning

		if (index < _actionsCount()) {
			// Actual implementation would do something based on index
			switch(index) {

				case 0:
				{
					#ifdef BUZZER_MANIFESTO_DEBUG
					Serial.println(F("\tCase 0 - Triggering the buzzer"));
					#endif
			
					#ifdef BUZZ_PIN
					#ifdef BUZZER_MANIFESTO_DEBUG
						Serial.print(F("\tBUZZ_PIN IS DEFINED as: "));
						Serial.println(BUZZ_PIN);
					#endif

					digitalWrite(BUZZ_PIN, HIGH);
					_buzz_start = (uint16_t)millis();

					#else
					#ifdef BUZZER_MANIFESTO_DEBUG
						Serial.println(F("\tBUZZ_PIN IS NOT DEFINED in this context!"));
					#endif
					#endif

					return true;
				}
				break;

				case 1:
					if (json_message.has_nth_value_number(0)) {
						_buzz_duration_ms = (uint16_t)json_message.get_nth_value_number(0);
					} else {
						json_message.set_nth_value_number(0, _buzz_duration_ms);
					}
					return true;
				break;
			
				case 2:
					_cyclic_transmission = true;
					return true;
				break;
					
				case 3:
					_cyclic_transmission = false;
					return true;
				break;
					
				default: break;
			}
		}
		return false;
	}
    

    void _echo(JsonTalker& talker, JsonMessage& json_message, MessageValue message_value, TalkerMatch talker_match) override {
		(void)talker;		// Silence unused parameter warning
        (void)message_value;	// Silence unused parameter warning
		(void)talker_match;	// Silence unused parameter warning

		char temp_string[TALKIE_MAX_LEN];
		json_message.get_from_name(temp_string);
		Serial.print( temp_string );
        Serial.print(" - ");
		
		ValueType value_type = json_message.get_nth_value_type(0);
		switch (value_type) {

			case ValueType::TALKIE_VT_STRING:
				json_message.get_nth_value_string(0, temp_string);
				Serial.println(temp_string);
			break;
			
			case ValueType::TALKIE_VT_INTEGER:
				Serial.println(json_message.get_nth_value_number(0));
			break;
			
			default:
            	Serial.println(F("Empty echo received!"));
			break;
		}
    }


    void _noise(JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) override {
		(void)talker;		// Silence unused parameter warning
		(void)talker_match;	// Silence unused parameter warning

		if (json_message.is_recover_message()) {
			char from_name[TALKIE_MAX_LEN];
			json_message.get_from_name(from_name);
			Serial.print("Recovery message from: ");
			Serial.print(from_name);
			Serial.print(" - ");
			json_message.write_to(Serial);
			Serial.print(" | ");
			Serial.println((int)json_message.get_recover_message_value());
		}
    }

};


#endif // BUZZER_MANIFESTO_HPP
