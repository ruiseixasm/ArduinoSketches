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


#define BROADCAST_SPI_DEBUG_1
#define BROADCAST_SPI_DEBUG_2


// Pin definitions
extern const int BUZZ_PIN;  // Declare as external (defined elsewhere)

#define BROADCAST_SOCKET_BUFFER_SIZE 128

// To make this value the minimum possible, always place the setting SPDR on top in the Slave code (SPDR =)
#define send_delay_us 10
#define receive_delay_us 10

class Master_class
{
public:
    
    enum MessageCode : uint8_t {
        START   = 0xF0, // Start of transmission
        END     = 0xF1, // End of transmission
        ACK     = 0xF2, // Acknowledge
        NACK    = 0xF3, // Not acknowledged
        READY   = 0xF4, // Slave is ready
        ERROR   = 0xF5, // Error frame
        RECEIVE = 0xF6, // Asks the receiver to start receiving
        SEND    = 0xF7, // Asks the receiver to start sending
        NONE    = 0xF8, // Means nothing to send
        FULL    = 0xF9, // Signals the buffer as full
        BUSY    = 0xFA, // Tells the Master to wait a little
        
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

	
    uint8_t sendString(uint8_t length, int ss_pin) {
        uint8_t size = 0;	// No interrupts, so, not volatile
		
		#ifdef BROADCAST_SPI_DEBUG_1
		Serial.print(F("\tSending on pin: "));
		Serial.println(ss_pin);
		#endif

		if (_sending_buffer[0] != '\0') {	// Don't send empty strings
			
			uint8_t c; // Avoid using 'char' while using values above 127

			for (uint8_t s = 0; size == 0 && s < 3; s++) {
		
				digitalWrite(ss_pin, LOW);
				delayMicroseconds(5);

				// Asks the Slave to start receiving
				c = _spi_instance->transfer(RECEIVE);

				if (c != VOID) {

					delayMicroseconds(10);	// Makes sure ACK is set by the slave (10us) (critical path)
					c = _spi_instance->transfer(_sending_buffer[0]);

					if (c == READY) {	// Makes sure the Slave it's ready first
					
						for (uint8_t i = 1; i < length; i++) {
							delayMicroseconds(send_delay_us);
							c = _spi_instance->transfer(_sending_buffer[i]);	// Receives the echoed _sending_buffer[i - 1]
							if (c < 128) {
								if (c != _sending_buffer[i - 1]) {    // Includes NACK situation
									#ifdef BROADCAST_SPI_DEBUG_1
									Serial.print(F("\t\tChar miss match at: "));
									Serial.println(i);
									#endif
									size = 0;
									break;
								}
							} else {
								#ifdef BROADCAST_SPI_DEBUG_1
								Serial.print(F("\t\tERROR at: "));
								Serial.println(i);
								#endif
								size = 0;
								break;
							}
						}
						delayMicroseconds(10);    // Makes sure the Status Byte is sent
						c = _spi_instance->transfer(END);
						if (c == _sending_buffer[length - 1]) {	// Last char
							#ifdef BROADCAST_SPI_DEBUG_1
							Serial.println(F("\t\tSend completed"));
							#endif
							size = length;
							break;
						} else {
							#ifdef BROADCAST_SPI_DEBUG_1
							Serial.println(F("\t\tLast char NOT received"));
							#endif
							size = 0;
							break;
						}
					} else if (c == BUSY) {
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tSlave is busy, waiting a little."));
						#endif
						delayMicroseconds(5);
						digitalWrite(ss_pin, HIGH);
						delay(2);	// Waiting 2ms
						continue;
					} else {
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tDevice NOT ready"));
						#endif
						size = 1; // Nothing to be sent
					}

				} else {
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\t\tReceived VOID"));
					#endif
					size = 1; // Avoids another try
				}

				if (size == 0) {
					delayMicroseconds(10);    // Makes sure the Status Byte is sent
					_spi_instance->transfer(ERROR);
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
					Serial.print(F("\t\tMessage NOT successfully sent on try: "));
					Serial.println(s + 1);
					Serial.println(F("\t\tBUZZER activated for 10ms!"));
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

        if (size > 0)
            size--;   // removes the '\0' from the length as final value
        return size;
    }


    uint8_t receiveString(int ss_pin) {
        uint8_t size = 0;	// No interrupts, so, not volatile
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

				delayMicroseconds(10);	// Makes sure ACK or NONE is set by the slave (10us) (critical path)
				c = _spi_instance->transfer('\0');   // Dummy char to get the ACK

				if (c == READY) {	// Makes sure the Slave it's ready first
					
					// Starts to receive all chars here
					for (uint8_t i = 0; i < BROADCAST_SOCKET_BUFFER_SIZE; i++) { // First i isn't a char byte
						delayMicroseconds(receive_delay_us);
						if (i > 0) {    // The first response is discarded because it's unrelated (offset by 1 communication)
							c = _spi_instance->transfer(_receiving_buffer[size]);    // size == i - 1
							if (c < 128) {   // Only accepts ASCII chars
								// Avoids increment beyond the real string size
								if (_receiving_buffer[size] != '\0') {    // size == i - 1
									_receiving_buffer[++size] = c;        // size == i (also sets '\0')
								}
							} else if (c == END) {
								delayMicroseconds(10);    // Makes sure the Status Byte is sent
								_spi_instance->transfer(END);  // Replies the END to confirm reception and thus Slave buffer deletion
								#ifdef BROADCAST_SPI_DEBUG_1
								Serial.println(F("\t\tReceive completed"));
								#endif
								size++;   // Adds up the '\0' uncounted char
								break;
							} else {    // Includes NACK (implicit)
								#ifdef BROADCAST_SPI_DEBUG_1
								Serial.print(F("\t\t\tNo END or Char, instead, received: "));
								Serial.println(c, HEX);
								#endif
								size = 0;
								break;
							}
						} else {
							c = _spi_instance->transfer('\0');   // Dummy char to get the ACK
							size = 0;
							if (c < 128) {	// Makes sure it's an ASCII char
								_receiving_buffer[0] = c;
							} else {
								#ifdef BROADCAST_SPI_DEBUG_1
								Serial.println(F("\t\tNot a valid ASCII char (< 128)"));
								#endif
								break;
							}
						}
					}
				} else if (c == NONE) {
					#ifdef BROADCAST_SPI_DEBUG_2
					Serial.println(F("\t\tThere is nothing to be received"));
					#endif
					_receiving_buffer[0] = '\0';
					size = 1; // Nothing received
					break;
				} else {
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\t\tDevice NOT ready"));
					#endif
					size = 1; // Nothing received
					break;
				}

				if (size == 0) {
					delayMicroseconds(10);    // Makes sure the Status Byte is sent
					_spi_instance->transfer(ERROR);    // Results from ERROR or NACK send by the Slave and makes Slave reset to NONE
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\t\t\tSent ERROR"));
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
                Serial.print(F("\t\tMessage NOT successfully received on try: "));
                Serial.println(r + 1);
                Serial.println(F("\t\tBUZZER activated for 2 x 10ms!"));
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

        if (size > 0)
            size--;   // removes the '\0' from the length as final value
        return size;
    }


    bool acknowledgeReady(int ss_pin) {
        uint8_t c; // Avoid using 'char' while using values above 127
        bool acknowledge = false;

		#ifdef BROADCAST_SPI_DEBUG_1
		Serial.print(F("\tAcknowledging on pin: "));
		Serial.println(ss_pin);
		#endif

        for (size_t a = 0; !acknowledge && a < 3; a++) {
    
            digitalWrite(ss_pin, LOW);
            delayMicroseconds(5);

            // Asks the Slave to acknowledge readiness
            c = _spi_instance->transfer(ACK);

			if (c != VOID) {

				delayMicroseconds(10);
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

        size_t length = 0;

        // ON cycle

		// Copy safely (takes into consideration the '\0' char)
		strlcpy(_sending_buffer, command_on, BROADCAST_SOCKET_BUFFER_SIZE);
        length = sendString(length_on, _ss_pin);
        if (length == 0) return false;

        delay(1000);
        length = receiveString(_ss_pin);
        if (length == 0) return false;

        // JsonDocument in the stack makes sure its memory is released (NOT GLOBAL)
        #if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument message_doc;
        #else
        StaticJsonDocument<BROADCAST_SOCKET_BUFFER_SIZE> message_doc;
        #endif

        DeserializationError error = deserializeJson(message_doc, _receiving_buffer, BROADCAST_SOCKET_BUFFER_SIZE);
        if (error) {
			#ifdef BROADCAST_SPI_DEBUG_1
			Serial.print(F("ERROR (ON): "));
			Serial.print(error.c_str());
			Serial.print(F(" - Code: "));
			Serial.println(error.code());
			
			// What ArduinoJson sees
			Serial.print(F("Input length: "));
			Serial.println(strlen(_receiving_buffer));
			
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
		if(strcmp(command_name, "OK_ON") != 0) return false;

        length = receiveString(_ss_pin);
        if (length > 0) return false;

		_sending_buffer[0] = '\0';	// Clears buffer
        length = sendString(0, _ss_pin);    // Testing sending nothing at all
        if (length > 0) return false;
        delay(1000);
        length = receiveString(_ss_pin);   // Testing that receiving nothing also works
        if (length > 0) return false;
        length = receiveString(_ss_pin);   // Testing that receiving nothing also works
        if (length > 0) return false;


        // OFF cycle

		// Copy safely (takes into consideration the '\0' char)
		strlcpy(_sending_buffer, command_off, BROADCAST_SOCKET_BUFFER_SIZE);
        length = sendString(length_off, _ss_pin);
        if (length == 0) return false;
		
        delay(1000);
        length = receiveString(_ss_pin);
        if (length == 0) return false;

        error = deserializeJson(message_doc, _receiving_buffer, BROADCAST_SOCKET_BUFFER_SIZE);
        if (error) {
			#ifdef BROADCAST_SPI_DEBUG_1
			Serial.println(F("ERROR (OFF): Failed to deserialize JSON"));
			#endif
            return false;
        }
        json_message = message_doc.as<JsonObject>();
		command_name = json_message["n"].as<const char*>();
		if(strcmp(command_name, "OK_OFF") != 0) return false;

        length = receiveString(_ss_pin);
        if (length > 0) return false;

		_sending_buffer[0] = '\0';	// Clears buffer
        length = sendString(0, _ss_pin);
        if (length > 0) return false;
        delay(1000);
        length = receiveString(_ss_pin);
        if (length > 0) return false;
        length = receiveString(_ss_pin);
        if (length > 0) return false;

        return true;
    }


    bool ready() {
        return acknowledgeReady(_ss_pin);
    }

};


#endif // MASTER_CLASS_HPP
