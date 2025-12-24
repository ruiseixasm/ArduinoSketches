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
#define MESSAGE_TESTER_DEBUG_NEW


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
		{"checksum", "Extract the message checksum"},
		{"message", "Gets the message number"},
		{"from", "Gets the from name string"},
		{"remove", "Removes a given field"},
		{"set", "Sets a given field"},
		{"edge", "Tests edge cases"},
		{"copy", "Tests the copy constructor"},
		{"checksum", "Tests extracting and inserting the checksum"}
    };
    
    const Action* getActionsArray() const override { return calls; }

    // Size methods
    uint8_t actionsCount() const override { return sizeof(calls)/sizeof(Action); }


public:
    
    // Index-based operations (simplified examples)
    bool actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& new_json_message) override {
        (void)talker;		// Silence unused parameter warning
		
		if (index >= sizeof(calls)/sizeof(Action)) return false;
		
		const char json_payload[] = "{\"m\":6,\"c\":29973,\"f\":\"buzzer\",\"i\":13825,\"0\":\"I'm a buzzer that buzzes\",\"t\":\"Talker-7a\"}";
		JsonMessage test_json_message;
		test_json_message.deserialize_buffer(json_payload, sizeof(json_payload) - 1);	// Discount the '\0' of the literal

		// Actual implementation would do something based on index
		switch(index) {

			case 0:
			{
				uint8_t failed_tests[10] = {0};
				uint8_t value_i = 0;
				bool no_errors = true;
				for (uint8_t test_i = 1; test_i < actionsCount(); test_i++) {
					if (!actionByIndex(test_i, talker, new_json_message)) {
						failed_tests[value_i++] = test_i;
						no_errors = false;
					}
				}
				for (value_i = 0; value_i < 10; value_i++) {	// Removes all 10 possible values
					old_json_message.remove( valueKey(value_i) );
					new_json_message.remove_nth_value(value_i);
				}
				if (!no_errors) {
					for (value_i = 0; failed_tests[value_i] && value_i < 10; value_i++) {
						old_json_message[ valueKey(value_i) ] = failed_tests[value_i];
						new_json_message.set_nth_value_number(value_i, failed_tests[value_i]);
					}
				}
				
				#ifdef MESSAGE_TESTER_DEBUG_NEW
				Serial.print(F("\t\t\tactionByIndex1: "));
				new_json_message.write_to(Serial);
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
					old_json_message[ valueKey(0) ] = "m";
					return false;
				}
				if (!test_json_message.has_key('c')) {
					old_json_message[ valueKey(0) ] = "c";
					return false;
				}
				if (!test_json_message.has_key('f')) {
					old_json_message[ valueKey(0) ] = "f";
					return false;
				}
				if (!test_json_message.has_key('i')) {
					old_json_message[ valueKey(0) ] = "i";
					return false;
				}
				if (!test_json_message.has_key('0')) {
					old_json_message[ valueKey(0) ] = "0";
					return false;
				}
				if (!test_json_message.has_key('t')) {
					old_json_message[ valueKey(0) ] = "t";
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
				old_json_message[ valueKey(0) ] = length;
				old_json_message[ valueKey(1) ] = test_json_message.get_length();
				if (test_json_message.get_length() != length) {
					return false;
				}
				return true;
			}
			break;
				
			case 6:
			{
				if (test_json_message.get_value_type('c') != JsonMessage::INTEGER) {
					old_json_message[ valueKey(0) ] = "c";
					old_json_message[ valueKey(1) ] = static_cast<int>(test_json_message.get_value_type('c'));
					return false;
				}
				if (test_json_message.get_value_type('f') != JsonMessage::STRING) {
					old_json_message[ valueKey(0) ] = "f";
					old_json_message[ valueKey(1) ] = static_cast<int>(test_json_message.get_value_type('f'));
					return false;
				}
				if (test_json_message.get_value_type('e') != JsonMessage::VOID) {
					old_json_message[ valueKey(0) ] = "e";
					old_json_message[ valueKey(1) ] = static_cast<int>(test_json_message.get_value_type('e'));
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
				old_json_message[ valueKey(0) ] = test_json_message.get_number('i');
				old_json_message[ valueKey(1) ] = 13825;
				return test_json_message.get_number('i') == 13825;
			}
			break;
				
			case 9:
			{
				old_json_message[ valueKey(0) ] = test_json_message.get_number('c');
				old_json_message[ valueKey(1) ] = 29973;
				return test_json_message.get_number('c') == 29973;
			}
			break;
				
			case 10:
			{
				MessageValue message_value = static_cast<MessageValue>( test_json_message.get_number('m') );
				old_json_message[ valueKey(0) ] = static_cast<int>(message_value);
				old_json_message[ valueKey(1) ] = static_cast<int>(MessageValue::ECHO);
				return message_value == MessageValue::ECHO;	// 6 is ECHO
			}
			break;
				
			case 11:
			{
				bool from_match = false;
				char from_name[] = "buzzer";
				char out_name[NAME_LEN] = {'\0'};	// Name chars 16 sized (scheme adopted)
				if (strcmp(test_json_message.get_from(), from_name) == 0) {
					from_match = true;
				}
				old_json_message[ valueKey(0) ] = static_cast<char*>(from_name);
				old_json_message[ valueKey(1) ] = static_cast<char*>(out_name);
				return from_match;
			}
			break;
				
			case 12:
			{
				bool payloads_match = true;
				const char final_payload1[] = "{\"m\":6,\"c\":29973,\"i\":13825,\"0\":\"I'm a buzzer that buzzes\",\"t\":\"Talker-7a\"}";
				test_json_message.remove('f');
				if (!test_json_message.compare_buffer(final_payload1, sizeof(final_payload1) - 1)) {
					old_json_message[ valueKey(0) ] = "Failed match 1";
					payloads_match = false;
				}
				const char final_payload2[] = "{\"m\":6,\"c\":29973,\"i\":13825,\"t\":\"Talker-7a\"}";
				test_json_message.remove('0');
				if (!test_json_message.compare_buffer(final_payload2, sizeof(final_payload2) - 1)) {
					if (payloads_match) {
						old_json_message[ valueKey(0) ] = "Failed match 2";
					} else {
						old_json_message[ valueKey(1) ] = "Failed match 2";
					}
					payloads_match = false;
				}
				return payloads_match;
			}
			break;
				
			case 13:
			{
				uint32_t big_number = 1234567;
				const char final_payload1[] = "{\"m\":6,\"c\":29973,\"f\":\"buzzer\",\"i\":13825,\"t\":\"Talker-7a\",\"0\":1234567}";
				if (!test_json_message.set_nth_value_number(0, big_number) || !test_json_message.compare_buffer(final_payload1, sizeof(final_payload1) - 1)) {
					old_json_message[ valueKey(0) ] = "1st";
					old_json_message[ valueKey(1) ] = sizeof(final_payload1) - 1;
					old_json_message[ valueKey(2) ] = test_json_message.get_length();
					return false;
				}
				const char from_green[] = "green";
				const char final_payload2[] = "{\"m\":6,\"c\":29973,\"i\":13825,\"t\":\"Talker-7a\",\"0\":1234567,\"f\":\"green\"}";
				if (!test_json_message.set_from(from_green) || !test_json_message.compare_buffer(final_payload2, sizeof(final_payload2) - 1)) {
					old_json_message[ valueKey(0) ] = "2nd";
					old_json_message[ valueKey(1) ] = sizeof(final_payload2) - 1;
					old_json_message[ valueKey(2) ] = test_json_message.get_length();
					return false;
				}
				return true;
			}
			break;
				
			case 14:
			{
				const char final_payload[] = "{\"c\":29973,\"f\":\"buzzer\",\"i\":13825,\"0\":\"I'm a buzzer that buzzes\",\"t\":\"Talker-7a\"}";
				if (!test_json_message.remove('m') || !test_json_message.compare_buffer(final_payload, sizeof(final_payload) - 1)) {	// The first key (no header ',' char)
					old_json_message[ valueKey(0) ] = "1st";
					return false;
				}
				const char single_key[] = "{\"i\":13825}";
				if (!test_json_message.deserialize_buffer(single_key, sizeof(single_key) - 1)) {
					old_json_message[ valueKey(0) ] = "2nd";
					return false;
				}
				if (!test_json_message.set_identity(32423)) {
					old_json_message[ valueKey(0) ] = "3rd";
					return false;
				}
				const char new_single_key[] = "{\"i\":32423}";
				if (!test_json_message.compare_buffer(new_single_key, sizeof(new_single_key) - 1)) {
					old_json_message[ valueKey(0) ] = "4th";
					old_json_message[ valueKey(1) ] = test_json_message.get_length();
					return false;
				}
				return true;
			}
			break;
				
			case 15:
			{
				JsonMessage copy_json_message(test_json_message);
				if (copy_json_message != test_json_message) {
					old_json_message[ valueKey(0) ] = "1st";
					return false;
				}
				const char different_payload[] = "{\"c\":29973,\"f\":\"buzzer\",\"i\":13825,\"0\":\"I'm a buzzer that buzzes\",\"t\":\"Talker-7a\"}";
				copy_json_message.deserialize_buffer(different_payload, sizeof(different_payload) - 1);
				if (copy_json_message == test_json_message) {
					old_json_message[ valueKey(0) ] = "2nd";
					return false;
				}
				return true;
			}
			break;
				
			case 16:
			{
				uint16_t message_checksum = test_json_message.extract_checksum();
				uint16_t generated_checksum = test_json_message.generate_checksum();
				if (message_checksum != generated_checksum) {
					old_json_message[ valueKey(0) ] = "1st";
					old_json_message[ valueKey(1) ] = message_checksum;
					old_json_message[ valueKey(2) ] = generated_checksum;
					return false;
				}
				const char different_payload[] = "{\"c\":0,\"f\":\"buzzer\",\"i\":13825,\"0\":\"I'm a buzzer that buzzes\",\"t\":\"Talker-7a\"}";
				test_json_message.deserialize_buffer(different_payload, sizeof(different_payload) - 1);
				generated_checksum = test_json_message.generate_checksum();
				test_json_message.insert_checksum();
				message_checksum = test_json_message.get_checksum();
				if (message_checksum != generated_checksum) {
					old_json_message[ valueKey(0) ] = "2nd";
					old_json_message[ valueKey(1) ] = message_checksum;
					old_json_message[ valueKey(2) ] = generated_checksum;
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
