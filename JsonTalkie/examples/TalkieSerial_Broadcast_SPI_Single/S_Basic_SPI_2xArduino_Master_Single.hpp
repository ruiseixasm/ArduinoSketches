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
#ifndef SPI_ARDUINO_ARDUINO_MASTER_SINGLE_HPP
#define SPI_ARDUINO_ARDUINO_MASTER_SINGLE_HPP


#include <BroadcastSocket.h>
#include <SPI.h>

// #define BROADCAST_SPI_DEBUG
// #define BROADCAST_SPI_DEBUG_1
// #define BROADCAST_SPI_DEBUG_2
// #define BROADCAST_SPI_DEBUG_NEW
// #define BROADCAST_SPI_DEBUG_TIMING


#define send_delay_us 10
#define receive_delay_us 10


#define TALKIE_MAX_NAMES 8


class S_Basic_SPI_2xArduino_Master_Single : public BroadcastSocket {
public:

    enum StatusByte : uint8_t {
        TALKIE_SB_ACK		= 0xF0, // Acknowledge
        TALKIE_SB_NACK		= 0xF1, // Not acknowledged
        TALKIE_SB_READY   	= 0xF2, // Slave is ready
        TALKIE_SB_BUSY   	= 0xF3, // Tells the Master to wait a little
        TALKIE_SB_RECEIVE	= 0xF4, // Asks the receiver to start receiving
        TALKIE_SB_SEND    	= 0xF5, // Asks the receiver to start sending
        TALKIE_SB_NONE    	= 0xF6, // Means nothing to send
        TALKIE_SB_START   	= 0xF7, // Start of transmission
        TALKIE_SB_END     	= 0xF8, // End of transmission
		TALKIE_SB_LAST		= 0xF9,	// Asks for the last char
		TALKIE_SB_DONE		= 0xFA,	// Marks the action as DONE
        TALKIE_SB_ERROR   	= 0xFB, // Error frame
        TALKIE_SB_FULL    	= 0xFC, // Signals the buffer as full
        
        TALKIE_SB_VOID    	= 0xFF  // MISO floating (0xFF) → no slave responding
    };


	#ifdef BROADCAST_SPI_DEBUG_TIMING
	unsigned long _reference_time = millis();
	#endif

protected:

	SPIClass* const _spi_instance = &SPI;  // Alias pointer
    int _ss_pin = 10;
	// Just create a pointer to the existing SPI object


    // Constructor
    S_Basic_SPI_2xArduino_Master_Single(int ss_pin) : BroadcastSocket() {
		
			_ss_pin = ss_pin;
			if (_spi_instance) {
				// Initialize SPI
				_spi_instance->begin();
				_spi_instance->setClockDivider(SPI_CLOCK_DIV4);    // Only affects the char transmission
				_spi_instance->setDataMode(SPI_MODE0);
				_spi_instance->setBitOrder(MSBFIRST);  // EXPLICITLY SET MSB FIRST! (OTHERWISE is LSB)
				// Enable the SS pin
				pinMode(_ss_pin, OUTPUT);
				digitalWrite(_ss_pin, HIGH);
				// Sets the class SS pin
			}
            _max_delay_ms = 0;  // SPI is sequencial, no need to control out of order packages
        }

    
    // Specific methods associated to Arduino SPI as Master

	
    bool sendSPI(int ss_pin, const char* message_buffer, size_t length) {
        size_t size = 0;	// No interrupts, so, not volatile
		
		#ifdef BROADCAST_SPI_DEBUG_1
		Serial.print(F("\tSending on pin: "));
		Serial.println(ss_pin);
		#endif

		if (length > TALKIE_BUFFER_SIZE) {
			
			#ifdef BROADCAST_SPI_DEBUG_1
			Serial.println(F("\tlength > TALKIE_BUFFER_SIZE"));
			#endif

			return false;
		}

		if (length > 0) {	// Don't send empty strings
			
			uint8_t c; // Avoid using 'char' while using values above 127

			for (uint8_t s = 0; size == 0 && s < 3; s++) {
		
				digitalWrite(ss_pin, LOW);
				delayMicroseconds(5);

				// Asks the Slave to start receiving
				c = _spi_instance->transfer(TALKIE_SB_RECEIVE);

				if (c != TALKIE_SB_VOID) {

					delayMicroseconds(12);	// Makes sure it's processed by the slave (12us) (critical path)
					c = _spi_instance->transfer(message_buffer[0]);

					if (c == TALKIE_SB_READY) {	// Makes sure the Slave it's ready first
					
						for (uint8_t i = 1; i < length; i++) {
							delayMicroseconds(send_delay_us);
							c = _spi_instance->transfer(message_buffer[i]);	// Receives the echoed message_buffer[i - 1]
							if (c < 128) {
								// Offset of 2 picks all mismatches than an offset of 1
								if (i > 1 && c != message_buffer[i - 2]) {
									#ifdef BROADCAST_SPI_DEBUG_1
									Serial.print(F("\t\tERROR: Char mismatch at index: "));
									Serial.println(i - 2);
									#endif
									break;
								}
							} else if (c == TALKIE_SB_FULL) {
								#ifdef BROADCAST_SPI_DEBUG_1
								Serial.println(F("\t\tERROR: Slave buffer overflow"));
								#endif
							} else {
								#ifdef BROADCAST_SPI_DEBUG_1
								Serial.print(F("\t\tERROR: Not an ASCII char at loop: "));
								Serial.println(i);
								#endif
								break;
							}
						}
						// Checks the last 2 chars still to be checked
						delayMicroseconds(12);    // Makes sure the Status Byte is sent
						c = _spi_instance->transfer(TALKIE_SB_LAST);
						if (c == message_buffer[length - 2]) {
							delayMicroseconds(12);    // Makes sure the Status Byte is sent
							c = _spi_instance->transfer(TALKIE_SB_END);
							if (c == message_buffer[length - 1]) {	// Last char
								size = length + 1;	// Just for error catch
								// Makes sure Slave does the respective sets
								for (uint8_t end_r = 0; c != TALKIE_SB_DONE && end_r < 3; end_r++) {	// Makes sure the receiving buffer of the Slave is deleted, for sure!
									delayMicroseconds(10);
									c = _spi_instance->transfer(TALKIE_SB_END);
								}
								#ifdef BROADCAST_SPI_DEBUG_1
								Serial.println(F("\t\tSend completed"));
								#endif
							} else {
								#ifdef BROADCAST_SPI_DEBUG_1
								Serial.print(F("\t\tERROR: Last char mismatch at index: "));
								Serial.println(length - 1);
								#endif
							}
						} else {
							#ifdef BROADCAST_SPI_DEBUG_1
							Serial.print(F("\t\tERROR: Penultimate Char mismatch at index: "));
							Serial.println(length - 2);
							#endif
						}
					} else if (c == TALKIE_SB_BUSY) {
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tBUSY: Slave is busy, waiting a little."));
						#endif
						if (s < 2) {
							delay(2);	// Waiting 2ms
						}
					} else if (c == TALKIE_SB_ERROR) {
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tERROR: Slave sent a transmission ERROR"));
						#endif
					} else if (c == TALKIE_SB_RECEIVE) {
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tERROR: Received RECEIVE back, need to retry"));
						#endif
					} else {
						size = 1;	// Nothing to be sent
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.print(F("\t\tERROR: Device NOT ready wit the reply: "));
						Serial.println(c, HEX);
						#endif
					}

				} else {
					size = 1; // Avoids another try
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\t\tERROR: Received VOID"));
					#endif
				}

				if (size == 0) {
					delayMicroseconds(12);    // Makes sure the Status Byte is sent
					_spi_instance->transfer(TALKIE_SB_ERROR);
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\t\t\tSent ERROR back to the Slave"));
					#endif
				}

				delayMicroseconds(5);
				digitalWrite(ss_pin, HIGH);

				if (size > 0) {
					#ifdef BROADCAST_SPI_DEBUG_1
					if (size > 1) {
						Serial.print("Sent message: ");
						Serial.write(json_message._read_buffer(), length);
						Serial.println();
					} else {
						Serial.println("\tNothing sent");
					}
					#endif
				} else {
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.print("\t\tMessage NOT successfully sent on try: ");
					Serial.println(s + 1);
					#endif
				}
			}

        } else {
			#ifdef BROADCAST_SPI_DEBUG_1
			Serial.println(F("\t\tNothing to be sent"));
			#endif
			size = 1; // Nothing to be sent
		}

        if (size > 1) return true;
        return false;
    }


    size_t receiveSPI(int ss_pin, char* message_buffer, size_t buffer_size = TALKIE_BUFFER_SIZE) {
        size_t size = 0;	// No interrupts, so, not volatile
        uint8_t c;			// Avoid using 'char' while using values above 127

		#ifdef BROADCAST_SPI_DEBUG_2
		Serial.print(F("\tReceiving on pin: "));
		Serial.println(ss_pin);
		#endif

        for (uint8_t r = 0; size == 0 && r < 3; r++) {
    
            digitalWrite(ss_pin, LOW);
            delayMicroseconds(5);

            // Asks the Slave to start receiving
            c = _spi_instance->transfer(TALKIE_SB_SEND);
			
			if (c != TALKIE_SB_VOID) {

				delayMicroseconds(12);	// Makes sure it's processed by the slave (12us) (critical path)
				c = _spi_instance->transfer('\0');   // Dummy char to get the ACK

				if (c == TALKIE_SB_READY) {	// Makes sure the Slave it's ready first
					
					delayMicroseconds(receive_delay_us);
					c = _spi_instance->transfer('\0');   // Dummy char to get the ACK
					message_buffer[0] = c;

					// Starts to receive all chars here
					for (uint8_t i = 1; c < 128 && i < buffer_size; i++) { // First i isn't a char byte
						delayMicroseconds(receive_delay_us);
						c = _spi_instance->transfer(message_buffer[i - 1]);
						message_buffer[i] = c;
						size = i;
					}
					if (c == TALKIE_SB_LAST) {
						delayMicroseconds(receive_delay_us);    // Makes sure the Status Byte is sent
						c = _spi_instance->transfer(message_buffer[size]);  // Replies the last char to trigger END in return
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tReceived LAST"));
						#endif
						if (c == TALKIE_SB_END) {
							delayMicroseconds(10);	// Makes sure the Status Byte is sent
							c = _spi_instance->transfer(TALKIE_SB_END);	// Replies the END to confirm reception and thus Slave buffer deletion
							for (uint8_t end_s = 0; c != TALKIE_SB_DONE && end_s < 3; end_s++) {	// Makes sure the sending buffer of the Slave is deleted, for sure!
								delayMicroseconds(10);
								c = _spi_instance->transfer(TALKIE_SB_END);
							}
							#ifdef BROADCAST_SPI_DEBUG_1
							Serial.println(F("\t\tReceive completed"));
							#endif
							size += 1;	// size equivalent to 'i + 2'
						} else {
							size = 0;	// Try again
							#ifdef BROADCAST_SPI_DEBUG_1
							Serial.println(F("\t\tERROR: END NOT received"));
							#endif
						}
					} else if (size == buffer_size) {
						delayMicroseconds(12);    // Makes sure the Status Byte is sent
						_spi_instance->transfer(TALKIE_SB_FULL);
						size = 1;	// Try no more
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tFULL: Master buffer overflow"));
						#endif
					} else {
						size = 0;	// Try again
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tERROR: Receiving sequence wasn't followed"));
						#endif
					}
				} else if (c == TALKIE_SB_NONE) {
					size = 1; // Nothing received
					#ifdef BROADCAST_SPI_DEBUG_2
					Serial.println(F("\t\tThere is nothing to be received"));
					#endif
				} else if (c == TALKIE_SB_ERROR) {
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\t\tERROR: Transmission ERROR received from Slave"));
					#endif
				} else if (c == TALKIE_SB_SEND) {
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\t\tERROR: Received SEND back, need to retry"));
					#endif
				} else if (c == TALKIE_SB_FULL) {
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\t\tERROR: Slave buffer overflow"));
					#endif
				} else {
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.print(F("\t\tERROR: Device NOT ready, received status message: "));
					Serial.println(c, HEX);
					#endif
					size = 1; // Nothing received
				}

				if (size == 0) {
					delayMicroseconds(12);    // Makes sure the Status Byte is sent
					_spi_instance->transfer(TALKIE_SB_ERROR);    // Results from ERROR or NACK send by the Slave and makes Slave reset to NONE
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\t\t\tSent ERROR back to the Slave"));
					#endif
				}

			} else {
				#ifdef BROADCAST_SPI_DEBUG_1
				Serial.println(F("\t\tReceived VOID"));
				#endif
				size = 1; // Avoids another try
			}

            delayMicroseconds(5);
            digitalWrite(ss_pin, HIGH);

            if (size > 0) {
                #ifdef BROADCAST_SPI_DEBUG_1
                if (size > 1) {
                    Serial.print("Received message: ");
					Serial.write(message_buffer, size - 1);
                    Serial.println();
                } else {
                	#ifdef BROADCAST_SPI_DEBUG_2
                    Serial.println("\tNothing received");
                	#endif
                }
                #endif
            } else {
                #ifdef BROADCAST_SPI_DEBUG_1
                Serial.print("\t\tMessage NOT successfully received on try: ");
                Serial.println(r + 1);
                #endif
            }
        }

        if (size > 0) size--;
        return size;
    }


    bool acknowledgeSPI(int ss_pin) {
        uint8_t c; // Avoid using 'char' while using values above 127
        bool acknowledge = false;

		#ifdef BROADCAST_SPI_DEBUG_1
		Serial.print(F("\tAcknowledging on pin: "));
		Serial.println(ss_pin);
		#endif

        for (uint8_t a = 0; !acknowledge && a < 3; a++) {
    
            digitalWrite(ss_pin, LOW);
            delayMicroseconds(5);

            // Asks the Slave to acknowledge readiness
            c = _spi_instance->transfer(TALKIE_SB_ACK);

			if (c != TALKIE_SB_VOID) {

				delayMicroseconds(12);
				c = _spi_instance->transfer(TALKIE_SB_ACK);  // When the response is collected
				
				if (c == TALKIE_SB_ACK) {
                	#ifdef BROADCAST_SPI_DEBUG_1
                	Serial.println(F("\t\tAcknowledged"));
					#endif
					acknowledge = true;
				}
				#ifdef BROADCAST_SPI_DEBUG_1
				else {
					Serial.println(F("\t\tNOT acknowledged"));
				}
				#endif
			}
            #ifdef BROADCAST_SPI_DEBUG_1
			else {
                Serial.println(F("\t\tReceived VOID"));
			}
			#endif

            delayMicroseconds(5);
            digitalWrite(ss_pin, HIGH);
        }

        #ifdef BROADCAST_SPI_DEBUG_1
        if (acknowledge) {
            Serial.println(F("Slave is ready!"));
        } else {
            Serial.println(F("Slave is NOT ready!"));
        }
        #endif

        return acknowledge;
    }


    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    void _receive() override {

		if (_spi_instance) {

			#ifdef BROADCAST_SPI_DEBUG_TIMING
			_reference_time = millis();
			#endif

			JsonMessage new_message;
			char* message_buffer = new_message._write_buffer();
			size_t length = receiveSPI(_ss_pin, message_buffer);

			if (length > 0) {
				
				new_message._set_length(length);
				_startTransmission(new_message);
			}
		}
    }

    
    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    bool _send(const JsonMessage& json_message) override {

		if (_spi_instance) {
			
			#ifdef BROADCAST_SPI_DEBUG_TIMING
			Serial.print("\n\tsend: ");
			#endif
				
			#ifdef BROADCAST_SPI_DEBUG_TIMING
			_reference_time = millis();
			#endif

			#ifdef BROADCAST_SPI_DEBUG
			Serial.print(F("\t\t\t\t\tsend1: Sent message: "));
			Serial.write(_sending_buffer, json_message.get_length());
			Serial.println();
			Serial.print(F("\t\t\t\t\tsend2: Sent length: "));
			Serial.println(json_message.get_length());
			#endif
			
			const char* message_buffer = json_message._read_buffer();
			size_t message_length = json_message.get_length();
			sendSPI(_ss_pin, message_buffer, message_length);

			#ifdef BROADCAST_SPI_DEBUG_TIMING
			Serial.print(" | ");
			Serial.print(millis() - _reference_time);
			#endif

			return true;
		}
        return false;
    }


public:

    // Move ONLY the singleton instance method to subclass
    static S_Basic_SPI_2xArduino_Master_Single& instance(int ss_pin) {
        static S_Basic_SPI_2xArduino_Master_Single instance(ss_pin);

        return instance;
    }

	// The Socket class description shouldn't be greater than 35 chars
	// {"m":7,"f":"","s":3,"b":1,"t":"","i":58485,"0":1,"1":"","2":11,"c":11266} <-- 128 - (73 + 2*10) = 35
    const char* class_description() const override { return "SPI_Arduino_x2_Master_S"; }

};



#endif // SPI_ARDUINO_ARDUINO_MASTER_SINGLE_HPP
