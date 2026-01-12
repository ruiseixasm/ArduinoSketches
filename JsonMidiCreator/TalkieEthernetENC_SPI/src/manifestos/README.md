# JsonTalkie - Talker Manifestos

Multiple manifestos that can be used with the [JsonTalkie](https://github.com/ruiseixasm/JsonTalkie)
software by implementing its `TalkerManifesto` interface.
You can see many examples of manifestos right in [this same folder](https://github.com/ruiseixasm/JsonTalkie/tree/main/src/manifestos).
## Implementation
You can create many Manifestos to different scenarios by extending the `TalkerManifesto` class.
To do so, you must override, at least, the following methods:
```
	const char* class_name() const override { return "LedManifesto"; }
	const Action* _getActionsArray() const override { return calls; }
    uint8_t _actionsCount() const override { return sizeof(calls)/sizeof(Action); }
```
As it's possible to be seen, these methods relate to the member variable `calls`,
that depending on the number of the actions, it shall follow the following structure:
```
    Action calls[3] = {
		{"on", "Turns led ON"},
		{"off", "Turns led OFF"},
		{"actions", "Returns the number of triggered Actions"}
    };
```
This structure is a list of `Action` objects with a name and a description of the respective `Action`.
Their id is given by the respective position in the array. You must make sure the number of actions is typed,
in the case above, that is represented with `[3]`.

### Example
Here is a bare minimum example of such implementation that controls a Blue LED:
```
#ifndef BLUE_MANIFESTO_HPP
#define BLUE_MANIFESTO_HPP

#include "../TalkerManifesto.hpp"

class BlueManifesto : public TalkerManifesto {
public:

    const char* class_name() const override { return "BlueManifesto"; }

    BlueManifesto(uint8_t led_pin) : TalkerManifesto(), _led_pin(led_pin)
	{
		pinMode(_led_pin, OUTPUT);
	}	// Constructor

    ~BlueManifesto()
	{	// ~TalkerManifesto() called automatically here
		digitalWrite(_led_pin, LOW);
		pinMode(_led_pin, INPUT);
	}	// Destructor


protected:

	const uint8_t _led_pin;
    bool _is_led_on = false;	// keep track of the led state, by default it's off
    uint16_t _total_calls = 0;

    Action calls[3] = {
		{"on", "Turns led ON"},
		{"off", "Turns led OFF"},
		{"actions", "Returns the number of triggered Actions"}
    };
    
public:
    
    const Action* _getActionsArray() const override { return calls; }
    uint8_t _actionsCount() const override { return sizeof(calls)/sizeof(Action); }


    // Index-based operations (simplified examples)
    bool _actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) override {
        (void)talker;		// Silence unused parameter warning
    	(void)talker_match;	// Silence unused parameter warning
		
		if (index >= sizeof(calls)/sizeof(Action)) return false;
		
		// Actual implementation would do something based on index
		switch(index) {

			case 0:
			{
				if (!_is_led_on) {
					digitalWrite(_led_pin, HIGH);
					_is_led_on = true;
					_total_calls++;
					return true;
				} else {
					json_message.set_nth_value_string(0, "Already On!");
					return false;
				}
			}
			break;

			case 1:
			{
				if (_is_led_on) {
				digitalWrite(_led_pin, LOW);
					_is_led_on = false;
					_total_calls++;
				} else {
					json_message.set_nth_value_string(0, "Already Off!");
					return false;
				}
				return true;
			}
			break;
			
            case 2:
				json_message.set_nth_value_number(0, _total_calls);
                return true;
            break;
				
            default: return false;
		}
		return false;
	}

};


#endif // BLUE_MANIFESTO_HPP
```

## Other methods
You can go beyond the bare minimum above, here are more methods that can be overridden.
### _loop
The `_loop` method is called constantly by the `MessageRepeater` class, so, you can program it
in the same fashion as you would for the typical `loop` function in the Arduino, but here, specifically for the Talker.

Here is an example of such programming, a buzzer that takes it's time that with the loop
avoids the usage of `delay` calls that interrupt the normal flow of the program.
```
	void _loop(JsonTalker& talker) override {
        (void)talker;		// Silence unused parameter warning
		if ((uint16_t)millis() - _buzz_start > _buzz_duration_ms) {
			#ifdef BUZZ_PIN
			digitalWrite(BUZZ_PIN, LOW);
			#endif
		}
	}
```
### _echo
The `_echo` method is used to process the message responses, echoes, to the original ones sent.
So, a talker not only is able to receive messages as also is able to send new ones created
by the Talker Manifesto, and thus, is able to process the respective responses, echoes, in this method.

Here is an example of a Manifesto that processes the responses to its generated pings.
```
void Spy::_echo(JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) {
    (void)talker_match;	// Silence unused parameter warning

	Original original_message = talker.get_original();
	
	// In condition to calculate the delay right away, no need to extra messages
	uint16_t actual_time = static_cast<uint16_t>(millis());
	uint16_t message_time = json_message.get_timestamp();	// must have
	uint16_t time_delay = actual_time - message_time;
	json_message.set_nth_value_number(0, time_delay);
	json_message.set_nth_value_string(1, json_message.get_from_name());

	// Prepares headers for the original REMOTE sender
	json_message.set_to_name(_original_talker.c_str());
	json_message.set_from_name(talker.get_name());

	// Emulates the REMOTE original call
	json_message.set_identity(_original_message.identity);

	// It's already an ECHO message, it's because of that that entered here
	// Finally answers to the REMOTE caller by repeating all other json fields
	json_message.set_broadcast_value(BroadcastValue::TALKIE_BC_REMOTE);
	talker.transmitToRepeater(json_message);
}
```
### _error
The `_error` method can be used for report the errors returned by other Talkers,
in this example the Talker Manifesto results in the printing of those errors received.
```
void Spy::_error(JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) {
    (void)talker;		// Silence unused parameter warning
    (void)talker_match;	// Silence unused parameter warning

	ValueType value_type = json_message.get_nth_value_type(0);
	switch (value_type) {

		case ValueType::TALKIE_VT_STRING:
			Serial.println(json_message.get_nth_value_string(0));
			break;
		
		case ValueType::TALKIE_VT_INTEGER:
			Serial.println(json_message.get_nth_value_number(0));
			break;
		
		default:
			Serial.println(F("Empty error received!"));
			break;
	}
}
```
### _noise
The `_noise` as it implies processes messages that lost their usual meaning, this is, their usual meaning was
removed and become noise. One typical scenario is a message that has a bad checksum and thus becomes 'noisy',
this gives the opportunity to the Talker process it as such. This means that only the 'noisy' messages without
any error associated end up triggering this method. So, the remaining 'noisy' messages can be processed here
in the way you see fit.

This is an example that just prints the first value of the 'noisy' message.
```
void Spy::_noise(JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) {
    (void)talker;		// Silence unused parameter warning
    (void)json_message;	// Silence unused parameter warning
    (void)talker_match;	// Silence unused parameter warning

	Serial.println(json_message.get_nth_value_string(0));
}
```
