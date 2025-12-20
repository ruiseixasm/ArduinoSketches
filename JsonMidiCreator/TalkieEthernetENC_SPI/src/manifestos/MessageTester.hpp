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
#ifndef MESSAGE_TESTER_MANIFESTO_HPP
#define MESSAGE_TESTER_MANIFESTO_HPP

#include "../TalkerManifesto.hpp"
#include "../JsonMessage.hpp"

// #define MESSAGE_TESTER_DEBUG


using MessageValue = TalkieCodes::MessageValue;

class MessageTester : public TalkerManifesto {
public:

    const char* class_name() const override { return "MessageTester"; }

    MessageTester() : TalkerManifesto()
	{
	}	// Constructor

    ~MessageTester()
	{	// ~TalkerManifesto() called automatically here
	}	// Destructor


protected:

    Action calls[15] = {
		{"all", "Tests all methods"},
		{"deserialize", "Test deserialize (fill up)"},
		{"compare", "Test if it's the same"},
		{"has", "Test if it finds the given char"},
		{"has_not", "Test if DOESN't find the given char"},
		{"length", "Test it has the right length"},
		{"type", "Test the type of value"},
		{"validate", "Validate message fields"},
		{"identity", "Extract the message identity"},
		{"checksum", "Extract the message checksum"},
		{"message", "Gets the message number"},
		{"from", "Gets the from name string"},
		{"remove", "Removes a given field"},
		{"set", "Sets a given field"},
		{"edge", "Tests edge cases"}
    };
    
    const Action* getActionsArray() const override { return calls; }

    // Size methods
    uint8_t actionsCount() const override { return sizeof(calls)/sizeof(Action); }


public:
    
    // Index-based operations (simplified examples)
    bool actionByIndex(uint8_t index, JsonObject& json_message, JsonTalker* talker) override {
        (void)talker;		// Silence unused parameter warning
		
		if (index >= sizeof(calls)/sizeof(Action)) return false;
		
		const char json_payload[] = "{\"m\":6,\"c\":29973,\"f\":\"buzzer\",\"i\":13825,\"0\":\"I'm a buzzer that buzzes\",\"t\":\"Talker-7a\"}";
		JsonMessage new_json_message;
		new_json_message.deserialize(json_payload, sizeof(json_payload) - 1);	// Discount the '\0' of the literal

		// Actual implementation would do something based on index
		switch(index) {

			case 0:
			{
				uint8_t failed_tests[10] = {0};
				uint8_t value_i = 0;
				bool no_errors = true;
				for (uint8_t test_i = 1; test_i < actionsCount(); test_i++) {
					if (!actionByIndex(test_i, json_message, talker)) {
						failed_tests[value_i++] = test_i;
						no_errors = false;
					}
				}
				for (value_i = 0; value_i < 10; value_i++) {	// Removes all 10 possible values
					json_message.remove( valueKey(value_i) );
				}
				if (!no_errors) {
					for (value_i = 0; failed_tests[value_i] && value_i < 10; value_i++) {
						json_message[ valueKey(value_i) ] = failed_tests[value_i];
					}
				}
				return no_errors;
			}
			break;

			case 1:
			{
				if (!new_json_message.deserialize(json_payload, sizeof(json_payload) - 1)) return false;
				return true;
			}
			break;

			case 2:
			{
				if (!new_json_message.compare(json_payload, sizeof(json_payload) - 1)) return false;
				if (!new_json_message.compare_string(json_payload)) return false;
				return true;
			}
			break;

			case 3:
			{
				if (!new_json_message.has_key('m')) {
					json_message[ valueKey(0) ] = "m";
					return false;
				}
				if (!new_json_message.has_key('c')) {
					json_message[ valueKey(0) ] = "c";
					return false;
				}
				if (!new_json_message.has_key('f')) {
					json_message[ valueKey(0) ] = "f";
					return false;
				}
				if (!new_json_message.has_key('i')) {
					json_message[ valueKey(0) ] = "i";
					return false;
				}
				if (!new_json_message.has_key('0')) {
					json_message[ valueKey(0) ] = "0";
					return false;
				}
				if (!new_json_message.has_key('t')) {
					json_message[ valueKey(0) ] = "t";
					return false;
				}
				return true;
			}
			break;
				
			case 4:
			{
				if (new_json_message.has_key('n')) return false;
				if (new_json_message.has_key('d')) return false;
				if (new_json_message.has_key('e')) return false;
				if (new_json_message.has_key('j')) return false;
				if (new_json_message.has_key('1')) return false;
				if (new_json_message.has_key('u')) return false;
				return true;
			}
			break;
				
			case 5:
			{
				size_t length = sizeof(json_payload) - 1;	// Discount the '\0' char at the end
				json_message[ valueKey(0) ] = length;
				json_message[ valueKey(1) ] = new_json_message.get_length();
				if (new_json_message.get_length() != length) {
					return false;
				}
				return true;
			}
			break;
				
			case 6:
			{
				if (new_json_message.get_value_type('c') != JsonMessage::INTEGER) {
					json_message[ valueKey(0) ] = "c";
					json_message[ valueKey(1) ] = static_cast<int>(new_json_message.get_value_type('c'));
					return false;
				}
				if (new_json_message.get_value_type('f') != JsonMessage::STRING) {
					json_message[ valueKey(0) ] = "f";
					json_message[ valueKey(1) ] = static_cast<int>(new_json_message.get_value_type('f'));
					return false;
				}
				if (new_json_message.get_value_type('e') != JsonMessage::VOID) {
					json_message[ valueKey(0) ] = "e";
					json_message[ valueKey(1) ] = static_cast<int>(new_json_message.get_value_type('e'));
					return false;
				}
				return true;
			}
			break;
				
			case 7:
			{
				return new_json_message.validate_fields();
			}
			break;
				
			case 8:
			{
				json_message[ valueKey(0) ] = new_json_message.get_number('i');
				json_message[ valueKey(1) ] = 13825;
				return new_json_message.get_number('i') == 13825;
			}
			break;
				
			case 9:
			{
				json_message[ valueKey(0) ] = new_json_message.get_number('c');
				json_message[ valueKey(1) ] = 29973;
				return new_json_message.get_number('c') == 29973;
			}
			break;
				
			case 10:
			{
				MessageValue message_value = static_cast<MessageValue>( new_json_message.get_number('m') );
				json_message[ valueKey(0) ] = static_cast<int>(message_value);
				json_message[ valueKey(1) ] = static_cast<int>(MessageValue::ECHO);
				return message_value == MessageValue::ECHO;	// 6 is ECHO
			}
			break;
				
			case 11:
			{
				bool from_match = false;
				char from_name[] = "buzzer";
				char out_name[16] = {'\0'};	// Name chars 16 sized (scheme adopted)
				if (new_json_message.get_string('f', out_name, 16)) {
					if (strcmp(out_name, from_name) == 0) {
						from_match = true;
					}
				}
				json_message[ valueKey(0) ] = static_cast<char*>(from_name);
				json_message[ valueKey(1) ] = static_cast<char*>(out_name);
				return from_match;
			}
			break;
				
			case 12:
			{
				bool payloads_match = true;
				const char final_payload1[] = "{\"m\":6,\"c\":29973,\"i\":13825,\"0\":\"I'm a buzzer that buzzes\",\"t\":\"Talker-7a\"}";
				new_json_message.remove('f');
				if (!new_json_message.compare_string(final_payload1)) {
					json_message[ valueKey(0) ] = "Failed match 1";
					payloads_match = false;
				}
				const char final_payload2[] = "{\"m\":6,\"c\":29973,\"i\":13825,\"t\":\"Talker-7a\"}";
				new_json_message.remove('0');
				if (!new_json_message.compare_string(final_payload2)) {
					if (payloads_match) {
						json_message[ valueKey(0) ] = "Failed match 2";
					} else {
						json_message[ valueKey(1) ] = "Failed match 2";
					}
					payloads_match = false;
				}
				return payloads_match;
			}
			break;
				
			case 13:
			{
				bool set_match = false;
				const char final_payload[] = "{\"m\":6,\"c\":29973,\"f\":\"buzzer\",\"i\":13825,\"t\":\"Talker-7a\",\"0\":1234567}";
				uint32_t big_number = 1234567;
				if (new_json_message.set('0', big_number) && new_json_message.compare_string(final_payload)) {
					set_match = true;
				}
				json_message[ valueKey(0) ] = sizeof(final_payload) - 1;
				json_message[ valueKey(1) ] = new_json_message.get_length();
				return set_match;
			}
			break;
				
			case 14:
			{
				const char final_payload[] = "{\"c\":29973,\"f\":\"buzzer\",\"i\":13825,\"0\":\"I'm a buzzer that buzzes\",\"t\":\"Talker-7a\"}";
				if (!new_json_message.remove('m') || !new_json_message.compare_string(final_payload)) {	// The first key (no header ',' char)
					json_message[ valueKey(0) ] = "first";
					return false;
				}
				const char single_key[] = "{\"i\":13825}";
				if (!new_json_message.deserialize_string(single_key)) {
					json_message[ valueKey(0) ] = "second";
					return false;
				}
				return true;
			}
			break;
				

            default: return false;
		}
		return false;
	}
    
};


#endif // MESSAGE_TESTER_MANIFESTO_HPP
