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
#ifndef SPY_MANIFESTO_HPP
#define SPY_MANIFESTO_HPP

#include <TalkerManifesto.hpp>

// #define SPY_MANIFESTO_DEBUG


class M_Spy : public TalkerManifesto {
public:

	// The Manifesto class description shouldn't be greater than 42 chars
	// {"m":7,"f":"","s":1,"b":1,"t":"","i":58485,"0":"","1":1,"c":11266} <-- 128 - (66 + 2*10) = 42
    const char* class_description() const override { return "Spy"; }

	// Constructor
    M_Spy() : TalkerManifesto() {}


protected:

	char _original_talker[TALKIE_NAME_LEN];
	uint16_t _trace_message_timestamp;

	// ALWAYS MAKE SURE THE DIMENSIONS OF THE ARRAYS BELOW ARE THE CORRECT!

    Action calls[3] = {
		{"ping", "Ping talkers by name or channel"},
		{"ping_self", "I can even ping myself"},
		{"call", "Able to do [<talker> <action>]"}
    };
    
public:

    const Action* _getActionsArray() const override { return calls; }

    // Size methods
    uint8_t _actionsCount() const override { return sizeof(calls)/sizeof(Action); }


    // Action implementations - MUST be implemented by derived
    bool _actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) override {
		(void)talker_match;	// Silence unused parameter warning
		
		bool ping = false;

		// As a spy it only answers to REMOTE calls
		BroadcastValue source_data = json_message.get_broadcast_value();
		if (source_data == BroadcastValue::TALKIE_BC_REMOTE) {

			if (index < _actionsCount()) {
				// Actual implementation would do something based on index
				switch(index) {

					case 0:
					{
						ping = true;

						// 1. Start by collecting info from message
						json_message.get_from_name(_original_talker);
						_trace_message_timestamp = json_message.get_identity();

						// 2. Repurpose it to be a LOCAL PING
						json_message.set_message_value(MessageValue::TALKIE_MSG_PING);
						json_message.remove_identity();
						if (json_message.get_nth_value_type(0) == ValueType::TALKIE_VT_STRING) {
							char value_name[TALKIE_NAME_LEN];
							if (json_message.get_nth_value_string(0, value_name, TALKIE_NAME_LEN)) {
								json_message.set_to_name(value_name);
							}
						} else if (json_message.get_nth_value_type(0) == ValueType::TALKIE_VT_INTEGER) {
							json_message.set_to_channel((uint8_t)json_message.get_nth_value_number(0));
						} else {	// Removes the original TO
							json_message.remove_to();	// Without TO works as broadcast
						}
						json_message.remove_nth_value(0);
						json_message.set_from_name(talker.get_name());	// Avoids the swapping

						// 3. Sends the message LOCALLY
						json_message.set_broadcast_value(BroadcastValue::TALKIE_BC_LOCAL);
						// No need to transmit the message, the normal ROGER reply does that for us!
					}
					break;

					case 1:
					{
						ping = true;

						// 1. Start by collecting info from message
						 json_message.get_from_name(_original_talker);
						_trace_message_timestamp = json_message.get_identity();

						// 2. Repurpose it to be a SELF PING
						json_message.set_message_value(MessageValue::TALKIE_MSG_PING);
						json_message.remove_identity();	// Makes sure a new IDENTITY is set
						json_message.set_from_name(talker.get_name());	// Avoids swapping

						// 3. Sends the message to myself
						json_message.set_broadcast_value(BroadcastValue::TALKIE_BC_SELF);
						// No need to transmit the message, the normal ROGER reply does that for us!
					}
					break;
					
					case 2:
					{
						ping = true;

						// 1. Start by setting the Action fields
						if (json_message.get_nth_value_type(0) == ValueType::TALKIE_VT_STRING) {
							char value_name[TALKIE_NAME_LEN];
							if (json_message.get_nth_value_string(0, value_name, TALKIE_NAME_LEN)) {
								json_message.set_to_name(value_name);
							}
						} else if (json_message.get_nth_value_type(0) == ValueType::TALKIE_VT_INTEGER) {
							json_message.set_to_channel((uint8_t)json_message.get_nth_value_number(0));
						} else {
							return false;
						}
						if (json_message.get_nth_value_type(1) == ValueType::TALKIE_VT_STRING) {
							char action_name[TALKIE_NAME_LEN];
							if (json_message.get_nth_value_string(1, action_name, TALKIE_NAME_LEN)) {
								json_message.set_action_name(action_name);
							}
						} else if (json_message.get_nth_value_type(1) == ValueType::TALKIE_VT_INTEGER) {
							json_message.set_action_index((uint8_t)json_message.get_nth_value_number(1));
						} else {
							return false;
						}
						json_message.remove_nth_value(0);
						json_message.set_message_value(MessageValue::TALKIE_MSG_CALL);

						// 2. Collect info from message
						json_message.get_from_name(_original_talker);
						_trace_message_timestamp = json_message.get_identity();

						// 3. Repurpose message with new targets
						json_message.remove_identity();
						json_message.set_from_name(talker.get_name());	// Avoids the swapping

						// 4. Sends the message LOCALLY
						json_message.set_broadcast_value(BroadcastValue::TALKIE_BC_LOCAL);
						// No need to transmit the message, the normal ROGER reply does that for us!
					}
					break;
				}
			}
		}
		return ping;
	}


    void _echo(JsonTalker& talker, JsonMessage& json_message, MessageValue message_value, TalkerMatch talker_match) override {
		(void)talker_match;	// Silence unused parameter warning
        (void)message_value;	// Silence unused parameter warning
		
		#ifdef SPY_MANIFESTO_DEBUG
		RecoveryMessage original_message = talker.getRecoveryMessage();
		Serial.print(F("\t\t\tSpy::echo1: "));
		json_message.write_to(Serial);
		Serial.print(" | ");
		Serial.println((int)original_message.message.get_message_value());
		#endif

		// In condition to calculate the delay right away, no need to extra messages
		uint16_t actual_time = static_cast<uint16_t>(millis());
		uint16_t message_time = json_message.get_timestamp();	// must have
		uint16_t time_delay = actual_time - message_time;
		json_message.set_nth_value_number(0, time_delay);
		char from_name[TALKIE_NAME_LEN];
		json_message.get_from_name(from_name);
		json_message.set_nth_value_string(1, from_name);

		// Prepares headers for the original REMOTE sender
		json_message.set_to_name(_original_talker);
		json_message.set_from_name(talker.get_name());

		// Emulates the REMOTE original call
		json_message.set_identity(_trace_message_timestamp);

		// It's already an ECHO message, it's because of that that entered here
		// Finally answers to the REMOTE caller by repeating all other json fields
		json_message.set_broadcast_value(BroadcastValue::TALKIE_BC_REMOTE);
		talker.transmitToRepeater(json_message);
	}


    void _error(JsonTalker& talker, JsonMessage& json_message, ErrorValue error_value, TalkerMatch talker_match) override {
		(void)talker;		// Silence unused parameter warning
		(void)error_value;	// Silence unused parameter warning
		(void)talker_match;	// Silence unused parameter warning

		char temp_string[TALKIE_MAX_LEN];
		json_message.get_from_name(temp_string);
		Serial.print( temp_string );
        Serial.print(" - ");
		
		ValueType value_type = json_message.get_nth_value_type(0);
		switch (value_type) {

			case ValueType::TALKIE_VT_STRING:
				json_message.get_nth_value_string(0, temp_string);
				Serial.println(temp_string);
			break;
			
			case ValueType::TALKIE_VT_INTEGER:
				Serial.println(json_message.get_nth_value_number(0));
			break;
			
			default:
            	Serial.println(F("Empty echo received!"));
			break;
		}
    }

};


#endif // SPY_MANIFESTO_HPP
