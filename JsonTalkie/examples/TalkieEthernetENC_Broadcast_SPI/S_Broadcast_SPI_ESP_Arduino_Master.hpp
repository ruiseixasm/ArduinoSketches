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
#ifndef SPI_ESP_ARDUINO_MASTER_HPP
#define SPI_ESP_ARDUINO_MASTER_HPP


#include <BroadcastSocket.h>
#include <SPI.h>

// #define BROADCAST_SPI_DEBUG
// #define BROADCAST_SPI_DEBUG_1
// #define BROADCAST_SPI_DEBUG_2
// #define BROADCAST_SPI_DEBUG_NEW
// #define BROADCAST_SPI_DEBUG_TIMING


#define ENABLE_DIRECT_ADDRESSING


#define send_delay_us 10
#define receive_delay_us 10
#define TALKIE_MAX_NAMES 8


class S_Broadcast_SPI_ESP_Arduino_Master : public BroadcastSocket {
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

	SPIClass* _spi_instance;  // Pointer to SPI instance
	bool _initiated = false;
    int* _ss_pins;
    uint8_t _ss_pins_count = 0;
	char _names[TALKIE_MAX_NAMES][TALKIE_NAME_LEN];
	uint8_t _actual_ss_pin_i = 0;


    // Constructor
    S_Broadcast_SPI_ESP_Arduino_Master(int* ss_pins, uint8_t ss_pins_count) : BroadcastSocket() {
            
        	_ss_pins = ss_pins;
        	_ss_pins_count = ss_pins_count;
			for (uint8_t ss_pin_i = 0; ss_pin_i < _ss_pins_count && ss_pin_i < TALKIE_MAX_NAMES; ++ss_pin_i) {
				_names[ss_pin_i][0] = '\0';
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

		// Too many SPI sends to the Slaves asking if there is something to send will overload them, so, a timeout is needed
		static uint16_t timeout = (uint16_t)micros();

		if (micros() - timeout > 250) {
			timeout = (uint16_t)micros();

			if (_initiated) {

				#ifdef BROADCAST_SPI_DEBUG_TIMING
				_reference_time = millis();
				#endif

				JsonMessage new_message;
				char* message_buffer = new_message._write_buffer();

				for (uint8_t ss_pin_i = 0; ss_pin_i < _ss_pins_count; ss_pin_i++) {
					
					size_t length = receiveSPI(_ss_pins[ss_pin_i], message_buffer);
					if (length > 0) {
						
						new_message._set_length(length);
						_actual_ss_pin_i = ss_pin_i;
						_startTransmission(new_message);
					}
				}
			}
		}
    }


	void _showMessage(const JsonMessage& json_message) override {

		if (_names[_actual_ss_pin_i][0] == '\0') {
			json_message.get_from_name(_names[_actual_ss_pin_i]);
			
			#ifdef BROADCAST_SPI_DEBUG
			Serial.print(F("\t_showMessage1: Saved actual named pin index i: "));
			Serial.println(_actual_ss_pin_i);
			Serial.print(F("\t_showMessage2: Saved name: "));
			Serial.println(_names[_actual_ss_pin_i]);
			Serial.print(F("\t_showMessage3: Concerning actual pin: "));
			Serial.println(_ss_pins[_actual_ss_pin_i]);
			#endif

		}
	}

    
    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    bool _send(const JsonMessage& json_message) override {

		if (_initiated) {
			
			#ifdef BROADCAST_SPI_DEBUG_TIMING
			Serial.print("\n\tsend: ");
			#endif
				
			#ifdef BROADCAST_SPI_DEBUG_TIMING
			_reference_time = millis();
			#endif

			#ifdef BROADCAST_SPI_DEBUG
			Serial.print(F("\t\t\t\t\tsend1: Sent message: "));
			Serial.write(json_message._read_buffer(), json_message.get_length());
			Serial.print(F("\n\t\t\t\t\tsend2: Sent length: "));
			Serial.println(json_message.get_length());
			#endif
			
			#ifdef ENABLE_DIRECT_ADDRESSING

			bool as_reply = false;
			char to_name[TALKIE_NAME_LEN];
			if (json_message.get_to_name(to_name)) {

				#ifdef BROADCAST_SPI_DEBUG
				Serial.println(F("\t\t\t\t\tsend3: json_message TO is a String"));
				Serial.print(F("\t\t\t\t\tsend4: Message name TO: "));
				Serial.println(to_name);
				#endif

				for (uint8_t ss_pin_i = 0; ss_pin_i < _ss_pins_count && ss_pin_i < TALKIE_MAX_NAMES; ++ss_pin_i) {
					
					#ifdef BROADCAST_SPI_DEBUG
					Serial.print(F("\t\t\t\t\tsend5: Comparing to the name: "));
					Serial.println(_names[ss_pin_i]);
					#endif

					if (strcmp(to_name, _names[ss_pin_i]) == 0) {
						as_reply = true;
						_actual_ss_pin_i = ss_pin_i;
						break;
					}
				}
			} else {
				#ifdef BROADCAST_SPI_DEBUG
				Serial.println(F("\t\t\t\t\tsend3: json_message TO is NOT a String or doesn't exist"));
				#endif
			}
			#ifdef BROADCAST_SPI_DEBUG_TIMING
			Serial.print(" | ");
			Serial.print(millis() - _reference_time);
			#endif

			const char* message_buffer = json_message._read_buffer();
			size_t message_length = json_message.get_length();

			if (as_reply) {
				sendSPI(_ss_pins[_actual_ss_pin_i], message_buffer, message_length);

				#ifdef BROADCAST_SPI_DEBUG
				Serial.print(F("\t\t\t\t\tsend4: --> Directly sent for the received pin --> "));
				Serial.println(_actual_ss_pin_i);
				#endif

			} else {    // Broadcast mode
				for (uint8_t ss_pin_i = 0; ss_pin_i < _ss_pins_count; ss_pin_i++) {
					sendSPI(_ss_pins[ss_pin_i], message_buffer, message_length);
				}
				
				#ifdef BROADCAST_SPI_DEBUG
				Serial.println(F("\t\t\t\t\tsend4: --> Broadcast sent to all pins -->"));
				#endif

			}
			#else
			for (uint8_t ss_pin_i = 0; ss_pin_i < _ss_pins_count; ss_pin_i++) {
				sendSPI(_ss_pins[ss_pin_i], message_buffer, message_length);
			}
			#ifdef BROADCAST_SPI_DEBUG
			Serial.println(F("\t\t\t\t\tsend4: --> Broadcast sent to all pins -->"));
			#endif
			#endif

			#ifdef BROADCAST_SPI_DEBUG_TIMING
			Serial.print(" | ");
			Serial.print(millis() - _reference_time);
			#endif

			return true;
		}
        return false;
    }


	bool initiate() {
		
		if (_spi_instance) {

			// Configure SPI settings
			_spi_instance->setDataMode(SPI_MODE0);
			_spi_instance->setBitOrder(MSBFIRST);  // EXPLICITLY SET MSB FIRST!
			_spi_instance->setFrequency(4000000); 	// 4MHz if needed (optional)
			// ====================================================
			
			// ================== CONFIGURE SS PINS ==================
			// CRITICAL: Configure all SS pins as outputs and set HIGH
			for (uint8_t i = 0; i < _ss_pins_count; i++) {
				pinMode(_ss_pins[i], OUTPUT);
				digitalWrite(_ss_pins[i], HIGH);
				delayMicroseconds(10); // Small delay between pins
			}

			_initiated = true;
			for (uint8_t ss_pin_i = 0; ss_pin_i < _ss_pins_count; ss_pin_i++) {
				if (!acknowledgeSPI(_ss_pins[ss_pin_i])) {
					_initiated = false;
					break;
				}
			}
		}

		#ifdef BROADCAST_SPI_DEBUG
		if (_initiated) {
			Serial.print(class_description());
			Serial.println(": initiate1: Socket initiated!");

			Serial.print(F("\tinitiate2: Total SS pins connected: "));
			Serial.println(_ss_pins_count);
			Serial.print(F("\t\tinitiate3: SS pins: "));
			
			for (uint8_t ss_pin_i = 0; ss_pin_i < _ss_pins_count; ss_pin_i++) {
				Serial.print(_ss_pins[ss_pin_i]);
				Serial.print(F(", "));
			}
			Serial.println();
		} else {
			Serial.println("initiate1: Socket NOT initiated!");
		}
	
		#endif

		return _initiated;
	}

public:

    // Move ONLY the singleton instance method to subclass
    static S_Broadcast_SPI_ESP_Arduino_Master& instance(int* ss_pins, uint8_t ss_pins_count) {
        static S_Broadcast_SPI_ESP_Arduino_Master instance(ss_pins, ss_pins_count);

        return instance;
    }

	// The Socket class description shouldn't be greater than 35 chars
	// {"m":7,"f":"","s":3,"b":1,"t":"","i":58485,"0":1,"1":"","2":11,"c":11266} <-- 128 - (73 + 2*10) = 35
    const char* class_description() const override { return "SPI_ESP_Arduino_Master"; }


    virtual void begin(SPIClass* spi_instance) {
		
		_spi_instance = spi_instance;
		initiate();
    }
};



#endif // SPI_ESP_ARDUINO_MASTER_HPP
