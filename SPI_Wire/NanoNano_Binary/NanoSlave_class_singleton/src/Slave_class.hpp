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


#define BROADCAST_SPI_DEBUG


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
        
        VOID    = 0xFF  // MISO floating (0xFF) → no slave responding
    };

private:

    char _receiving_buffer[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};
    char _sending_buffer[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};

    // Buffers and state variables
	// Exclusive to the handleSPI_Interrupt method, otherwise declare them as 'volatile'
    static char* _ptr_receiving_buffer;
    static char* _ptr_sending_buffer;

    volatile static uint8_t _receiving_index;
    volatile static uint8_t _sending_index;
    volatile static uint8_t _validation_index;
    volatile static uint8_t _send_iteration_i;
    volatile static MessageCode _transmission_mode;
	volatile static bool _received_data;
	volatile static bool _ready_to_send;


    void processMessage() {

        Serial.print("Processed command: ");
        Serial.println(_receiving_buffer);

        // JsonDocument in the stack makes sure its memory is released (NOT GLOBAL)
        #if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument message_doc;
        #else
        StaticJsonDocument<BROADCAST_SOCKET_BUFFER_SIZE> message_doc;
        #endif

        DeserializationError error = deserializeJson(message_doc, _receiving_buffer, BROADCAST_SOCKET_BUFFER_SIZE);
        if (error) {
			#ifdef BROADCAST_SPI_DEBUG
			Serial.println("ERROR: Failed to deserialize JSON");
			#endif
            return;
        }
        JsonObject json_message = message_doc.as<JsonObject>();

        const char* command_name = json_message["n"].as<const char*>();


        if (strcmp(command_name, "ON") == 0) {
            digitalWrite(GREEN_LED_PIN, HIGH);
            json_message["n"] = "OK_ON";
            size_t length = serializeJson(json_message, _sending_buffer, BROADCAST_SOCKET_BUFFER_SIZE);
			#ifdef BROADCAST_SPI_DEBUG
			if (length == 0)
				Serial.println("ERROR: Failed to serialize JSON");
			#endif
            Serial.print("LED is ON");
            Serial.print(" | Sending: ");
            Serial.println(_sending_buffer);

        } else if (strcmp(command_name, "OFF") == 0) {
            digitalWrite(GREEN_LED_PIN, LOW);
            json_message["n"] = "OK_OFF";
            size_t length = serializeJson(json_message, _sending_buffer, BROADCAST_SOCKET_BUFFER_SIZE);
			#ifdef BROADCAST_SPI_DEBUG
			if (length == 0)
				Serial.println("ERROR: Failed to serialize JSON");
			#endif
            Serial.print("LED is OFF");
            Serial.print(" | Sending: ");
            Serial.println(_sending_buffer);
        } else {
            json_message["n"] = "BUZZ";
            size_t length = serializeJson(json_message, _sending_buffer, BROADCAST_SOCKET_BUFFER_SIZE);
			#ifdef BROADCAST_SPI_DEBUG
			if (length == 0)
				Serial.println("ERROR: Failed to serialize JSON");
			#endif
            Serial.print("Unknown command");
            Serial.print(" | Sending: ");
            Serial.println(_sending_buffer);
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
                        // Returns same received char as receiving confirmation (no need to set SPDR)
                        _ptr_receiving_buffer[_receiving_index++] = c;
                    } else {
                        SPDR = FULL;    // ALWAYS ON TOP
                        _transmission_mode = NONE;
                    }
                    break;
                case SEND:
					if (_sending_index < BROADCAST_SOCKET_BUFFER_SIZE) {
						SPDR = _ptr_sending_buffer[_sending_index];		// This way avoids being the critical path (in advance)
						if (_validation_index > _sending_index) {	// Less missed sends this way
							SPDR = END;	// All chars have been checked
							break;
						}
					} else {
						SPDR = FULL;
						_transmission_mode = NONE;
						break;
					}
					// Starts checking 2 indexes after
					if (_send_iteration_i > 1) {    // Two positions of delay
						if (c != _ptr_sending_buffer[_validation_index]) {   // Also checks '\0' char
							SPDR = ERROR;
							_transmission_mode = NONE;  // Makes sure no more communication is done, regardless
							break;
						}
						_validation_index++; // Starts checking after two sent
					}
					// Only increments if NOT at the end of the string being sent
					if (_ptr_sending_buffer[_sending_index] != '\0') {
						_sending_index++;
					}
                    _send_iteration_i++;
                    break;
                default:
                    SPDR = NACK;
            }

        } else {    // It's a control message 0xFX
            
            // switch O(1) is more efficient than an if-else O(n) sequence because the compiler uses a jump table

            switch (c) {
                case RECEIVE:
                    if (_ptr_receiving_buffer) {
						if (_transmission_mode == NONE && !_received_data) {
							SPDR = READY;
							_transmission_mode = RECEIVE;
							_receiving_index = 0;
						} else {
                        	SPDR = BUSY;
							#ifdef BROADCAST_SPI_DEBUG
							Serial.println("\tI'm busy (RECEIVE)");
							#endif
						}
                    } else {
                        SPDR = VOID;
                    }
                    break;
                case SEND:
                    if (_ptr_sending_buffer) {
                        if (_ready_to_send) {
							if (_transmission_mode == NONE) {
								SPDR = READY;
								_transmission_mode = SEND;
								_sending_index = 0;
								_validation_index = 0;
								_send_iteration_i = 0;
							} else {
								SPDR = BUSY;
								#ifdef BROADCAST_SPI_DEBUG
								Serial.println("\tI'm busy (SEND)");
								#endif
							}
                        } else {
                            SPDR = NONE;
							#ifdef BROADCAST_SPI_DEBUG
							Serial.println("\tNothing to be sent");
							#endif
                        }
                    } else {
                        SPDR = VOID;
                    }
                    break;
                case END:
                    SPDR = ACK;
					if (_transmission_mode == RECEIVE) {
						_received_data = true;
						#ifdef BROADCAST_SPI_DEBUG
						Serial.println("\tReceived message");
						#endif
                    } else if (_transmission_mode == SEND) {
                        _ready_to_send = false;	// Makes sure the sending buffer is tagged as sent
						#ifdef BROADCAST_SPI_DEBUG
						Serial.println("\tSent message");
						#endif
                    }
                    _transmission_mode = NONE;
                    break;
                case ACK:
					if (_transmission_mode == NONE) {
                    	SPDR = READY;
					} else {
                        SPDR = BUSY;
						#ifdef BROADCAST_SPI_DEBUG
						Serial.println("\tI'm busy (ACK)");
						#endif
					}
                    break;
                case ERROR:
                case FULL:
                    SPDR = ACK;
                    _transmission_mode = NONE;
					#ifdef BROADCAST_SPI_DEBUG
					Serial.println("\tTransmission ended");
					#endif
                    break;
                default:
                    SPDR = NACK;
            }
        }
    }

	
    void process() {
        if (_received_data) {
            processMessage();   // Called only once!
			_ready_to_send = true;
			_received_data = false;
        }
    }

};


// Initialize static members
char* Slave_class::_ptr_receiving_buffer = nullptr;
char* Slave_class::_ptr_sending_buffer = nullptr;

volatile uint8_t Slave_class::_receiving_index = 0;
volatile uint8_t Slave_class::_sending_index = 0;
volatile uint8_t Slave_class::_validation_index = 0;
volatile uint8_t Slave_class::_send_iteration_i = 0;
volatile Slave_class::MessageCode Slave_class::_transmission_mode = Slave_class::MessageCode::NONE;
volatile bool Slave_class::_received_data = false;
volatile bool Slave_class::_ready_to_send = false;


#endif // SLAVE_CLASS_HPP
