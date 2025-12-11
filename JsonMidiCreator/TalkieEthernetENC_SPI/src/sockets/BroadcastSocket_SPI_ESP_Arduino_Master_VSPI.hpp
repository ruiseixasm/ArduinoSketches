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
#include <ArduinoJson.h>    // Include ArduinoJson Library to be used as a dictionary
#include "../BroadcastSocket.hpp"

// #define BROADCAST_SPI_DEBUG


#define ENABLE_DIRECT_ADDRESSING


// ================== VSPI PIN DEFINITIONS ==================
// VSPI pins for ESP32
#define SPI_MOSI 23    // GPIO23 for VSPI MOSI
#define SPI_MISO 19    // GPIO19 for VSPI MISO
#define SPI_SCK  18    // GPIO18 for VSPI SCK
#define SPI_SS   5     // GPIO5 for VSPI SCK
// SS pin can be any GPIO - kept as parameter
// ==========================================================


#define send_delay_us 5
#define receive_delay_us 10


class BroadcastSocket_SPI_ESP_Arduino_Master_VSPI : public BroadcastSocket {
public:

    enum MessageCode : uint8_t {
        START   = 0xF0, // Start of transmission
        END     = 0xF1, // End of transmission
        ACK     = 0xF2, // Acknowledge
        NACK    = 0xF3, // Not acknowledged
        READY   = 0xF4, // Slave has response ready
        ERROR   = 0xF5, // Error frame
        RECEIVE = 0xF6, // Asks the receiver to start receiving
        SEND    = 0xF7, // Asks the receiver to start sending
        NONE    = 0xF8, // Means nothing to send
        FULL    = 0xF9, // Signals the buffer as full
        
        VOID    = 0xFF  // MISO floating (0xFF) → no slave responding
    };


private:
	bool _initiated = false;
    int* _talkers_ss_pins;
    uint8_t _ss_pins_count = 0;
    uint8_t _actual_ss_pin = SPI_SS;

protected:
    // Needed for the compiler, the base class is the one being called though
    // ADD THIS CONSTRUCTOR - it calls the base class constructor
    BroadcastSocket_SPI_ESP_Arduino_Master_VSPI(JsonTalker** json_talkers, uint8_t talker_count)
        : BroadcastSocket(json_talkers, talker_count) {
            
            _max_delay_ms = 0;  // SPI is sequencial, no need to control out of order packages
            // // Initialize devices control object (optional initial setup)
            // devices_ss_pins["initialized"] = true;
        }

    
    // Specific methods associated to Arduino SPI as Master

    size_t sendString(int ss_pin = SPI_SS) {
        size_t length = 0;	// No interrupts, so, not volatile
		
		if (_sending_buffer[0] != '\0') {	// Don't send empty strings
			
			uint8_t c; // Avoid using 'char' while using values above 127

			for (size_t s = 0; length == 0 && s < 3; s++) {
		
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
								// // There is always some interrupts stacking, avoiding a tailing one makes no difference
								// delayMicroseconds(receive_delay_us);    // Avoids interrupts stacking on Slave side
								c = SPI.transfer(END);
								if (c == '\0') {
									#ifdef BROADCAST_SPI_DEBUG
									Serial.println("\t\tSent completed");
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
					// // There is always some interrupts stacking, avoiding a tailing one makes no difference
					// delayMicroseconds(receive_delay_us);    // Avoids interrupts stacking on Slave side
					SPI.transfer(ERROR);
					// _receiving_buffer[0] = '\0'; // Implicit char
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


    size_t receiveString(int ss_pin = SPI_SS) {
        size_t length = 0;	// No interrupts, so, not volatile
        uint8_t c; // Avoid using 'char' while using values above 127

        for (size_t r = 0; length == 0 && r < 3; r++) {
    
            digitalWrite(ss_pin, LOW);
            delayMicroseconds(5);

            // Asks the Slave to start receiving
            c = SPI.transfer(SEND);
			
			if (c != VOID) {

				delayMicroseconds(receive_delay_us);
				c = SPI.transfer('\0');   // Dummy char to get the ACK

				if (c == ACK) { // Makes sure there is an Acknowledge first
					
					// Starts to receive all chars here
					for (uint8_t i = 0; i < BROADCAST_SOCKET_BUFFER_SIZE; i++) { // First i isn't a char byte
						delayMicroseconds(receive_delay_us);
						if (i > 0) {    // The first response is discarded because it's unrelated (offset by 1 communication)
							c = SPI.transfer(_receiving_buffer[length]);    // length == i - 1
							if (c < 128) {   // Only accepts ASCII chars
								// Avoids increment beyond the real string size
								if (_receiving_buffer[length] != '\0') {    // length == i - 1
									_receiving_buffer[++length] = c;        // length == i (also sets '\0')
								}
							} else if (c == END) {
								// // There is always some interrupts stacking, avoiding a tailing one makes no difference
								// delayMicroseconds(receive_delay_us);    // Avoids interrupts stacking on Slave side
								SPI.transfer(END);  // Replies the END to confirm reception and thus Slave buffer deletion
								#ifdef BROADCAST_SPI_DEBUG
								Serial.println("\t\t\tSent END");
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
								_receiving_buffer[0] = c;
							} else {
								#ifdef BROADCAST_SPI_DEBUG
								Serial.println("\t\tNot a valid ASCII char (< 128)");
								#endif
								break;
							}
						}
					}
				} else {
					#ifdef BROADCAST_SPI_DEBUG
					Serial.println("\t\tDevice ACK NOT received");
					#endif
					length = 1; // Nothing to be sent
					break;
				}

				if (length == 0) {
					// // There is always some interrupts stacking, avoiding a tailing one makes no difference
					// delayMicroseconds(receive_delay_us);    // Avoids interrupts stacking on Slave side
					SPI.transfer(ERROR);    // Results from ERROR or NACK send by the Slave and makes Slave reset to NONE
					_receiving_buffer[0] = '\0'; // Implicit char
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
                    Serial.println(_receiving_buffer);
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


    bool acknowledgeReady(int ss_pin = SPI_SS) {
        uint8_t c; // Avoid using 'char' while using values above 127
        bool acknowledge = false;

        for (size_t a = 0; !acknowledge && a < 3; a++) {
    
            digitalWrite(ss_pin, LOW);
            delayMicroseconds(5);

            // Asks the Slave to acknowledge readiness
            c = SPI.transfer(ACK);

            if (c != VOID) {

                delayMicroseconds(send_delay_us);
                c = SPI.transfer(ACK);  // When the response is collected
                
                if (c == READY) {
                    #ifdef BROADCAST_SPI_DEBUG
                    Serial.println("\t\tReceived READY");
                    #endif
                    acknowledge = true;
                }
                #ifdef BROADCAST_SPI_DEBUG
                else {
                    Serial.println("\t\tDidn't receive READY");
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

    
    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    size_t send(size_t length, bool as_reply = false, uint8_t target_index = 255) override {

		if (_initiated) {

			// Need to call homologous method in super class first
			length = BroadcastSocket::send(length, as_reply); // Very important pre processing !!

			if (length > 0) {
				#ifdef ENABLE_DIRECT_ADDRESSING
				if (as_reply) {
					sendString(_actual_ss_pin);
				} else if (target_index < _ss_pins_count) {
					sendString(_talkers_ss_pins[target_index]);
				} else {    // Broadcast mode
					for (uint8_t ss_pin_i = 0; ss_pin_i < _ss_pins_count; ss_pin_i++) {
						sendString(_talkers_ss_pins[ss_pin_i]);
					}
				}
				#else
				for (uint8_t ss_pin_i = 0; ss_pin_i < _ss_pins_count; ss_pin_i++) {
					sendString(_talkers_ss_pins[ss_pin_i]);
				}
				#endif
			}
			// Makes sure the _sending_buffer is reset with '\0'
			_sending_buffer[0] = '\0';
		}
        return 0;   // Returns 0 because everything is dealt internally in this method
    }

public:

    // Move ONLY the singleton instance method to subclass
    static BroadcastSocket_SPI_ESP_Arduino_Master_VSPI& instance(JsonTalker** json_talkers, uint8_t talker_count) {
        static BroadcastSocket_SPI_ESP_Arduino_Master_VSPI instance(json_talkers, talker_count);
        return instance;
    }

    const char* class_name() const override { return "BroadcastSocket_SPI_ESP_Arduino_Master_VSPI"; }


    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    size_t receive() override {

		if (_initiated) {

			// Need to call homologous method in super class first
			size_t length = BroadcastSocket::receive(); // Very important to do or else it may stop receiving !!

			for (uint8_t ss_pin_i = 0; ss_pin_i < _ss_pins_count; ss_pin_i++) {
				length = receiveString(_talkers_ss_pins[ss_pin_i]);
				if (length > 0) {
					_actual_ss_pin = _talkers_ss_pins[ss_pin_i];
					BroadcastSocket::triggerTalkers(length);
				}
			}
			// Makes sure the _receiving_buffer is reset with '\0'
			_receiving_buffer[0] = '\0';
		}

        return 0;   // Receives are all called internally in this method
    }


    virtual void setup(int* talkers_ss_pins, uint8_t ss_pins_count) {
		
		// ================== INITIALIZE HSPI ==================
		// Initialize SPI with HSPI pins: SCK=14, MISO=12, MOSI=13
		// This method signature is only available in ESP32 Arduino SPI library!
		SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
		
		// Configure SPI settings
		SPI.setClockDivider(SPI_CLOCK_DIV4);    // Only affects the char transmission
		SPI.setDataMode(SPI_MODE0);
		SPI.setBitOrder(MSBFIRST);  // EXPLICITLY SET MSB FIRST!
		// SPI.setFrequency(1000000); // 1MHz if needed (optional)
		// ====================================================
        
        _talkers_ss_pins = talkers_ss_pins;
        _ss_pins_count = ss_pins_count;
		_initiated = true;
    }
};

#endif // BROADCAST_SOCKET_SPI_ESP_ARDUINO_MASTER_VSPI_HPP
