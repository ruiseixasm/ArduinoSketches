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
#ifndef SYSTEM_MANAGER_HPP
#define SYSTEM_MANAGER_HPP

#include "IManifesto.hpp"


class SystemManager : public IManifesto {

protected:

    static constexpr uint8_t MAX_ACTIONS = 10;
    
    Action runs[MAX_ACTIONS] = {
        {"start", "Start the system"},
        {"stop", "Stop the system"},
        {"reset", "Reset to defaults"}
    };
    
    Action sets[MAX_ACTIONS] = {
        {"speed", "Set motor speed"},
        {"temp", "Set temperature"}
    };
    
    Action gets[MAX_ACTIONS] = {
        {"status", "Get system status"},
        {"counter", "Get operation counter"}
    };
    
    uint8_t runsCount_ = 3;
    uint8_t setsCount_ = 2;
    uint8_t getsCount_ = 2;
    
    // Iterator states
    uint8_t runsIterIdx = 0;
    uint8_t setsIterIdx = 0;
    uint8_t getsIterIdx = 0;


public:

    const char* class_name() const override { return "SystemManager"; }


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
    bool runByName(const char* name) override {
        for (uint8_t i = 0; i < runsCount_; i++) {
            if (strcmp(runs[i].name, name) == 0) {
                return runByIndex(i);
            }
        }
        return false;
    }
    
    bool setByName(const char* name, uint32_t value) override {
        for (uint8_t i = 0; i < setsCount_; i++) {
            if (strcmp(sets[i].name, name) == 0) {
                return setByIndex(i, value);
            }
        }
        return false;
    }
    
    uint32_t getByName(const char* name) const override {
        for (uint8_t i = 0; i < getsCount_; i++) {
            if (strcmp(gets[i].name, name) == 0) {
                return getByIndex(i);
            }
        }
        return 0;  // Or some error value
    }

    
    // Index-based operations (simplified examples)
    bool runByIndex(uint8_t index) override {
        if (index >= runsCount_) return false;
        
        // Actual implementation would do something based on index
        switch(index) {
            case 0: Serial.println("Starting system"); break;
            case 1: Serial.println("Stopping system"); break;
            case 2: Serial.println("Resetting system"); break;
        }
        return true;
    }
    
    bool setByIndex(uint8_t index, uint32_t value) override {
        if (index >= setsCount_) return false;
        
        switch(index) {
            case 0: 
                Serial.print("Setting speed to: ");
                Serial.println(value);
                break;
            case 1:
                Serial.print("Setting temperature to: ");
                Serial.println(value);
                break;
        }
        return true;
    }
    
    uint32_t getByIndex(uint8_t index) const override {
        if (index >= getsCount_) return 0;
        
        switch(index) {
            case 0: return 123;  // status
            case 1: return 456;  // counter
        }
        return 0;
    }
};


#endif // SYSTEM_MANAGER_HPP
