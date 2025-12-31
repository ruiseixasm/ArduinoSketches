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
#ifndef BROADCAST_SOCKET_SPI_ESP_ARDUINO_MASTER_VSPI_HPP
#define BROADCAST_SOCKET_SPI_ESP_ARDUINO_MASTER_VSPI_HPP


#include <SPI.h>
#include "../BroadcastSocket.h"

// #define BROADCAST_SPI_DEBUG


#define ENABLE_DIRECT_ADDRESSING


// ================== VSPI PIN DEFINITIONS ==================
// VSPI pins for ESP32
#define VSPI_MOSI 23	// GPIO23 for VSPI MOSI
#define VSPI_MISO 19    // GPIO19 for VSPI MISO
#define VSPI_SCK  18    // GPIO18 for VSPI SCK
#define VSPI_SS   5     // GPIO5 for VSPI SCK
// SS pin can be any GPIO - kept as parameter
// ==========================================================


#define send_delay_us 5
#define receive_delay_us 10


class BroadcastSocket_SPI_ESP_Arduino_Master_VSPI : public BroadcastSocket {
public:

    enum StatusByte : uint8_t {
        ACK     = 0xF0, // Acknowledge
        NACK    = 0xF1, // Not acknowledged
        READY   = 0xF2, // Slave is ready
        BUSY    = 0xF3, // Tells the Master to wait a little
        RECEIVE = 0xF4, // Asks the receiver to start receiving
        SEND    = 0xF5, // Asks the receiver to start sending
        NONE    = 0xF6, // Means nothing to send
        START   = 0xF7, // Start of transmission
        END     = 0xF8, // End of transmission
		LAST	= 0xF9,	// Asks for the last char
		DONE	= 0xFA,	// Marks the action as DONE
        ERROR   = 0xFB, // Error frame
        FULL    = 0xFC, // Signals the buffer as full
        
        VOID    = 0xFF  // MISO floating (0xFF) → no slave responding
    };


protected:

	bool _initiated = false;
    int* _ss_pins;
    uint8_t _ss_pins_count = 0;
    uint8_t _actual_ss_pin = VSPI_SS;
	String _from_name = "";

    // Needed for the compiler, the base class is the one being called though
    // ADD THIS CONSTRUCTOR - it calls the base class constructor
    BroadcastSocket_SPI_ESP_Arduino_Master_VSPI(int* ss_pins, uint8_t ss_pins_count)
		: BroadcastSocket() {
            
        	_ss_pins = ss_pins;
			_ss_pins_count = ss_pins_count;
            _max_delay_ms = 0;  // SPI is sequencial, no need to control out of order packages
            // // Initialize devices control object (optional initial setup)
            // devices_ss_pins["initialized"] = true;
        }

    
    // Specific methods associated to Arduino SPI as Master

    uint8_t sendString(int ss_pin) {
        uint8_t length = 0;	// No interrupts, so, not volatile
		
		#ifdef BROADCAST_SPI_DEBUG
		Serial.print("\tSending on pin: ");
		Serial.println(ss_pin);
		#endif

		if (_sending_buffer[0] != '\0') {	// Don't send empty strings
			
			uint8_t c; // Avoid using 'char' while using values above 127

			for (uint8_t s = 0; length == 0 && s < 3; s++) {
		
				digitalWrite(ss_pin, LOW);
				delayMicroseconds(5);

				// Asks the Slave to start receiving
				c = SPI.transfer(RECEIVE);

				if (c != VOID) {

					delayMicroseconds(10);	// Makes sure ACK is set by the slave (10us) (critical path)
					c = SPI.transfer(_sending_buffer[0]);

					if (c == ACK) {
					
						for (uint8_t i = 1; i < BROADCAST_SOCKET_BUFFER_SIZE; i++) {
							delayMicroseconds(send_delay_us);
							c = SPI.transfer(_sending_buffer[i]);	// Receives the echoed _sending_buffer[i - 1]
							if (c != _sending_buffer[i - 1]) {    // Includes NACK situation
								#ifdef BROADCAST_SPI_DEBUG
								Serial.print("\t\tChar miss match at: ");
								Serial.println("i");
								#endif
								length = 0;
								break;
							}
							if (_sending_buffer[i] == '\0') {
								delayMicroseconds(10);    // Makes sure the Status Byte is sent
								c = SPI.transfer(END);
								if (c == '\0') {
									#ifdef BROADCAST_SPI_DEBUG
									Serial.println("\t\tSend completed");
									#endif
									length = i;
									break;
								} else {
									#ifdef BROADCAST_SPI_DEBUG
									Serial.println("\t\tLast char '\\0' NOT received");
									#endif
									length = 0;
									break;
								}
							}
						}
					} else {
						#ifdef BROADCAST_SPI_DEBUG
						Serial.println("\t\tDevice ACK NOT received");
						#endif
						length = 1; // Nothing to be sent
					}

				} else {
					#ifdef BROADCAST_SPI_DEBUG
					Serial.println("\t\tReceived VOID");
					#endif
					length = 1; // Avoids another try
				}

				if (length == 0) {
					delayMicroseconds(10);    // Makes sure the Status Byte is sent
					SPI.transfer(ERROR);
					// _received_buffer[0] = '\0'; // Implicit char
				}

				delayMicroseconds(5);
				digitalWrite(ss_pin, HIGH);

				if (length > 0) {
					#ifdef BROADCAST_SPI_DEBUG
					if (length > 1) {
						Serial.print("Command successfully sent: ");
						Serial.println(_sending_buffer);
					} else {
						Serial.println("\tNothing sent");
					}
					#endif
				} else {
					#ifdef BROADCAST_SPI_DEBUG
					Serial.print("\t\tCommand NOT successfully sent on try: ");
					Serial.println(s + 1);
					#endif
				}
			}

        } else {
			#ifdef BROADCAST_SPI_DEBUG
			Serial.println("\t\tNothing to be sent");
			#endif
			length = 1; // Nothing to be sent
		}

        if (length > 0)
            length--;   // removes the '\0' from the length as final value
        return length;
    }


    uint8_t receiveString(int ss_pin) {
        uint8_t length = 0;	// No interrupts, so, not volatile
        uint8_t c; // Avoid using 'char' while using values above 127

		#ifdef BROADCAST_SPI_DEBUG
		Serial.print("\tReceiving on pin: ");
		Serial.println(ss_pin);
		#endif

        for (uint8_t r = 0; length == 0 && r < 3; r++) {
    
            digitalWrite(ss_pin, LOW);
            delayMicroseconds(5);

            // Asks the Slave to start receiving
            c = SPI.transfer(SEND);
			
			if (c != VOID) {

				delayMicroseconds(10);	// Makes sure ACK or NONE is set by the slave (10us) (critical path)
				c = SPI.transfer('\0');   // Dummy char to get the ACK

				if (c == ACK) { // Makes sure there is an Acknowledge first
					
					// Starts to receive all chars here
					for (uint8_t i = 0; i < BROADCAST_SOCKET_BUFFER_SIZE; i++) { // First i isn't a char byte
						delayMicroseconds(receive_delay_us);
						if (i > 0) {    // The first response is discarded because it's unrelated (offset by 1 communication)
							c = SPI.transfer(_received_buffer[length]);    // length == i - 1
							if (c < 128) {   // Only accepts ASCII chars
								// Avoids increment beyond the real string size
								if (_received_buffer[length] != '\0') {    // length == i - 1
									_received_buffer[++length] = c;        // length == i (also sets '\0')
								}
							} else if (c == END) {
								delayMicroseconds(10);    // Makes sure the Status Byte is sent
								SPI.transfer(END);  // Replies the END to confirm reception and thus Slave buffer deletion
								#ifdef BROADCAST_SPI_DEBUG
								Serial.println("\t\tReceive completed");
								#endif
								length++;   // Adds up the '\0' uncounted char
								break;
							} else {    // Includes NACK (implicit)
								#ifdef BROADCAST_SPI_DEBUG
								Serial.print("\t\t\tNo END or Char, instead, received: ");
								Serial.println(c, HEX);
								#endif
								length = 0;
								break;
							}
						} else {
							c = SPI.transfer('\0');   // Dummy char to get the ACK
							length = 0;
							if (c < 128) {	// Makes sure it's an ASCII char
								_received_buffer[0] = c;
							} else {
								#ifdef BROADCAST_SPI_DEBUG
								Serial.println("\t\tNot a valid ASCII char (< 128)");
								#endif
								break;
							}
						}
					}
				} else if (c == NONE) {
					#ifdef BROADCAST_SPI_DEBUG
					Serial.println("\t\tThere is nothing to be received");
					#endif
					_received_buffer[0] = '\0';
					length = 1; // Nothing received
					break;
				} else {
					#ifdef BROADCAST_SPI_DEBUG
					Serial.println("\t\tSlave ACK or NONE was NOT received");
					#endif
					length = 1; // Nothing received
					break;
				}

				if (length == 0) {
					delayMicroseconds(10);    // Makes sure the Status Byte is sent
					SPI.transfer(ERROR);    // Results from ERROR or NACK send by the Slave and makes Slave reset to NONE
					_received_buffer[0] = '\0'; // Implicit char
					#ifdef BROADCAST_SPI_DEBUG
					Serial.println("\t\t\tSent ERROR");
					#endif
				}

			} else {
				#ifdef BROADCAST_SPI_DEBUG
				Serial.println("\t\tReceived VOID");
				#endif
				length = 1; // Avoids another try
			}

            delayMicroseconds(5);
            digitalWrite(ss_pin, HIGH);

            if (length > 0) {
                #ifdef BROADCAST_SPI_DEBUG
                if (length > 1) {
                    Serial.print("Received message: ");
					Serial.write(_received_buffer, _received_length);
                    Serial.println();
                } else {
                    Serial.println("\tNothing received");
                }
                #endif
            } else {
                #ifdef BROADCAST_SPI_DEBUG
                Serial.print("\t\tMessage NOT successfully received on try: ");
                Serial.println(r + 1);
                #endif
            }
        }

        if (length > 0)
            length--;   // removes the '\0' from the length as final value
        return length;
    }


    bool acknowledgeReady(int ss_pin) {
        uint8_t c; // Avoid using 'char' while using values above 127
        bool acknowledge = false;

		#ifdef BROADCAST_SPI_DEBUG
		Serial.print("\tAcknowledging on pin: ");
		Serial.println(ss_pin);
		#endif

        for (uint8_t a = 0; !acknowledge && a < 3; a++) {
    
            digitalWrite(ss_pin, LOW);
            delayMicroseconds(5);

            // Asks the Slave to acknowledge readiness
            c = SPI.transfer(ACK);

			if (c != VOID) {

				delayMicroseconds(10);
				c = SPI.transfer(ACK);  // When the response is collected
				
				if (c == READY) {
                	#ifdef BROADCAST_SPI_DEBUG
                	Serial.println("\t\tAcknowledge with READY");
					#endif
					acknowledge = true;
				}
				#ifdef BROADCAST_SPI_DEBUG
				else {
					Serial.println("\t\tNOT acknowledge");
				}
				#endif
			}
            #ifdef BROADCAST_SPI_DEBUG
			else {
                Serial.println("\t\tReceived VOID");
			}
			#endif

            delayMicroseconds(5);
            digitalWrite(ss_pin, HIGH);

        }

        #ifdef BROADCAST_SPI_DEBUG
        if (acknowledge) {
            Serial.println("Slave is ready!");
        } else {
            Serial.println("Slave is NOT ready!");
        }
        #endif

        return acknowledge;
    }


	// Allows the overriding class to peek at the received JSON message
	bool receivedJsonMessage(const JsonMessage& json_message) override {

		if (BroadcastSocket::receivedJsonMessage(json_message)) {
			_from_name = json_message.get_from_name();
			return true;
		}
		return false;
	}

    
    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    bool send(const JsonMessage& json_message) override {

		if (_initiated && BroadcastSocket::send(json_message)) {	// Very important pre processing !!

			bool as_reply = json_message.is_to_name(_from_name.c_str());

			#ifdef ENABLE_DIRECT_ADDRESSING
			if (as_reply) {
				sendString(_actual_ss_pin);
			} else {    // Broadcast mode
				for (uint8_t ss_pin_i = 0; ss_pin_i < _talker_count; ss_pin_i++) {
					sendString(_ss_pins[ss_pin_i]);
				}
			}
			#else
			for (uint8_t ss_pin_i = 0; ss_pin_i < _talker_count; ss_pin_i++) {
				sendString(_ss_pins[ss_pin_i]);
			}
			#endif

			_sending_length = 0;	// Deletes sending buffer
			return true;
		}
        return false;
    }

	bool initiate() {
		
		// Configure SPI settings
		SPI.setDataMode(SPI_MODE0);
		SPI.setBitOrder(MSBFIRST);  // EXPLICITLY SET MSB FIRST!
		SPI.setFrequency(4000000); 	// 4MHz if needed (optional)
		// ====================================================
        
		// ================== CONFIGURE SS PINS ==================
		// CRITICAL: Configure all SS pins as outputs and set HIGH
		for (uint8_t i = 0; i < _talker_count; i++) {
			pinMode(_ss_pins[i], OUTPUT);
			digitalWrite(_ss_pins[i], HIGH);
			delayMicroseconds(10); // Small delay between pins
		}

		_initiated = true;
		for (uint8_t ss_pin_i = 0; ss_pin_i < _talker_count; ss_pin_i++) {
			if (!acknowledgeReady(_ss_pins[ss_pin_i])) {
				_initiated = false;
				break;
			}
		}
		
		#ifdef BROADCAST_SPI_DEBUG
		if (_initiated) {
			Serial.println("Socket initiated!");
		} else {
			Serial.println("Socket NOT initiated!");
		}
		#endif
		return _initiated;
	}

public:

    // Move ONLY the singleton instance method to subclass
    static BroadcastSocket_SPI_ESP_Arduino_Master_VSPI& instance(int* ss_pins, uint8_t ss_pins_count) {
        static BroadcastSocket_SPI_ESP_Arduino_Master_VSPI instance(ss_pins, ss_pins_count);

        return instance;
    }

    const char* class_name() const override { return "BroadcastSocket_SPI_ESP_Arduino_Master_VSPI"; }


    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    size_t receive() override {

		if (_initiated) {

			// Need to call homologous method in super class first
			uint8_t length = BroadcastSocket::receive(); // Very important to do or else it may stop receiving !!

			for (uint8_t ss_pin_i = 0; ss_pin_i < _talker_count; ss_pin_i++) {
				length = receiveString(_ss_pins[ss_pin_i]);
				if (length > 0) {
					_actual_ss_pin = _ss_pins[ss_pin_i];
					_received_length = length;
					triggerTalkers();
				}
			}
			// Makes sure the _received_buffer is deleted with 0
			_received_length = 0;
		}
        return 0;   // Receives are all called internally in this method
    }


    virtual void begin() {
		
		#ifdef BROADCAST_SPI_DEBUG
		Serial.println("Pins set for VSPI:");
		Serial.print("\tVSPI_SCK: ");
		Serial.println(VSPI_SCK);
		Serial.print("\tVSPI_MISO: ");
		Serial.println(VSPI_MISO);
		Serial.print("\tVSPI_MOSI: ");
		Serial.println(VSPI_MOSI);
		Serial.print("\tVSPI_SS: ");
		Serial.println(VSPI_SS);
		#endif

		// ================== INITIALIZE HSPI ==================
		// Initialize SPI with VSPI pins: SCK=18, MISO=19, MOSI=23
		// This method signature is only available in ESP32 Arduino SPI library!
		SPI.begin(VSPI_SCK, VSPI_MISO, VSPI_MOSI);
		
		initiate();
    }
};

#endif // BROADCAST_SOCKET_SPI_ESP_ARDUINO_MASTER_VSPI_HPP
