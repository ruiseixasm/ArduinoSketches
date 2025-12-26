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
#ifndef BROADCAST_SOCKET_SPI_ESP_ARDUINO_SLAVE_HPP
#define BROADCAST_SOCKET_SPI_ESP_ARDUINO_SLAVE_HPP


#include <SPI.h>
#include <ArduinoJson.h>    // Include ArduinoJson Library to be used as a dictionary
#include "../BroadcastSocket.hpp"


// #define BROADCAST_SPI_DEBUG
// #define BROADCAST_SPI_DEBUG_1
// #define BROADCAST_SPI_DEBUG_2


class SPI_ESP_Arduino_Slave : public BroadcastSocket {
public:

    const char* class_name() const override { return "SPI_ESP_Arduino_Slave"; }

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

    static char* _ptr_received_buffer;
    static char* _ptr_sending_buffer;

    volatile static uint8_t _receiving_index;
	volatile static uint8_t _received_length_spi;
    volatile static uint8_t _sending_index;
    volatile static uint8_t _validation_index;
	volatile static uint8_t _sending_length_spi;
    volatile static StatusByte _transmission_mode;


    // Needed for the compiler, the base class is the one being called though
    // ADD THIS CONSTRUCTOR - it calls the base class constructor
    SPI_ESP_Arduino_Slave(JsonTalker** const json_talkers, uint8_t talker_count, BroadcastValue source_value = BroadcastValue::REMOTE)
        : BroadcastSocket(json_talkers, talker_count, source_value) {
            
			// Initialize SPI
			SPI.begin();
			SPI.setClockDivider(SPI_CLOCK_DIV4);    // Only affects the char transmission
			SPI.setDataMode(SPI_MODE0);
			SPI.setBitOrder(MSBFIRST);  // EXPLICITLY SET MSB FIRST! (OTHERWISE is LSB)

			pinMode(MISO, OUTPUT);  // MISO must be OUTPUT for Slave to send data!
			
			// Initialize SPI as slave - EXPLICIT MSB FIRST
			SPCR = 0;  // Clear register
			SPCR |= _BV(SPE);    // SPI Enable
			SPCR |= _BV(SPIE);   // SPI Interrupt Enable  
			SPCR &= ~_BV(DORD);  // MSB First (DORD=0 for MSB first)
			SPCR &= ~_BV(CPOL);  // Clock polarity 0
			SPCR &= ~_BV(CPHA);  // Clock phase 0 (MODE0)

            // For static access to the buffers
            _ptr_received_buffer = _received_buffer;
            _ptr_sending_buffer = _sending_buffer;

            _max_delay_ms = 0;  // SPI is sequencial, no need to control out of order packages
            // // Initialize devices control object (optional initial setup)
            // devices_ss_pins["initialized"] = true;
        }


	bool availableReceivingBuffer(uint8_t wait_seconds = 3) override {
		uint16_t start_waiting = (uint16_t)millis();
		while (_received_length_spi) {
			if ((uint16_t)millis() - start_waiting > 1000 * wait_seconds) {
				return false;
			}
		}
		return true;
	}

	bool availableSendingBuffer(uint8_t wait_seconds = 3) override {
		uint16_t start_waiting = (uint16_t)millis();
		while (_sending_length_spi) {
			if ((uint16_t)millis() - start_waiting > 1000 * wait_seconds) {
				
				#ifdef BROADCASTSOCKET_DEBUG
				Serial.println(F("\tavailableSendingBuffer: NOT available sending buffer"));
				#endif

				return false;
			}
		}
		
		#ifdef BROADCASTSOCKET_DEBUG
		Serial.println(F("\tavailableSendingBuffer: Available sending buffer"));
		#endif

		return true;
	}


    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    bool send(const JsonMessage& json_message) override {

		if (BroadcastSocket::send(json_message)) {	// Very important pre processing !!
            
			#ifdef BROADCAST_SPI_DEBUG
			Serial.print(F("\tsend1: Sent message: "));
			Serial.write(_sending_buffer, _sending_length);
			Serial.println();
			Serial.print(F("\tsend2: Sent length: "));
			Serial.println(_sending_length);
			#endif

			// Marks the sending buffer ready to be sent
            _sending_length_spi = _sending_length;
			
		}
        return false;
    }

    uint8_t receive() override {

        // Need to call homologous method in super class first
        uint8_t length = BroadcastSocket::receive(); // Very important to do or else it may stop receiving !!
		length = _received_length_spi;

		if (length) {
			
			#ifdef BROADCAST_SPI_DEBUG
			Serial.print(F("\treceive1: Received message: "));
			Serial.write(_received_buffer, length);
			Serial.println();
			Serial.print(F("\treceive1: Received length: "));
			Serial.println(length);
			#endif
			
			_received_length = length;
			BroadcastSocket::triggerTalkers();
			_received_length_spi = 0;	// Allows the device to receive more data
			_received_length = 0;
		}

        return length;
    }


public:

    // Move ONLY the singleton instance method to subclass
    static SPI_ESP_Arduino_Slave& instance(JsonTalker** const json_talkers, uint8_t talker_count, BroadcastValue source_value = BroadcastValue::REMOTE) {

        static SPI_ESP_Arduino_Slave instance(json_talkers, talker_count, source_value);
        return instance;
    }

	// Specific methods associated to Arduino SPI as Slave

    // Actual interrupt handler
    static void handleSPI_Interrupt() {

        // WARNING 1:
        //     AVOID PLACING HEAVY CODE OR CALL SELF. THIS INTERRUPTS THE LOOP!

        // WARNING 2:
        //     AVOID PLACING Serial.print CALLS SELF BECAUSE IT WILL DELAY 
        //     THE POSSIBILITY OF SPI CAPTURE AND RESPONSE IN TIME !!!

        // WARNING 3:
        //     THE SETTING OF THE `SPDR` VARIABLE SHALL ALWAYS BE DONE AFTER ALL OTHER SETTINGS,
		//     TO MAKE SURE THEY ARE REALLY SET WHEN THE `SPDR` REPORTS A SET CONDITION!

		// WARNING 4:
		//     FOR FINALLY USAGE MAKE SURE TO COMMENT OUT THE BROADCAST_SPI_DEBUG_1 AND BROADCAST_SPI_DEBUG_1
		// 	   DEFINITIONS OR ELSE THE SLAVE WONT RESPOND IN TIME AND ERRORS WILL RESULT DUE TO IT!

        uint8_t c = SPDR;    // Avoid using 'char' while using values above 127

        if (c < 128) {  // Only ASCII chars shall be transmitted as data

            // switch O(1) is more efficient than an if-else O(n) sequence because the compiler uses a jump table

            switch (_transmission_mode) {
                case RECEIVE:
                    if (_receiving_index < BROADCAST_SOCKET_BUFFER_SIZE) {
                        _ptr_received_buffer[_receiving_index] = c;
						if (_receiving_index > 0) {
							SPDR = _ptr_received_buffer[_receiving_index - 1];	// Char sent with an offset to guarantee matching
						}
						_receiving_index++;
                    } else {
						_transmission_mode = NONE;
                        SPDR = FULL;
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tERROR: Slave buffer overflow"));
						#endif
                    }
                    break;
                case SEND:
					if (_sending_index < _sending_length_spi) {
						SPDR = _ptr_sending_buffer[_sending_index];		// This way avoids being the critical path (in advance)
					} else if (_sending_index == _sending_length_spi) {
						SPDR = LAST;	// Asks for the LAST char
					} else {	// Less missed sends this way
						SPDR = END;		// All chars have been checked
					}
					// Starts checking 2 indexes after
					if (_sending_index > 1) {    // Two positions of delay
						if (c == _ptr_sending_buffer[_validation_index]) {	// Checks all chars
							_validation_index++; // Starts checking after two sent
						} else {
							_transmission_mode = NONE;  // Makes sure no more communication is done, regardless
							SPDR = ERROR;
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
                    if (_ptr_received_buffer) {
						if (!_received_length_spi) {
							_transmission_mode = RECEIVE;
							_receiving_index = 0;
							SPDR = READY;	// Doing it at the end makes sure everything above was actually set
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
                        if (_sending_length_spi) {
							if (_sending_length_spi > BROADCAST_SOCKET_BUFFER_SIZE) {
								_sending_length_spi = 0;
								SPDR = FULL;
							} else {
								_transmission_mode = SEND;
								_sending_index = 0;
								_validation_index = 0;
								SPDR = READY;	// Doing it at the end makes sure everything above was actually set
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
						SPDR = _ptr_received_buffer[_receiving_index - 1];
                    } else if (_transmission_mode == SEND && _sending_length_spi > 0) {
						SPDR = _ptr_sending_buffer[_sending_length_spi - 1];
                    } else {
						SPDR = NONE;
					}
                    break;
                case END:
					if (_transmission_mode == RECEIVE) {
						_received_length_spi = _receiving_index;
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\tReceived message"));
						#endif
                    } else if (_transmission_mode == SEND) {
                        _sending_length_spi = 0;	// Makes sure the sending buffer is zeroed
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\tSent message"));
						#endif
					}
                    _transmission_mode = NONE;
					SPDR = DONE;	// Doing it at the end makes sure everything above was actually set
                    break;
                case ACK:
                    SPDR = ACK;
                    break;
                case ERROR:
                case FULL:
					_transmission_mode = NONE;
					SPDR = ACK;
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\tTransmission ended with received ERROR or FULL"));
					#endif
                    break;
                default:
                    SPDR = NACK;
            }
        }
    }

};


#endif // BROADCAST_SOCKET_SPI_ESP_ARDUINO_SLAVE_HPP
