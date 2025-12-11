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


#define BROADCAST_SPI_DEBUG


class BroadcastSocket_SPI_ESP_Arduino_Slave : public BroadcastSocket {
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

    static char _isr_receiving_buffer[BROADCAST_SOCKET_BUFFER_SIZE];
    static char _isr_sending_buffer[BROADCAST_SOCKET_BUFFER_SIZE];
	
    volatile static uint8_t _receiving_index;
    volatile static uint8_t _sending_index;
    volatile static uint8_t _validation_index;
    volatile static uint8_t _send_iteration_i;
    volatile static MessageCode _transmission_mode;
	volatile static bool _received_data;

protected:
    // Needed for the compiler, the base class is the one being called though
    // ADD THIS CONSTRUCTOR - it calls the base class constructor
    BroadcastSocket_SPI_ESP_Arduino_Slave(JsonTalker** json_talkers, uint8_t talker_count)
        : BroadcastSocket(json_talkers, talker_count) {
            
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

            _max_delay_ms = 0;  // SPI is sequencial, no need to control out of order packages
            // // Initialize devices control object (optional initial setup)
            // devices_ss_pins["initialized"] = true;
        }


    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    size_t send(size_t length, bool as_reply = false, uint8_t target_index = 255) override {
        (void)as_reply; 	// Silence unused parameter warning
        (void)target_index; // Silence unused parameter warning

		// Need to call homologous method in super class first
		length = BroadcastSocket::send(length, as_reply); // Very important pre processing !!

		if (length > 0) {
			memcpy(_isr_sending_buffer, _sending_buffer, length);
		}

        return length;
    }


public:

	// Specific methods associated to Arduino SPI as Slave

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
                        _isr_receiving_buffer[_receiving_index++] = c;
                    } else {
                        SPDR = FULL;    // ALWAYS ON TOP
                        _transmission_mode = NONE;
                    }
                    break;
                case SEND:
					if (_sending_index < BROADCAST_SOCKET_BUFFER_SIZE) {
						SPDR = _isr_sending_buffer[_sending_index];		// This way avoids being the critical path (in advance)
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
						if (c != _isr_sending_buffer[_validation_index]) {   // Also checks '\0' char
							SPDR = ERROR;
							_transmission_mode = NONE;  // Makes sure no more communication is done, regardless
							break;
						}
						_validation_index++; // Starts checking after two sent
					}
					// Only increments if NOT at the end of the string being sent
					if (_isr_sending_buffer[_sending_index] != '\0') {
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
                    SPDR = ACK;
                    _transmission_mode = RECEIVE;
                    _receiving_index = 0;
                    break;
                case SEND:
                    if (_isr_sending_buffer[0] == '\0') {
                        SPDR = NONE;
                    } else {
                        SPDR = ACK;
                        _transmission_mode = SEND;
                        _sending_index = 0;
                        _validation_index = 0;
                        _send_iteration_i = 0;
                    }
                    break;
                case END:
                    SPDR = ACK;
					if (_transmission_mode == RECEIVE) {
						_received_data = true;
                    } else if (_transmission_mode == SEND) {
                        _isr_sending_buffer[0] = '\0';	// Makes sure the sending buffer is marked as empty (NONE next time)
						_sending_index = 0;
                    }
                    _transmission_mode = NONE;
                    break;
                case ACK:
                    SPDR = READY;
                    break;
                case ERROR:
                case FULL:
                    SPDR = ACK;
                    if (_transmission_mode == RECEIVE) {
                        _isr_receiving_buffer[0] = '\0';	// Makes sure the receiving buffer is marked as empty in case of error
						_receiving_index = 0;
                    }
                    _transmission_mode = NONE;
                    break;
                default:
                    SPDR = NACK;
            }
        }
    }


    // Move ONLY the singleton instance method to subclass
    static BroadcastSocket_SPI_ESP_Arduino_Slave& instance(JsonTalker** json_talkers, uint8_t talker_count) {

        static BroadcastSocket_SPI_ESP_Arduino_Slave instance(json_talkers, talker_count);
        return instance;
    }


    size_t receive() override {

        // Need to call homologous method in super class first
        size_t length = BroadcastSocket::receive(); // Very important to do or else it may stop receiving !!

		if (_received_data) {
			
			#ifdef BROADCAST_SPI_DEBUG
			Serial.print("Received message: ");
			Serial.println(_isr_receiving_buffer);
			#endif

			length = _receiving_index + 1;	// Makes sure everything is included, doesn't exclude the '\0' char
			memcpy(_receiving_buffer, _isr_receiving_buffer, length);
			length = BroadcastSocket::triggerTalkers(length);
			_received_data = false;
		}

        return length;
    }

};


#endif // BROADCAST_SOCKET_SPI_ESP_ARDUINO_SLAVE_HPP
