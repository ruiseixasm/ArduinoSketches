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

#include "../TalkerManifesto.hpp"

// #define BUZZER_MANIFESTO_DEBUG

#define BUZZ_PIN 2	// External BUZZER pin


class BuzzerManifesto : public TalkerManifesto {
public:

    const char* class_name() const override { return "BuzzerManifesto"; }

    BuzzerManifesto() : TalkerManifesto() {}	// Constructor


protected:

    uint16_t _buzz_duration_ms = 100;
	uint16_t _buzz_start = 0;

	// ALWAYS MAKE SURE THE DIMENSIONS OF THE ARRAYS BELOW ARE THE CORRECT!

    Action calls[2] = {
		{"buzz", "Buzz for a while"},
		{"ms", "Gets and sets the buzzing duration"}
    };
    
    const Action* getActionsArray() const override { return calls; }

    // Size methods
    uint8_t actionsCount() const override { return sizeof(calls)/sizeof(Action); }


public:

	void loop(JsonTalker* talker) override {
        (void)talker;		// Silence unused parameter warning
		if ((uint16_t)millis() - _buzz_start > _buzz_duration_ms) {
			#ifdef BUZZ_PIN
			digitalWrite(BUZZ_PIN, LOW);
			#endif
		}
	}

    
    // Index-based operations (simplified examples)
    bool actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& json_message) override {
        (void)talker;		// Silence unused parameter warning
        (void)json_message;	// Silence unused parameter warning

		if (index < actionsCount()) {
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
    

    void echo(JsonTalker& talker, JsonMessage& json_message) override {
        (void)talker;		// Silence unused parameter warning
		
		// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
		Serial.print( json_message.get_from_name() );
        Serial.print(" - ");
		ValueType value_type = json_message.get_nth_value_type(0);
		switch (value_type) {

			case ValueType::STRING:
				Serial.println(json_message.get_nth_value_string(0));
			break;
			
			case ValueType::INTEGER:
				Serial.println(json_message.get_nth_value_number(0));
			break;
			
			default:
            	Serial.println(F("Empty echo received!"));
			break;
		}
    }


    void error(JsonTalker& talker, JsonMessage& json_message) override {
        (void)talker;		// Silence unused parameter warning
		
		// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
		Serial.print( json_message.get_from_name() );
        Serial.print(" - ");
		ValueType value_type = json_message.get_nth_value_type(0);
		switch (value_type) {

			case ValueType::STRING:
				Serial.println(json_message.get_nth_value_string(0));
			break;
			
			case ValueType::INTEGER:
				Serial.println(json_message.get_nth_value_number(0));
			break;
			
			default:
            	Serial.println(F("Empty echo received!"));
			break;
		}
    }

};


#endif // BUZZER_MANIFESTO_HPP
