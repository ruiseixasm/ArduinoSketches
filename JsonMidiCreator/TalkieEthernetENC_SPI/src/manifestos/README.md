# JsonTalkie - Talker Manifestos

Multiple manifestos that can be used with the [JsonTalkie](https://github.com/ruiseixasm/JsonTalkie) software by implementing its `TalkerManifesto` interface.

## Implementation
You can create many Manifestos to different scenarios by extending the `TalkerManifesto` class. To do so, you must override, at least, the following methods:
```
	const char* class_name() const override { return "LedManifesto"; }
	const Action* _getActionsArray() const override { return calls; }
    uint8_t _actionsCount() const override { return sizeof(calls)/sizeof(Action); }
```
As it's possible to be seen, these methods relate to the member variable `calls`, that depending on the number of the actions, it shall follow the following structure:
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
You can go beyond the bare minimum above, here are more methods that may be overridden.
### _loop


### _echo



### _error



### _noise




Lightweight socket intended to be used with low memory boards like the Uno and the Nano, for the ethernet module `ENC28J60`.
This library has the limitation of not being able to send unicast messages, all its responses are in broadcast mode, so, one
way to avoid it is to mute the Talker and in that way the `call` commands on it don't overload the network.
```
>>> list nano
        [call nano 0|buzz]         Buzz for a while
        [call nano 1|ms]           Gets and sets the buzzing duration
>>> call nano 0
        [call nano 0]              roger
>>> system nano mute 1
        [system nano mute]         1
>>> call nano 0
>>>
```
### Changed_EthernetENC
This is the best library to be used with the module `ENC28J60`, it requires more memory so it shall be used with an Arduino Mega
or any other with similar amount of memory or more.

One of it's problems is that relies in a change done to the original library, as it's one that doesn't respond to broadcasted UDP packets
out of the box. To do so, you can pick up the already changed one [here](https://github.com/ruiseixasm/JsonTalkie/tree/main/extras/Changed_EthernetENC),
or you can tweak the original yourself, by changing these lines in the file `Enc28J60Network.cpp`.

Comment the existing lines and add the new one as bellow:
```
    // Pattern matching disabled – not needed for broadcast
    // writeRegPair(EPMM0, 0x303f);
    // writeRegPair(EPMCSL, 0xf7f9);
    writeReg(ERXFCON, ERXFCON_UCEN | ERXFCON_CRCEN | ERXFCON_BCEN);
```
After these changes the library will start to receive broadcasted UDP messages too.
For more details check the data sheet of the chip [ENC28J60](https://ww1.microchip.com/downloads/en/devicedoc/39662a.pdf).
### BroadcastSocket_Ethernet
This socket is intended to be used with the original [Arduino Ethernet board](https://docs.arduino.cc/retired/shields/arduino-ethernet-shield-without-poe-module/),
or other that has the chip `W5500` or `W5100`. Take note that depending on the board, the pins may vary, so read the following notes first.
```
Arduino communicates with both the W5100 and SD card using the SPI bus (through the ICSP header).
This is on digital pins 10, 11, 12, and 13 on the Uno and pins 50, 51, and 52 on the Mega. On both boards,
pin 10 is used to select the W5100 and pin 4 for the SD card. These pins cannot be used for general I/O.
On the Mega, the hardware SS pin, 53, is not used to select either the W5100 or the SD card,
but it must be kept as an output or the SPI interface won't work.
```

## SPI
SPI is among the most difficult protocols to implement, mainly in the Slave side. This happens because the SPI Arduino Slave is software based and the interrupts
are done per byte and also they take their time, around, 12us. So, a message of 90 bytes long will take around 1 millisecond to be transmitted, this means that,
it is best to target the talkers by name (unicast) than by channel (broadcast) to avoid repeating a single message among multiple Slave sockets.
### ESP32 Master
#### SPI_ESP_Arduino_Master
This Socket allows the communication centered in a single ESP32 master board to many Arduino slave boards.
### Arduino Master
#### SPI_Arduino_Arduino_Master_Multiple
This Socket allows the communication centered in a single Arduino master board to many Arduino slave boards.
#### SPI_Arduino_Arduino_Master_Single
This Socket allows the communication centered in a single Arduino master board to another single Arduino slave board.
### Arduino Slave
#### SPI_Arduino_Slave
This Socket is targeted to Arduino boards intended to be used as SPI Slaves.

## Serial
### SocketSerial
This is the simplest Socket of all, and it's not a Broadcast Socket at all, given that the Serial protocol is one-to-one, so, it's main purpose is just that, for one board to communicate with other, ideal for local communication in the same platform.

It may be used for testing too, by allowing a direct connection with a computer. Just make sure you disable any Serial print as those
may interfere with the normal flow of communication if formatted in the same way as a json string.





