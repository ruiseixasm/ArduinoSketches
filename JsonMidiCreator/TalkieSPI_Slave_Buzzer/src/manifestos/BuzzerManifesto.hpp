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

#include "../IManifesto.hpp"

// #define BUZZER_MANIFESTO_DEBUG

#define BUZZ_PIN 2	// External BUZZER pin

class BuzzerManifesto : public IManifesto {
public:

    const char* class_name() const override { return "BuzzerManifesto"; }

    BuzzerManifesto() : IManifesto() {}	// Constructor


protected:

    uint16_t _buzz_time_ms = 100;

	// ALWAYS MAKE SURE THE DIMENSIONS OF THE ARRAYS BELOW ARE THE CORRECT!

    Action runs[1] = {
		{"buzz", "Buzz for a while"}
    };
    
    Action sets[1] = {
        {"ms", "Sets the buzzing duration"}
    };
    
    Action gets[1] = {
        {"ms", "Gets the buzzing duration"}
    };

    const Action* getRunsArray() const override { return runs; }
    const Action* getSetsArray() const override { return sets; }
    const Action* getGetsArray() const override { return gets; }

    // Size methods
    uint8_t runsCount() const override { return sizeof(runs)/sizeof(Action); }
    uint8_t setsCount() const override { return sizeof(sets)/sizeof(Action); }
    uint8_t getsCount() const override { return sizeof(gets)/sizeof(Action); }


public:
    
    // Index-based operations (simplified examples)
    bool runByIndex(uint8_t index, JsonObject& json_message, JsonTalker* talker) override {
        (void)json_message;	// Silence unused parameter warning
        (void)talker;		// Silence unused parameter warning
		if (index < runsCount()) {
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
					delay(_buzz_time_ms);
					digitalWrite(BUZZ_PIN, LOW);

					#else
					#ifdef BUZZER_MANIFESTO_DEBUG
						Serial.println(F("\tBUZZ_PIN IS NOT DEFINED in this context!"));
					#endif
					#endif

					return true;
				}
				break;
			}
		}
		return false;
	}
    
    bool setByIndex(uint8_t index, uint32_t value, JsonObject& json_message, JsonTalker* talker) override {
        (void)json_message;	// Silence unused parameter warning
        (void)talker;		// Silence unused parameter warning
        if (index < setsCount()) {
			switch(index) {
				case 0:
					_buzz_time_ms = value;
					return true;
					break;
			}
		}
        return false;
    }
    
    uint32_t getByIndex(uint8_t index, JsonObject& json_message, JsonTalker* talker) const override {
        (void)json_message;	// Silence unused parameter warning
        (void)talker;		// Silence unused parameter warning
        if (index < getsCount()) {
			switch(index) {
				case 0: return _buzz_time_ms;
			}
		}
        return 0;
    }

};


#endif // BUZZER_MANIFESTO_HPP
