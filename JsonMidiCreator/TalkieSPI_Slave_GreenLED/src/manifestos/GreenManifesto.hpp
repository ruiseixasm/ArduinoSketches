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

protected:

    uint16_t _bpm_10 = 1200;

    uint16_t _total_runs = 0;
    bool _is_led_on = false;  // keep track of state yourself, by default it's off


    const uint8_t runsCount_ = 2;
    const uint8_t setsCount_ = 1;
    const uint8_t getsCount_ = 1;
    
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
    
    // Iterator states
    uint8_t runsIterIdx = 0;
    uint8_t setsIterIdx = 0;
    uint8_t getsIterIdx = 0;


public:

    const char* class_name() const override { return "GreenManifesto"; }

    GreenManifesto() : IManifesto() {}


    // Size methods
    uint8_t runsCount() const override { return runsCount_; }
    uint8_t setsCount() const override { return setsCount_; }
    uint8_t getsCount() const override { return getsCount_; }


    // Iterator methods
    bool iterateRunsReset() override { 
        runsIterIdx = 0; 
        return true; 
    }
    
    Action* iterateRunsNext() override {
        if (runsIterIdx < runsCount_) {
            return &runs[runsIterIdx++];
        }
        return nullptr;
    }
    
    bool iterateSetsReset() override { 
        setsIterIdx = 0; 
        return true; 
    }
    
    Action* iterateSetsNext() override {
        if (setsIterIdx < setsCount_) {
            return &sets[setsIterIdx++];
        }
        return nullptr;
    }
    
    bool iterateGetsReset() override { 
        getsIterIdx = 0; 
        return true; 
    }
    
    Action* iterateGetsNext() override {
        if (getsIterIdx < getsCount_) {
            return &gets[getsIterIdx++];
        }
        return nullptr;
    }


    // Name-based operations
    uint8_t runIndex(const char* name) const override {
        for (uint8_t i = 0; i < runsCount_; i++) {
            if (strcmp(runs[i].name, name) == 0) {
                return i;
            }
        }
        return 255;
    }
    
    uint8_t setIndex(const char* name) const override {
        for (uint8_t i = 0; i < setsCount_; i++) {
            if (strcmp(sets[i].name, name) == 0) {
                return i;
            }
        }
        return 255;
    }
    
    uint8_t getIndex(const char* name) const override {
        for (uint8_t i = 0; i < getsCount_; i++) {
            if (strcmp(gets[i].name, name) == 0) {
                return i;
            }
        }
        return 255;
    }

    
    // Index-based operations (simplified examples)
    bool runByIndex(uint8_t index, JsonObject& json_message, JsonTalker* talker) override {
        (void)talker;		// Silence unused parameter warning
		if (index >= runsCount_) return false;
		
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
        if (index >= setsCount_) return false;
        
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
        if (index >= getsCount_) return 0;
        
        switch(index) {
            case 0: return _bpm_10;
        }
        return 0;
    }

};


#endif // GREEN_MANIFESTO_HPP
