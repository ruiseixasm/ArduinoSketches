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

#include "../IManifesto.hpp"

// #define GREEN_TALKER_DEBUG


class GreenManifesto : public IManifesto {
public:

    const char* class_name() const override { return "GreenManifesto"; }

    GreenManifesto() : IManifesto() {}	// Constructor


protected:

    bool _is_led_on = false;  // keep track of state yourself, by default it's off
    uint16_t _bpm_10 = 1200;
    uint16_t _total_runs = 0;


    Action runs[2] = {
		{"on", "Turns led ON"},
		{"off", "Turns led OFF"}
    };
    
    Action sets[1] = {
        {"bpm_10", "Sets the Tempo in BPM x 10"}
    };
    
    Action gets[1] = {
        {"bpm_10", "Gets the Tempo in BPM x 10"}
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
        (void)talker;		// Silence unused parameter warning
		
		if (index >= sizeof(runs)/sizeof(Action)) return false;
		
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
					_total_runs++;
					return true;
				} else {
					json_message["r"] = "Already On!";
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
					_total_runs++;
				} else {
					json_message["r"] = "Already Off!";
					return false;
				}
				return true;
			}
			break;
		}
		return false;
	}
    
    bool setByIndex(uint8_t index, uint32_t value, JsonObject& json_message, JsonTalker* talker) override {
        (void)json_message;	// Silence unused parameter warning
        (void)talker;		// Silence unused parameter warning
        if (index >= sizeof(sets)/sizeof(Action)) return false;
        
        switch(index) {
            case 0:
                _bpm_10 = value;
                return true;
                break;
        }
        return false;
    }
    
    uint32_t getByIndex(uint8_t index, JsonObject& json_message, JsonTalker* talker) const override {
        (void)json_message;	// Silence unused parameter warning
        (void)talker;		// Silence unused parameter warning
        if (index >= sizeof(gets)/sizeof(Action)) return 0;
        
        switch(index) {
            case 0: return _bpm_10;
        }
        return 0;
    }

};


#endif // GREEN_MANIFESTO_HPP
