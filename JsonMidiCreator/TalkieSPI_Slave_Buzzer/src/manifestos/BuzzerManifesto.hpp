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


using JsonKey = TalkieCodes::JsonKey;

class BuzzerManifesto : public TalkerManifesto {
public:

    const char* class_name() const override { return "BuzzerManifesto"; }

    BuzzerManifesto() : TalkerManifesto() {}	// Constructor


protected:

    uint16_t _buzz_duration_ms = 100;
	uint16_t _buzz_start = 0;

	// ALWAYS MAKE SURE THE DIMENSIONS OF THE ARRAYS BELOW ARE THE CORRECT!

    Action calls[3] = {
		{"buzz", "Buzz for a while"},
		{"ms", "Sets the buzzing duration"},
		{"ms", "Gets the buzzing duration"}
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
    bool actionByIndex(uint8_t index, JsonObject& json_message, JsonTalker* talker) override {
        (void)json_message;	// Silence unused parameter warning
        (void)talker;		// Silence unused parameter warning
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
					_buzz_duration_ms = json_message[ valueKey(0) ].as<uint16_t>();
					return true;
				break;

				// case 2:
				// 	return _buzz_duration_ms;
			}
		}
		return false;
	}
    

    void echo(JsonObject& json_message, JsonTalker* talker) override {
        (void)talker;		// Silence unused parameter warning
        Serial.print(json_message[ JsonKey::FROM ].as<String>());
        Serial.print(" - ");
        if (json_message[ valueKey(0) ].is<String>()) {
            Serial.println(json_message[ valueKey(0) ].as<String>());
        } else if (json_message[ JsonKey::DESCRIPTION ].is<String>()) {
            Serial.println(json_message[ JsonKey::DESCRIPTION ].as<String>());
        } else {
            Serial.println(F("Empty echo received!"));
        }
    }


    void error(JsonObject& json_message, JsonTalker* talker) override {
        (void)talker;		// Silence unused parameter warning
        Serial.print(json_message[ JsonKey::FROM ].as<String>());
        Serial.print(" - ");
        if (json_message[ valueKey(0) ].is<String>()) {
            Serial.println(json_message[ valueKey(0) ].as<String>());
        } else if (json_message[ JsonKey::DESCRIPTION ].is<String>()) {
            Serial.println(json_message[ JsonKey::DESCRIPTION ].as<String>());
        } else {
            Serial.println(F("Empty error received!"));
        }
    }

};


#endif // BUZZER_MANIFESTO_HPP
