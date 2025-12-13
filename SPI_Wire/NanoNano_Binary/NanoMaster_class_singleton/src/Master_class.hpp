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
#ifndef MASTER_CLASS_HPP
#define MASTER_CLASS_HPP

#include <SPI.h>
#include <ArduinoJson.h>    // Include ArduinoJson Library


// #define BROADCAST_SPI_DEBUG_1
// #define BROADCAST_SPI_DEBUG_2


// Pin definitions
extern const int BUZZ_PIN;  // Declare as external (defined elsewhere)

#define BROADCAST_SOCKET_BUFFER_SIZE 128

// To make this value the minimum possible, always place the setting SPDR on top in the Slave code (SPDR =)
#define send_delay_us 10
#define receive_delay_us 10

class Master_class
{
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


	const char* command_on = "{'t':'Nano','m':2,'n':'ON','f':'Talker-9f','i':3540751170,'c':24893}";
	const uint8_t length_on = 68;
	const char* command_off = "{'t':'Nano','m':2,'n':'OFF','f':'Talker-9f','i':3540751170,'c':24893}";
	const uint8_t length_off = 69;


protected:

    char _receiving_buffer[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};
    char _sending_buffer[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};

    int _ss_pin = 10;

	// Just create a pointer to the existing SPI object
	SPIClass* _spi_instance = &SPI;  // Alias pointer

	// JsonDocument in the stack makes sure its memory is released (NOT GLOBAL)
	#if ARDUINOJSON_VERSION_MAJOR >= 7
	JsonDocument message_doc;
	#else
	StaticJsonDocument<BROADCAST_SOCKET_BUFFER_SIZE> message_doc;
	#endif

	
    bool sendSPI(uint8_t length, int ss_pin) {
        uint8_t size = 0;
		
		#ifdef BROADCAST_SPI_DEBUG_1
		Serial.print(F("\tSending on pin: "));
		Serial.println(ss_pin);
		#endif

		if (length > BROADCAST_SOCKET_BUFFER_SIZE) {
			
			#ifdef BROADCAST_SPI_DEBUG_1
			Serial.println(F("\tlength > BROADCAST_SOCKET_BUFFER_SIZE"));
			#endif

			return false;
		}

		if (length > 0) {	// Don't send empty strings
			
			uint8_t c; // Avoid using 'char' while using values above 127

			for (uint8_t s = 0; size == 0 && s < 3; s++) {
		
				digitalWrite(ss_pin, LOW);
				delayMicroseconds(5);

				// Asks the Slave to start receiving
				c = _spi_instance->transfer(RECEIVE);

				if (c != VOID) {

					delayMicroseconds(12);	// Makes sure it's processed by the slave (12us) (critical path)
					c = _spi_instance->transfer(_sending_buffer[0]);

					if (c == READY) {	// Makes sure the Slave it's ready first
					
						for (uint8_t i = 1; i < length; i++) {
							delayMicroseconds(send_delay_us);
							c = _spi_instance->transfer(_sending_buffer[i]);	// Receives the echoed _sending_buffer[i - 1]
							if (c < 128) {
								// Offset of 2 picks all mismatches than an offset of 1
								if (i > 1 && c != _sending_buffer[i - 2]) {
									#ifdef BROADCAST_SPI_DEBUG_1
									Serial.print(F("\t\tERROR: Char mismatch at index: "));
									Serial.println(i - 2);
									#endif
									break;
								}
							} else if (c == FULL) {
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
						c = _spi_instance->transfer(LAST);
						if (c == _sending_buffer[length - 2]) {
							delayMicroseconds(12);    // Makes sure the Status Byte is sent
							c = _spi_instance->transfer(END);
							if (c == _sending_buffer[length - 1]) {	// Last char
								size = length + 1;	// Just for error catch
								// Makes sure Slave does the respective sets
								for (uint8_t end_r = 0; c != DONE && end_r < 3; end_r++) {	// Makes sure the receiving buffer of the Slave is deleted, for sure!
									delayMicroseconds(10);
									c = _spi_instance->transfer(END);
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
					} else if (c == BUSY) {
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tBUSY: Slave is busy, waiting a little."));
						#endif
						if (s < 2) {
							delay(2);	// Waiting 2ms
						}
					} else if (c == ERROR) {
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tERROR: Slave sent a transmission ERROR"));
						#endif
					} else if (c == RECEIVE) {
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
					_spi_instance->transfer(ERROR);
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\t\t\tSent ERROR back to the Slave"));
					#endif
				}

				delayMicroseconds(5);
				digitalWrite(ss_pin, HIGH);

				if (size > 0) {
					#ifdef BROADCAST_SPI_DEBUG_1
					if (size > 1) {
						Serial.print(F("\tSent message: "));
						Serial.write(_sending_buffer, size - 1);
						Serial.println();
						Serial.print(F("\tSent size: "));
						Serial.println(size - 1);
					} else {
						Serial.println(F("\tNothing sent"));
					}
					#endif
				} else {
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.print(F("\t\t\tRETRY: Message NOT successfully sent on try: "));
					Serial.println(s + 1);
					Serial.println(F("\t\t\t\tBUZZER activated for 10ms!"));
					#endif
					digitalWrite(BUZZ_PIN, HIGH);
					delay(10);  // Buzzer on for 10ms
					digitalWrite(BUZZ_PIN, LOW);
					delay(500);
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


    uint8_t receiveSPI(int ss_pin) {
        uint8_t size = 0;
        uint8_t c; // Avoid using 'char' while using values above 127

		#ifdef BROADCAST_SPI_DEBUG_2
		Serial.print(F("\tReceiving on pin: "));
		Serial.println(ss_pin);
		#endif

        for (uint8_t r = 0; size == 0 && r < 3; r++) {
    
            digitalWrite(ss_pin, LOW);
            delayMicroseconds(5);

            // Asks the Slave to start receiving
            c = _spi_instance->transfer(SEND);
			
			if (c != VOID) {

				delayMicroseconds(12);	// Makes sure it's processed by the slave (12us) (critical path)
				c = _spi_instance->transfer('\0');   // Dummy char to get the ACK

				if (c == READY) {	// Makes sure the Slave it's ready first
					
					delayMicroseconds(receive_delay_us);
					c = _spi_instance->transfer('\0');   // Dummy char to get the ACK
					_receiving_buffer[0] = c;

					// Starts to receive all chars here
					for (uint8_t i = 1; c < 128 && i < BROADCAST_SOCKET_BUFFER_SIZE; i++) { // First i isn't a char byte
						delayMicroseconds(receive_delay_us);
						c = _spi_instance->transfer(_receiving_buffer[i - 1]);
						_receiving_buffer[i] = c;
						size = i;
					}
					if (c == LAST) {
						delayMicroseconds(receive_delay_us);    // Makes sure the Status Byte is sent
						c = _spi_instance->transfer(_receiving_buffer[size]);  // Replies the last char to trigger END in return
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tReceived LAST"));
						#endif
						if (c == END) {
							delayMicroseconds(10);	// Makes sure the Status Byte is sent
							c = _spi_instance->transfer(END);	// Replies the END to confirm reception and thus Slave buffer deletion
							for (uint8_t end_s = 0; c != DONE && end_s < 3; end_s++) {	// Makes sure the sending buffer of the Slave is deleted, for sure!
								delayMicroseconds(10);
								c = _spi_instance->transfer(END);
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
					} else if (size == BROADCAST_SOCKET_BUFFER_SIZE) {
						delayMicroseconds(12);    // Makes sure the Status Byte is sent
						_spi_instance->transfer(FULL);
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
				} else if (c == NONE) {
					size = 1; // Nothing received
					#ifdef BROADCAST_SPI_DEBUG_2
					Serial.println(F("\t\tThere is nothing to be received"));
					#endif
				} else if (c == ERROR) {
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\t\tERROR: Transmission ERROR received from Slave"));
					#endif
				} else if (c == SEND) {
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\t\tERROR: Received SEND back, need to retry"));
					#endif
				} else if (c == FULL) {
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
					_spi_instance->transfer(ERROR);    // Results from ERROR or NACK send by the Slave and makes Slave reset to NONE
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
                    Serial.print(F("\tReceived message: "));
					Serial.write(_receiving_buffer, size - 1);
                    Serial.println();
					Serial.print(F("\tReceived size: "));
					Serial.println(size - 1);
                } else {
                	#ifdef BROADCAST_SPI_DEBUG_2
                    Serial.println(F("\tNothing received"));
                	#endif
                }
                #endif
            } else {
                #ifdef BROADCAST_SPI_DEBUG_1
                Serial.print(F("\t\t\tRETRY: Message NOT successfully received on try: "));
                Serial.println(r + 1);
                Serial.println(F("\t\t\t\tBUZZER activated for 2 x 10ms!"));
                #endif
                digitalWrite(BUZZ_PIN, HIGH);
                delay(10);  // Buzzer on for 10ms
                digitalWrite(BUZZ_PIN, LOW);
                delay(200);
                digitalWrite(BUZZ_PIN, HIGH);
                delay(10);  // Buzzer on for 10ms
                digitalWrite(BUZZ_PIN, LOW);
                delay(500);
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
            c = _spi_instance->transfer(ACK);

			if (c != VOID) {

				delayMicroseconds(12);
				c = _spi_instance->transfer(ACK);  // When the response is collected
				
				if (c == ACK) {
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


    Master_class(int ss_pin = 10) {

        _ss_pin = ss_pin;

        // Initialize SPI
        SPI.begin();
        SPI.setClockDivider(SPI_CLOCK_DIV4);    // Only affects the char transmission
        SPI.setDataMode(SPI_MODE0);
        SPI.setBitOrder(MSBFIRST);  // EXPLICITLY SET MSB FIRST! (OTHERWISE is LSB)
        // Enable the SS pin
        pinMode(ss_pin, OUTPUT);
        digitalWrite(ss_pin, HIGH);
        // Sets the class SS pin
    }

    ~Master_class() {
        // This returns the pin to exact power-on state:
        pinMode(_ss_pin, INPUT);
        digitalWrite(_ss_pin, LOW);  // Important: disables any pull-up
    }


public:

	// // Master_class master_class = Master_class(SS_PIN);  // WRONG!
	// Master_class& master_class = Master_class::instance(SS_PIN);  // CORRECT!
	
    // Move ONLY the singleton instance method to subclass
    static Master_class& instance(int ss_pin = 10) {

        static Master_class instance(ss_pin);
        return instance;
    }

    
    bool test() {

        uint8_t length = 0;

		//
        // ON cycle
		//

		// Use memcpy to copy the value of 'command_on' into '_sending_buffer'
    	memcpy(_sending_buffer, command_on, length_on);
        if (!sendSPI(length_on, _ss_pin)) return false;

        DeserializationError error = deserializeJson(message_doc, _sending_buffer, length_on);
        if (error) {
			#ifdef BROADCAST_SPI_DEBUG_1
			Serial.print(F("ERROR (ON): "));
			Serial.print(error.c_str());
			Serial.print(F(" - Code: "));
			Serial.println(error.code());
			
			// What ArduinoJson sees
			Serial.print(F("Input length: "));
			Serial.println(strlen(_sending_buffer));
			
			// Test with simple JSON
			const char* test_simple = "{}";
			JsonDocument test_doc;
			DeserializationError test_err = deserializeJson(test_doc, test_simple);
			Serial.print(F("Simple {} test: "));
			Serial.println(test_err.c_str());
			#endif
            return false;
        }
        JsonObject json_message = message_doc.as<JsonObject>();
		const char* command_name = json_message["n"].as<const char*>();

		if(strcmp(command_name, "ON") != 0) {
			
			Serial.print(F("Didn't send 'ON' to Slave as expected"));
			return false;
		}


        delay(1000);
        length = receiveSPI(_ss_pin);
        if (!length) return false;

        error = deserializeJson(message_doc, _receiving_buffer, length);
        if (error) {
			#ifdef BROADCAST_SPI_DEBUG_1
			Serial.println(F("ERROR (OFF): Failed to deserialize JSON"));
			#endif
            return false;
        }
        json_message = message_doc.as<JsonObject>();
		command_name = json_message["n"].as<const char*>();

		if(strcmp(command_name, "OK_ON") != 0) {
			
			Serial.print(F("Didn't receive 'OK_ON' from Slave as expected"));
			return false;
		}

        if (receiveSPI(_ss_pin)) return false;
        if (sendSPI(0, _ss_pin)) return false;
        delay(1000);
        if (receiveSPI(_ss_pin)) return false;	// Testing that receiving nothing also works
        if (receiveSPI(_ss_pin)) return false;


		//
        // OFF cycle
		//

		// Copy safely (takes into consideration the '\0' char)
		strlcpy(_sending_buffer, command_off, BROADCAST_SOCKET_BUFFER_SIZE);
        if (!sendSPI(length_off, _ss_pin)) return false;
		
        error = deserializeJson(message_doc, _sending_buffer, length_off);
        if (error) {
			#ifdef BROADCAST_SPI_DEBUG_1
			Serial.println(F("ERROR (OFF): Failed to deserialize JSON"));
			#endif
            return false;
        }
        json_message = message_doc.as<JsonObject>();
		command_name = json_message["n"].as<const char*>();

		if(strcmp(command_name, "OFF") != 0) {
			
			Serial.print(F("Didn't send 'OFF' to Slave as expected"));
			return false;
		}

        delay(1000);
        length = receiveSPI(_ss_pin);
        if (!length) return false;

        error = deserializeJson(message_doc, _receiving_buffer, length);
        if (error) {
			#ifdef BROADCAST_SPI_DEBUG_1
			Serial.println(F("ERROR (OFF): Failed to deserialize JSON"));
			#endif
            return false;
        }
        json_message = message_doc.as<JsonObject>();
		command_name = json_message["n"].as<const char*>();

		if(strcmp(command_name, "OK_OFF") != 0) {
			
			Serial.print(F("Didn't receive 'OK_OFF' from Slave as expected"));
			return false;
		}

        if (receiveSPI(_ss_pin)) return false;
        if (sendSPI(0, _ss_pin)) return false;
        delay(1000);
        if (receiveSPI(_ss_pin)) return false;
        if (receiveSPI(_ss_pin)) return false;

        return true;
    }


    bool ready() {
        return acknowledgeSPI(_ss_pin);
    }


	void yellow_on() {

		const char* yellow_on = "{'t':'Nano','m':2,'n':'YELLOW_ON','f':'Talker-9f','i':3540751170,'c':24893}";
		const uint8_t length_yellow = 75;

		// Copy safely (takes into consideration the '\0' char)
		strlcpy(_sending_buffer, yellow_on, BROADCAST_SOCKET_BUFFER_SIZE);

		sendSPI(length_yellow, _ss_pin);
	}

	void yellow_off() {

		const char* yellow_off = "{'t':'Nano','m':2,'n':'YELLOW_OFF','f':'Talker-9f','i':3540751170,'c':24893}";
		const uint8_t length_yellow = 76;

		// Copy safely (takes into consideration the '\0' char)
		strlcpy(_sending_buffer, yellow_off, BROADCAST_SOCKET_BUFFER_SIZE);

		sendSPI(length_yellow, _ss_pin);
	}

};


#endif // MASTER_CLASS_HPP
