# JsonTalkie - Broadcast Sockets

Multiple sockets that can be used with the [JsonTalkie](https://github.com/ruiseixasm/JsonTalkie) software by implementing its `BroadcastSocket` interface.

## Ethernet
### Changed_EthernetENC
Lightweight socket intended to be used with low memory boards like the Uno and the Nano, for the ethernet module `ENC28J60`.
This library has the limitation of not being able to send unicast messages, all its responses are in broadcast mode, so, one
way to avoid it is to mute the Talker and in that way the `call` commands on it don't overload the network.
### BroadcastSocket_EtherCard
This is the best library to be used with the module `ENC28J60`, it requires more memory so it shall be used with an Arduino Mega
or any other with similar amount of memory or more.

One of it's problems is that relies in a change done to the original library, as it's one that doesn't respond to broadcasted UDP packets
out of the box. To do so, you can pick up the already tweaked one [here](https://github.com/ruiseixasm/JsonTalkie/tree/main/extras/Changed_EthernetENC),
or you can tweak the original yourself, by changing these lines in the file `Enc28J60Network.cpp`.

Comment the existing lines and add the new one as bellow:
```
    // Pattern matching disabled – not needed for broadcast
    // writeRegPair(EPMM0, 0x303f);
    // writeRegPair(EPMCSL, 0xf7f9);
    writeReg(ERXFCON, ERXFCON_UCEN | ERXFCON_CRCEN | ERXFCON_BCEN);
```
After the changes above the library will start to receive broadcasted UDP messages too.
For more details check the data sheet of the chip [ENC28J60](https://ww1.microchip.com/downloads/en/devicedoc/39662a.pdf)
- **BroadcastSocket_Ethernet**
This socket is intended to be used with the original [Arduino Ethernet board](https://docs.arduino.cc/retired/shields/arduino-ethernet-shield-without-poe-module/),
or other that has the chip `W5500` or `W5100`. Take note that depending on the board, the pins may vary, so read the following note first.
```
Arduino communicates with both the W5100 and SD card using the SPI bus (through the ICSP header).
This is on digital pins 10, 11, 12, and 13 on the Uno and pins 50, 51, and 52 on the Mega. On both boards,
pin 10 is used to select the W5100 and pin 4 for the SD card. These pins cannot be used for general I/O.
On the Mega, the hardware SS pin, 53, is not used to select either the W5100 or the SD card,
but it must be kept as an output or the SPI interface won't work.
```

## SPI
SPI is among the most difficult protocols to implement, mainly in the Slave side. This happens because the SPI Arduino Slave is software based and the interrupts
are done per byte and also they take their time, around, 100us. So, a message of 90 bytes long will take around 1 millisecond to be transmitted, this means that,
it is best to target the talkers by name (unicast) than by channel (broadcast).
### ESP32 Master
#### SPI_ESP_Arduino_Master

#### SPI_Arduino_Arduino_Master_Multiple
### Arduino Master
#### SPI_Arduino_Arduino_Master_Single
### Arduino Slave
#### SPI_ESP_Arduino_Slave


## Serial
- **SocketSerial**





