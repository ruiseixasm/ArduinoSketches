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
#ifndef SLAVE_CLASS_HPP
#define SLAVE_CLASS_HPP

#include <Arduino.h>
#include <SPI.h>
#include <avr/interrupt.h>
#include <ArduinoJson.h>


#define BROADCAST_SPI_DEBUG_1
#define BROADCAST_SPI_DEBUG_2


// Pin definitions - define these in your main sketch
#ifndef GREEN_LED_PIN
#define GREEN_LED_PIN 2
#endif


// Why A7 cannot be HIGH

// On the ATmega328P Nano (old bootloader or new):
//     Pin	Digital I/O?	Analog Input?
//     A0–A5	✔ Yes	✔ Yes
//     A6–A7	❌ No (input-only)	✔ Yes

#ifndef YELLOW_LED_PIN
#define YELLOW_LED_PIN 19   // A5
#endif

#define BROADCAST_SOCKET_BUFFER_SIZE 128


class Slave_class
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
		LAST	= 0xFB,	// Asks for the last char
		CLEAR_R	= 0xFC,	// Asks for the receiving buffer clear on the Slave
		CLEAR_S	= 0xFD,	// Asks for the sending buffer clear on the Slave
        
        VOID    = 0xFF  // MISO floating (0xFF) → no slave responding
    };


protected:

    // volatile here makes sure the memory is read instead of cache values in registers
    char _receiving_buffer[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};
    char _sending_buffer[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};

    // Buffers and state variables

	// Pointers never change handleSPI_Interrupt method, otherwise declare them as 'volatile'
    static char* _ptr_receiving_buffer;
    static char* _ptr_sending_buffer;

    volatile static uint8_t _receiving_index;
	volatile static uint8_t _received_length;
    volatile static uint8_t _sending_index;
    volatile static uint8_t _validation_index;
	volatile static uint8_t _sending_length;
    volatile static MessageCode _transmission_mode;

	// JsonDocument in the stack makes sure its memory is released (NOT GLOBAL)
	#if ARDUINOJSON_VERSION_MAJOR >= 7
	JsonDocument message_doc;
	#else
	StaticJsonDocument<BROADCAST_SOCKET_BUFFER_SIZE> message_doc;
	#endif


	bool availableReceivingBuffer(uint8_t wait_seconds = 3) {
		unsigned long start_waiting = millis();
		while (_received_length) {
			if (millis() - start_waiting > 1000 * wait_seconds) {
				return false;
			}
		}
		return true;
	}

	bool availableSendingBuffer(uint8_t wait_seconds = 3) {
		unsigned long start_waiting = millis();
		while (_sending_length) {
			if (millis() - start_waiting > 1000 * wait_seconds) {
				return false;
			}
		}
		return true;
	}


    void processMessage() {

		#ifdef BROADCAST_SPI_DEBUG_1
        Serial.print(F("Processed command: "));
		Serial.write(_receiving_buffer, _received_length);
		Serial.println();
		#endif

        DeserializationError error = deserializeJson(message_doc, _receiving_buffer, _received_length);
        if (error) {
			#ifdef BROADCAST_SPI_DEBUG_1
			Serial.print(F("ERROR: Failed to deserialize JSON: "));
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
			
    		digitalWrite(YELLOW_LED_PIN, HIGH);
            return;
        }
        JsonObject json_message = message_doc.as<JsonObject>();

        const char* command_name = json_message["n"].as<const char*>();

		_received_length = 0;	// Receiving buffer can be released given that it was already processed

		if (!availableSendingBuffer()) {	// Makes sure the _sending_buffer is sent first

			#ifdef BROADCAST_SPI_DEBUG_1
			Serial.println(F("\tERROR: The _sending_buffer isn't available for sending"));
			#endif
			return;
		}

        if (strcmp(command_name, "ON") == 0) {
            digitalWrite(GREEN_LED_PIN, HIGH);
			digitalWrite(YELLOW_LED_PIN, LOW);
            json_message["n"] = "OK_ON";
            _sending_length = serializeJson(json_message, _sending_buffer, BROADCAST_SOCKET_BUFFER_SIZE);
			if (!_sending_length) {
				#ifdef BROADCAST_SPI_DEBUG_1
				Serial.println(F("ERROR: Failed to serialize JSON"));
				#endif
    			digitalWrite(YELLOW_LED_PIN, HIGH);
			}
            Serial.print(F("LED is ON"));
            Serial.print(F(" | Sending: "));
			Serial.write(_sending_buffer, _sending_length);
			Serial.println();
			
        } else if (strcmp(command_name, "OFF") == 0) {
            digitalWrite(GREEN_LED_PIN, LOW);
    		digitalWrite(YELLOW_LED_PIN, LOW);
            json_message["n"] = "OK_OFF";
            _sending_length = serializeJson(json_message, _sending_buffer, BROADCAST_SOCKET_BUFFER_SIZE);
			if (!_sending_length) {
				#ifdef BROADCAST_SPI_DEBUG_1
				Serial.println(F("ERROR: Failed to serialize JSON"));
				#endif
    			digitalWrite(YELLOW_LED_PIN, HIGH);
			}
            Serial.print(F("LED is OFF"));
            Serial.print(F(" | Sending: "));
			Serial.write(_sending_buffer, _sending_length);
			Serial.println();

        } else if (strcmp(command_name, "YELLOW_ON") == 0) {
            digitalWrite(GREEN_LED_PIN, LOW);
    		digitalWrite(YELLOW_LED_PIN, HIGH);
            json_message["n"] = "OK_OFF";
            _sending_length = serializeJson(json_message, _sending_buffer, BROADCAST_SOCKET_BUFFER_SIZE);
			if (!_sending_length) {
				#ifdef BROADCAST_SPI_DEBUG_1
				Serial.println(F("ERROR: Failed to serialize JSON"));
				#endif
    			digitalWrite(YELLOW_LED_PIN, HIGH);
			}
			_sending_length = 0;	// Removes blockage due to need for reply
            Serial.print(F("LED is OFF"));
            Serial.print(F(" | Sending: "));
			Serial.write(_sending_buffer, _sending_length);
			Serial.println();

        } else if (strcmp(command_name, "YELLOW_OFF") == 0) {
            digitalWrite(GREEN_LED_PIN, LOW);
    		digitalWrite(YELLOW_LED_PIN, LOW);
            json_message["n"] = "OK_OFF";
            _sending_length = serializeJson(json_message, _sending_buffer, BROADCAST_SOCKET_BUFFER_SIZE);
			if (!_sending_length) {
				#ifdef BROADCAST_SPI_DEBUG_1
				Serial.println(F("ERROR: Failed to serialize JSON"));
				#endif
    			digitalWrite(YELLOW_LED_PIN, HIGH);
			}
			_sending_length = 0;	// Removes blockage due to need for reply
            Serial.print(F("LED is OFF"));
            Serial.print(F(" | Sending: "));
			Serial.write(_sending_buffer, _sending_length);
			Serial.println();

        } else {
            json_message["n"] = "BUZZ";
            _sending_length = serializeJson(json_message, _sending_buffer, BROADCAST_SOCKET_BUFFER_SIZE);
			if (!_sending_length) {
				#ifdef BROADCAST_SPI_DEBUG_1
				Serial.println(F("ERROR: Failed to serialize JSON"));
				#endif
			}
            Serial.print(F("Unknown command"));
            Serial.print(F(" | Sending: "));
			Serial.write(_sending_buffer, _sending_length);
			Serial.println();
    		digitalWrite(YELLOW_LED_PIN, HIGH);
			
        }
    }


    void initSPISlave() {  // FIX 3: Add missing method definition
        pinMode(MISO, OUTPUT);  // MISO must be OUTPUT for Slave to send data!
        
        // Initialize SPI as slave - EXPLICIT MSB FIRST
        SPCR = 0;  // Clear register
        SPCR |= _BV(SPE);    // SPI Enable
        SPCR |= _BV(SPIE);   // SPI Interrupt Enable  
        SPCR &= ~_BV(DORD);  // MSB First (DORD=0 for MSB first)
        SPCR &= ~_BV(CPOL);  // Clock polarity 0
        SPCR &= ~_BV(CPHA);  // Clock phase 0 (MODE0)
    }


public:

    Slave_class() {

		// For static access to the buffers
		_ptr_receiving_buffer = _receiving_buffer;
		_ptr_sending_buffer = _sending_buffer;

        // Initialize pins
        pinMode(GREEN_LED_PIN, OUTPUT);
        digitalWrite(GREEN_LED_PIN, LOW);
        pinMode(YELLOW_LED_PIN, OUTPUT);
        digitalWrite(YELLOW_LED_PIN, LOW);

        // Setup SPI as slave
        initSPISlave();
    }

    ~Slave_class() {
		
		// Disable SPI interrupt
		SPCR &= ~(1 << SPIE);

        // This returns the pin to exact power-on state:
        pinMode(GREEN_LED_PIN, INPUT);
        digitalWrite(GREEN_LED_PIN, LOW);  // Important: disables any pull-up

        pinMode(YELLOW_LED_PIN, INPUT);
        digitalWrite(YELLOW_LED_PIN, LOW);
    }


	// // Slave_class Slave_class = Slave_class();  // WRONG!
	// Slave_class& Slave_class = Slave_class::instance();  // CORRECT!
	
    // Move ONLY the singleton instance method to subclass
    static Slave_class& instance() {

        static Slave_class instance;  	// ← No parentheses! Creates OBJECT
        return instance;				// ← Returns the object
    }


    // Actual interrupt handler
    static void handleSPI_Interrupt() {

        // WARNING 1:
        //     AVOID PLACING HEAVY CODE OR CALL HERE. THIS INTERRUPTS THE LOOP!

        // WARNING 2:
        //     AVOID PLACING Serial.print CALLS HERE BECAUSE IT WILL DELAY 
        //     THE POSSIBILITY OF SPI CAPTURE AND RESPONSE IN TIME !!!

        // WARNING 3:
        //     THE SETTING OF THE `SPDR` VARIABLE SHALL ALWAYS BE ON TOP, FIRSTLY THAN ALL OTHERS!

        uint8_t c = SPDR;    // Avoid using 'char' while using values above 127

        if (c < 128) {  // Only ASCII chars shall be transmitted as data

            // switch O(1) is more efficient than an if-else O(n) sequence because the compiler uses a jump table

            switch (_transmission_mode) {
                case RECEIVE:
                    if (_receiving_index < BROADCAST_SOCKET_BUFFER_SIZE) {
                        _ptr_receiving_buffer[_receiving_index] = c;
						if (_receiving_index > 0) {
							SPDR = _ptr_receiving_buffer[_receiving_index - 1];	// Char sent with an offset to guarantee matching
						}
						_receiving_index++;
                    } else {
                        SPDR = FULL;    // ALWAYS ON TOP
                        _transmission_mode = NONE;
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tERROR: Slave buffer overflow"));
						#endif
                    }
                    break;
                case SEND:
					if (_sending_index < _sending_length) {
						SPDR = _ptr_sending_buffer[_sending_index];		// This way avoids being the critical path (in advance)
					} else if (_sending_index == _sending_length) {
						SPDR = LAST;	// Asks for the LAST char
					} else {	// Less missed sends this way
						SPDR = END;		// All chars have been checked
					}
					// Starts checking 2 indexes after
					if (_sending_index > 1) {    // Two positions of delay
						if (c == _ptr_sending_buffer[_validation_index]) {	// Checks all chars
							_validation_index++; // Starts checking after two sent
						} else {
							SPDR = ERROR;
							_transmission_mode = NONE;  // Makes sure no more communication is done, regardless
							#ifdef BROADCAST_SPI_DEBUG_1
							Serial.println(F("\t\tERROR: Sent char mismatch"));
							#endif
							break;
						}
					}
					_sending_index++;
                    break;
                default:
                    SPDR = NACK;
            }

        } else {    // It's a control message 0xFX
            
            // switch O(1) is more efficient than an if-else O(n) sequence because the compiler uses a jump table

            switch (c) {
                case RECEIVE:
                    if (_ptr_receiving_buffer) {
						if (!_received_length) {
							SPDR = READY;
							_transmission_mode = RECEIVE;
							_receiving_index = 0;
						} else {
                        	SPDR = BUSY;
							#ifdef BROADCAST_SPI_DEBUG_1
							Serial.println(F("\t\tBUSY: I'm busy (RECEIVE)"));
							#endif
						}
                    } else {
                        SPDR = VOID;
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tERROR: Receiving buffer pointer NOT set"));
						#endif
                    }
                    break;
                case SEND:
                    if (_ptr_sending_buffer) {
                        if (_sending_length) {
							if (_sending_length > BROADCAST_SOCKET_BUFFER_SIZE) {
								SPDR = FULL;
								_sending_length = 0;
							} else {
								SPDR = READY;
								_transmission_mode = SEND;
								_sending_index = 0;
								_validation_index = 0;
							}
                        } else {
                            SPDR = NONE;
							#ifdef BROADCAST_SPI_DEBUG_2
							Serial.println(F("\tNothing to be sent"));
							#endif
                        }
                    } else {
                        SPDR = VOID;
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tERROR: Sending buffer pointer NOT set"));
						#endif
                    }
                    break;
                case LAST:
					if (_transmission_mode == RECEIVE) {
						SPDR = _ptr_receiving_buffer[_receiving_index - 1];
                    } else if (_transmission_mode == SEND && _sending_length > 0) {
						SPDR = _ptr_sending_buffer[_sending_length - 1];
                    } else {
						SPDR = NONE;
					}
                    break;
                case END:
					if (_transmission_mode == RECEIVE) {
						SPDR = ACK;
						_received_length = _receiving_index;
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\tReceived message"));
						#endif
                    } else if (_transmission_mode == SEND) {
                        _sending_length = 0;	// Makes sure the sending buffer is zeroed
						SPDR = _sending_length;
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\tSent message"));
						#endif
                    }
                    _transmission_mode = NONE;
                    break;
                case ACK:
                    SPDR = ACK;
                    break;
                case ERROR:
                case FULL:
                    SPDR = ACK;
                    _transmission_mode = NONE;
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\tTransmission ended with received ERROR or FULL"));
					#endif
                    break;
				case CLEAR_R:
					_received_length = 0;	// Clears receiving buffer
					SPDR = _received_length;
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\tCleared receiving buffer"));
					#endif
                    break;
				case CLEAR_S:
					_sending_length = 0;	// Clears sending buffer
					SPDR = _sending_length;
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\tCleared sending buffer"));
					#endif
                    break;
                default:
                    SPDR = NACK;
            }
        }
    }

	
    void process() {
        if (_received_length) {
            processMessage();   // Called only once!
			_received_length = 0;   // Makes sure the receiving buffer is zeroed
        }
    }

};


// Initialize static members
char* Slave_class::_ptr_receiving_buffer = nullptr;
char* Slave_class::_ptr_sending_buffer = nullptr;

volatile uint8_t Slave_class::_receiving_index = 0;
volatile uint8_t Slave_class::_received_length = 0;
volatile uint8_t Slave_class::_sending_index = 0;
volatile uint8_t Slave_class::_validation_index = 0;
volatile uint8_t Slave_class::_sending_length = 0;
volatile Slave_class::MessageCode Slave_class::_transmission_mode = Slave_class::MessageCode::NONE;


#endif // SLAVE_CLASS_HPP
