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

#include "../TalkerManifesto.hpp"

// #define GREEN_TALKER_DEBUG

#define BUZZ_PIN 2	// External BUZZER pin


class MultipleManifesto : public TalkerManifesto {
public:

    const char* class_name() const override { return "MultipleManifesto"; }

    MultipleManifesto() : TalkerManifesto() {}	// Constructor


protected:

    uint16_t _buzz_duration_ms = 100;
	uint16_t _buzz_start = 0;

	// ALWAYS MAKE SURE THE DIMENSIONS OF THE ARRAYS BELOW ARE THE CORRECT!

    Action calls[2] = {
		{"buzz", "Buzz a little"},
		{"ms", "For how long"}
    };
    
public:
    
    const Action* _getActionsArray() const override { return calls; }

    // Size methods
    uint8_t _actionsCount() const override { return sizeof(calls)/sizeof(Action); }


	void _loop(JsonTalker* talker) override {
        (void)talker;		// Silence unused parameter warning
		if ((uint16_t)millis() - _buzz_start > _buzz_duration_ms) {
			#ifdef BUZZ_PIN
			digitalWrite(BUZZ_PIN, LOW);
			#endif
		}
	}

    
    // Index-based operations (simplified examples)
    bool _actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) override {
        (void)talker;		// Silence unused parameter warning
        (void)json_message;	// Silence unused parameter warning

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
						// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (IN PROGRESS) ***************
						_buzz_duration_ms = (uint16_t)json_message.get_nth_value_number(0);
					} else {
						json_message.set_nth_value_number(0, _buzz_duration_ms);
					}
					return true;
				break;

				// case 2:
				// 	return _buzz_duration_ms;
			}
		}
		return false;
	}
    
};


#endif // SERIAL_MANIFESTO_HPP
