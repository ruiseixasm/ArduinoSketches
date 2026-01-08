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
// #define MESSAGE_TESTER_DEBUG_NEW


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

    Action calls[17] = {
		{"all", "Tests all methods"},
		{"deserialize", "Test deserialize (fill up)"},
		{"compare", "Test if it's the same"},
		{"has", "Test if it finds the given char"},
		{"has_not", "Test if DOESN't find the given char"},
		{"length", "Test it has the right length"},
		{"type", "Test the type of value"},
		{"validate", "Validate message fields"},
		{"identity", "Extract the message identity"},
		{"value", "Checks if it has a value 0"},
		{"message", "Gets the message number"},
		{"from", "Gets the from name string"},
		{"remove", "Removes a given field"},
		{"set", "Sets a given field"},
		{"edge", "Tests edge cases"},
		{"copy", "Tests the copy constructor"},
		{"string", "Checks if it has a value 0 as string"}
    };
    
public:
    
    const Action* _getActionsArray() const override { return calls; }

    // Size methods
    uint8_t _actionsCount() const override { return sizeof(calls)/sizeof(Action); }


    // Index-based operations (simplified examples)
    bool _actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) override {
		
		if (index >= sizeof(calls)/sizeof(Action)) return false;
		
		const char json_payload[] = "{\"m\":7,\"b\":0,\"f\":\"buzzer\",\"i\":13825,\"0\":\"I'm a buzzer that buzzes\",\"t\":\"Talker-7a\"}";
		JsonMessage test_json_message;
		test_json_message.deserialize_buffer(json_payload, sizeof(json_payload) - 1);	// Discount the '\0' of the literal

		// Actual implementation would do something based on index
		switch(index) {

			case 0:
			{
				uint8_t failed_tests[10] = {0};
				uint8_t value_i = 0;
				bool no_errors = true;
				for (uint8_t test_i = 1; test_i < _actionsCount(); test_i++) {
					if (!_actionByIndex(test_i, talker, json_message, talker_match)) {
						failed_tests[value_i++] = test_i;
						no_errors = false;
					}
				}
				for (value_i = 0; value_i < 10; value_i++) {	// Removes all 10 possible values
					json_message.remove_nth_value(value_i);
				}
				if (!no_errors) {
					for (value_i = 0; failed_tests[value_i] && value_i < 10; value_i++) {
						json_message.set_nth_value_number(value_i, failed_tests[value_i]);
					}
				}
				
				#ifdef MESSAGE_TESTER_DEBUG_NEW
				Serial.print(F("\t\t\tactionByIndex1: "));
				json_message.write_to(Serial);
				Serial.print(" | ");
				Serial.println(index);
				#endif

				return no_errors;
			}
			break;

			case 1:
			{
				if (!test_json_message.deserialize_buffer(json_payload, sizeof(json_payload) - 1)) return false;
				return true;
			}
			break;

			case 2:
			{
				if (!test_json_message.compare_buffer(json_payload, sizeof(json_payload) - 1)) return false;
				return true;
			}
			break;

			case 3:
			{
				if (!test_json_message.has_key('m')) {
					json_message.set_nth_value_string(0, "m");
					return false;
				}
				if (!test_json_message.has_key('f')) {
					json_message.set_nth_value_string(0, "f");
					return false;
				}
				if (!test_json_message.has_key('i')) {
					json_message.set_nth_value_string(0, "i");
					return false;
				}
				if (!test_json_message.has_key('0')) {
					json_message.set_nth_value_string(0, "0");
					return false;
				}
				if (!test_json_message.has_key('t')) {
					json_message.set_nth_value_string(0, "t");
					return false;
				}
				return true;
			}
			break;
				
			case 4:
			{
				if (test_json_message.has_key('n')) return false;
				if (test_json_message.has_key('d')) return false;
				if (test_json_message.has_key('e')) return false;
				if (test_json_message.has_key('j')) return false;
				if (test_json_message.has_key('1')) return false;
				if (test_json_message.has_key('u')) return false;
				return true;
			}
			break;
				
			case 5:
			{
				size_t length = sizeof(json_payload) - 1;	// Discount the '\0' char at the end
				json_message.set_nth_value_number(0, length);
				json_message.set_nth_value_number(1, test_json_message.get_length());
				if (test_json_message.get_length() != length) {
					return false;
				}
				return true;
			}
			break;
				
			case 6:
			{
				if (test_json_message._get_value_type('m') != ValueType::TALKIE_VT_INTEGER) {
					json_message.set_nth_value_string(0, "m");
					json_message.set_nth_value_number(1, static_cast<int>(test_json_message._get_value_type('m')));
					return false;
				}
				if (test_json_message._get_value_type('f') != ValueType::TALKIE_VT_STRING) {
					json_message.set_nth_value_string(0, "f");
					json_message.set_nth_value_number(1, static_cast<int>(test_json_message._get_value_type('f')));
					return false;
				}
				if (test_json_message._get_value_type('i') != ValueType::TALKIE_VT_INTEGER) {
					json_message.set_nth_value_string(0, "i");
					json_message.set_nth_value_number(1, static_cast<int>(test_json_message._get_value_type('i')));
					return false;
				}
				return true;
			}
			break;
				
			case 7:
			{
				return test_json_message.validate_fields();
			}
			break;
				
			case 8:
			{
				
				json_message.set_nth_value_number(0, test_json_message._get_value_number('i'));
				json_message.set_nth_value_number(1, 13825);
				return test_json_message._get_value_number('i') == 13825;
			}
			break;
				
			case 9:
			{
				return test_json_message.has_nth_value(0);
			}
			break;
				
			case 10:
			{
				MessageValue message_value = static_cast<MessageValue>( test_json_message._get_value_number('m') );
				json_message.set_nth_value_number(0, static_cast<int>(message_value));
				json_message.set_nth_value_number(1, static_cast<int>(MessageValue::TALKIE_MSG_ECHO));
				return message_value == MessageValue::TALKIE_MSG_ECHO;
			}
			break;
				
			case 11:
			{
				bool from_match = false;
				char from_name[] = "buzzer";
				if (strcmp(test_json_message.get_from_name(), from_name) == 0) {
					from_match = true;
				}
				json_message.set_nth_value_string(0, from_name);
				json_message.set_nth_value_string(1, test_json_message.get_from_name());
				return from_match;
			}
			break;
				
			case 12:
			{
				bool payloads_match = true;
				const char final_payload1[] = "{\"m\":7,\"b\":0,\"i\":13825,\"0\":\"I'm a buzzer that buzzes\",\"t\":\"Talker-7a\"}";
				test_json_message.remove_from();
				if (!test_json_message.compare_buffer(final_payload1, sizeof(final_payload1) - 1)) {
					json_message.set_nth_value_string(0, "Failed match 1");
					payloads_match = false;
				}
				const char final_payload2[] = "{\"m\":7,\"b\":0,\"i\":13825,\"t\":\"Talker-7a\"}";
				test_json_message.remove_nth_value(0);
				if (!test_json_message.compare_buffer(final_payload2, sizeof(final_payload2) - 1)) {
					if (payloads_match) {	// has to be at 0
						json_message.set_nth_value_string(0, "Failed match 2");
					} else {
						json_message.set_nth_value_string(1, "Failed match 2");
					}
					payloads_match = false;
				}
				return payloads_match;
			}
			break;
				
			case 13:
			{
				uint32_t big_number = 1234567;
				const char final_payload1[] = "{\"m\":7,\"b\":0,\"f\":\"buzzer\",\"i\":13825,\"t\":\"Talker-7a\",\"0\":1234567}";
				if (!test_json_message.set_nth_value_number(0, big_number) || !test_json_message.compare_buffer(final_payload1, sizeof(final_payload1) - 1)) {
					json_message.set_nth_value_string(0, "1st");
					json_message.set_nth_value_number(1, sizeof(final_payload1) - 1);
					json_message.set_nth_value_number(2, test_json_message.get_length());
					return false;
				}
				const char from_green[] = "green";
				const char final_payload2[] = "{\"m\":7,\"b\":0,\"i\":13825,\"t\":\"Talker-7a\",\"0\":1234567,\"f\":\"green\"}";
				if (!test_json_message.set_from_name(from_green) || !test_json_message.compare_buffer(final_payload2, sizeof(final_payload2) - 1)) {
					json_message.set_nth_value_string(0, "2nd");
					json_message.set_nth_value_number(1, sizeof(final_payload2) - 1);
					json_message.set_nth_value_number(2, test_json_message.get_length());
					return false;
				}
				return true;
			}
			break;
				
			case 14:
			{
				const char final_payload[] = "{\"b\":0,\"f\":\"buzzer\",\"i\":13825,\"0\":\"I'm a buzzer that buzzes\",\"t\":\"Talker-7a\"}";
				if (!test_json_message.remove_message() || !test_json_message.compare_buffer(final_payload, sizeof(final_payload) - 1)) {	// The first key (no header ',' char)
					json_message.set_nth_value_string(0, "1st");
					return false;
				}
				const char single_key[] = "{\"i\":13825}";
				if (!test_json_message.deserialize_buffer(single_key, sizeof(single_key) - 1)) {
					json_message.set_nth_value_string(0, "2nd");
					return false;
				}
				if (!test_json_message.set_identity(32423)) {
					json_message.set_nth_value_string(0, "3rd");
					return false;
				}
				const char new_single_key[] = "{\"i\":32423}";
				if (!test_json_message.compare_buffer(new_single_key, sizeof(new_single_key) - 1)) {
					json_message.set_nth_value_string(0, "4th");
					json_message.set_nth_value_number(1, test_json_message.get_length());
					return false;
				}
				return true;
			}
			break;
				
			case 15:
			{
				JsonMessage copy_json_message(test_json_message);
				if (copy_json_message != test_json_message) {
					json_message.set_nth_value_string(0, "1st");
					return false;
				}
				const char different_payload[] = "{\"f\":\"buzzer\",\"i\":13825,\"0\":\"I'm a buzzer that buzzes\",\"t\":\"Talker-7a\"}";
				copy_json_message.deserialize_buffer(different_payload, sizeof(different_payload) - 1);
				if (copy_json_message == test_json_message) {
					json_message.set_nth_value_string(0, "2nd");
					return false;
				}
				return true;
			}
			break;
				
			case 16:
			{
				return test_json_message.has_nth_value_string(0);
			}
			break;
				

            default: return false;
		}
		return false;
	}
    
};


#endif // MESSAGE_TESTER_MANIFESTO_HPP
