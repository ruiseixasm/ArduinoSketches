# JsonTalkie - Broadcast Sockets

Multiple sockets that can be used with the [JsonTalkie](https://github.com/ruiseixasm/JsonTalkie) software by implementing its `BroadcastSocket` interface.
You can see many examples of sockets right in [this same folder](https://github.com/ruiseixasm/JsonTalkie/tree/main/src/sockets).

## Implementation
You can always implement your own socket, by extending the `BroadcastSocket` class. To do so, you must override the following methods:
```
	const char* class_name() const override { return "YourSocketName"; }
	void _receive() override {}
	bool _send(const JsonMessage& json_message) override {}
```
The methods above should follow these basic rules:
- In the `_receive` method you must create a `JsonMessage` and write on it or deserialize on it the data received, on that `new_message` you shall always call the methods `_validate_json` and `_process_checksum`. After that, it should be called the method `_startTransmission` to process the received data. See example bellow for details;
- In the `_send` method you must read from the `json_message` buffer with the help of the methods `_read_buffer` and `_get_length`.
### Example
Here is an example of such implementation for the Serial protocol:
```
#ifndef SOCKET_SERIAL_HPP
#define SOCKET_SERIAL_HPP

#include "../BroadcastSocket.h"

class SocketSerial : public BroadcastSocket {
public:

    const char* class_name() const override { return "SocketSerial"; }

protected:

    // Singleton accessor
    SocketSerial() : BroadcastSocket() {}

	JsonMessage _json_message;
	bool _reading_serial = false;


    void _receive() override {
    
		while (Serial.available()) {
			char c = Serial.read();

			char* message_buffer = _json_message._write_buffer();
			if (_reading_serial) {

				size_t message_length = _json_message._get_length();
				if (message_length < TALKIE_BUFFER_SIZE) {
					if (c == '}' && message_length && message_buffer[message_length - 1] != '\\') {

						_reading_serial = false;

						if (_json_message._append('}') && _json_message._validate_json()) {
							_json_message._process_checksum();	// Has to validate and process the checksum
							_startTransmission(_json_message);
						}
						return;
					} else if (!_json_message._append(c)) {
						return;
					}
				} else {
					_reading_serial = false;
					_json_message._set_length(0);	// Reset to start writing
				}
			} else if (c == '{') {
				
				_json_message._set_length(0);
				_reading_serial = true;
				_json_message._append('{');
			}
		}
    }


    bool _send(const JsonMessage& json_message) override {

		const char* message_buffer = json_message._read_buffer();
		size_t message_length = json_message._get_length();
		return Serial.write(message_buffer, message_length) == message_length;
    }


public:
    // Move ONLY the singleton instance method to subclass
    static SocketSerial& instance() {

        static SocketSerial instance;
        return instance;
    }

};

#endif // SOCKET_SERIAL_HPP
```
## Ethernet
### BroadcastSocket_EtherCard
Lightweight socket intended to be used with low memory boards like the Uno and the Nano, for the ethernet module `ENC28J60`.
This library has the limitation of not being able to send *unicast* messages, all its responses are in *broadcast* mode, so, one
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
Not being able to work in unicast, means that the replies take have high latency or can even not reach the source
if the message was sent via Wi-Fi.
```
>>> talk uno
>>> talk uno
	[talk uno]           	   Arduino Uno
>>> ping uno
>>> ping uno
	[ping uno]           	   106
>>>
```
Above, besides a long ping, 106 milliseconds, some replies weren't even received, so, this library shouldn't be
used via Wi-Fi if replies are critical. Must be noted however, that this huge latency is mainly due to the broadcasted reply,
meaning that the received unicast message by the Uno board has the usual small latency.
### Changed_EthernetENC
This is the best library to be used with the module `ENC28J60`, it requires more memory so it shall be used with an
Arduino Mega or any other with similar amount of memory or more.

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
Contrary to the socket implementation `BroadcastSocket_EtherCard` described above, this socket does *unicast*, so,
it can be used via Wi-Fi too without the latency referred above, 4 instead of 106 milliseconds.
```
>>> talk spy
	[talk spy]           	   I'm a Spy and I spy the talkers' pings
>>> ping spy
	[ping spy]           	   4
>>>
```
### BroadcastSocket_Ethernet
This socket is intended to be used with the original [Arduino Ethernet board](https://docs.arduino.cc/retired/shields/arduino-ethernet-shield-without-poe-module/),
or other that has the chip `W5500` or `W5100`. Take note that depending on the board, the pins may vary, so read the
following notes first.
```
Arduino communicates with both the W5100 and SD card using the SPI bus (through the ICSP header).
This is on digital pins 10, 11, 12, and 13 on the Uno and pins 50, 51, and 52 on the Mega. On both boards,
pin 10 is used to select the W5100 and pin 4 for the SD card. These pins cannot be used for general I/O.
On the Mega, the hardware SS pin, 53, is not used to select either the W5100 or the SD card,
but it must be kept as an output or the SPI interface won't work.
```
Contrary to the socket implementation `BroadcastSocket_EtherCard` described above, this socket does *unicast*, so,
it can be used via Wi-Fi too without the latency referred above, 6 instead of 106 milliseconds.
```
>>> talk mega
	[talk mega]          	   I'm a Mega talker
>>> ping mega
	[ping mega]          	   6
>>>
```
## WiFi
### BroadcastESP_WiFi
This socket is intended to be used with the boards ESP8266 or ESP32 that come with WiFi out of the box.

Contrary to the socket implementation `BroadcastSocket_EtherCard` described above, this socket does *unicast*, so,
it can be used via Wi-Fi too without the latency referred above, 3 instead of 106 milliseconds.
```
>>> talk blue
	[talk blue]          	   I turn led Blue on and off
>>> ping blue
	[ping blue]          	   3
>>>
```
One thing to take into consideration though, is that the fist ping can be sent in Broadcast mode because the first command is also
used to get the Talker address and only then the messages are sent in unicast mode afterwards. So, in the example above,
the `talk blue` command resulted in the association of the IP address with the Talker's name enabling the unicast `ping blue` command.
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





