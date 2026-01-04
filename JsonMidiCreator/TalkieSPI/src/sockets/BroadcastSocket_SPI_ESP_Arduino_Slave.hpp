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
#include "../BroadcastSocket.h"

#define BROADCAST_SPI_DEBUG


#define ENABLE_DIRECT_ADDRESSING



class BroadcastSocket_SPI_ESP_Arduino_Slave : public BroadcastSocket {
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


private:
    JsonObject* _talkers_ss_pins;
	
    volatile static uint8_t _receiving_index;
    volatile static uint8_t _sending_index;
    volatile static uint8_t _validation_index;
    volatile static uint8_t _send_iteration_i;
    volatile static StatusByte _transmission_mode;
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

            _max_delay_ms = 0;  // SPI is sequencial, no need to control out of order packages
            // // Initialize devices control object (optional initial setup)
            // devices_ss_pins["initialized"] = true;
        }


	//
	// No need for send override, the handleSPI_Interrupt handles the sending
	//


	void deleteReceived() {
		_receiving_buffer[0] = '\0';
		_received_data = false;
	}
	
    
public:

	// Specific methods associated to Arduino SPI as Slave

    // Actual interrupt handler
    static void handleSPI_Interrupt() {

        // WARNING 1:
        //     AVOID PLACING HEAVY CODE OR CALL SELF. THIS INTERRUPTS THE LOOP!

        // WARNING 2:
        //     AVOID PLACING Serial.print CALLS SELF BECAUSE IT WILL DELAY 
        //     THE POSSIBILITY OF SPI CAPTURE AND RESPONSE IN TIME !!!

        // WARNING 3:
        //     THE SETTING OF THE `SPDR` VARIABLE SHALL ALWAYS BE ON TOP, FIRSTLY THAN ALL OTHERS!

        uint8_t c = SPDR;    // Avoid using 'char' while using values above 127

        if (c < 128) {  // Only ASCII chars shall be transmitted as data

            // switch O(1) is more efficient than an if-else O(n) sequence because the compiler uses a jump table

            switch (_transmission_mode) {
                case RECEIVE:
                    if (_receiving_index < TALKIE_BUFFER_SIZE) {
                        // Returns same received char as receiving confirmation (no need to set SPDR)
                        _receiving_buffer[_receiving_index++] = c;
                    } else {
                        SPDR = FULL;    // ALWAYS ON TOP
                        _transmission_mode = NONE;
                    }
                    break;
                case SEND:
					if (_sending_index < TALKIE_BUFFER_SIZE) {
						SPDR = _sending_buffer[_sending_index];		// This way avoids being the critical path (in advance)
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
						if (c != _sending_buffer[_validation_index]) {   // Also checks '\0' char
							SPDR = ERROR;
							_transmission_mode = NONE;  // Makes sure no more communication is done, regardless
							break;
						}
						_validation_index++; // Starts checking after two sent
					}
					// Only increments if NOT at the end of the string being sent
					if (_sending_buffer[_sending_index] != '\0') {
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
                    SPDR = ACK;
                    _transmission_mode = SEND;
                    _sending_index = 0;
                    _validation_index = 0;
                    _send_iteration_i = 0;
                    break;
                case END:
                    SPDR = ACK;
					if (_transmission_mode == RECEIVE) {
						_received_data = true;
                    } else if (_transmission_mode == SEND) {
                        _sending_buffer[0] = '\0';	// Makes sure the sending buffer is marked as empty (NONE next time)
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
                        _receiving_buffer[0] = '\0';	// Makes sure the receiving buffer is marked as empty in case of error
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
			length = BroadcastSocket::startTransmission(_receiving_index);
			deleteReceived();
		}

        return length;
    }

};


volatile uint8_t BroadcastSocket_SPI_ESP_Arduino_Slave::_receiving_index = 0;
volatile uint8_t BroadcastSocket_SPI_ESP_Arduino_Slave::_sending_index = 0;
volatile uint8_t BroadcastSocket_SPI_ESP_Arduino_Slave::_validation_index = 0;
volatile uint8_t BroadcastSocket_SPI_ESP_Arduino_Slave::_send_iteration_i = 0;
volatile BroadcastSocket_SPI_ESP_Arduino_Slave::StatusByte BroadcastSocket_SPI_ESP_Arduino_Slave::_transmission_mode 
																	= BroadcastSocket_SPI_ESP_Arduino_Slave::StatusByte::NONE;
volatile bool BroadcastSocket_SPI_ESP_Arduino_Slave::_received_data = false;

// Define ISR at GLOBAL SCOPE (outside the class)
ISR(SPI_STC_vect) {
    // You need a way to call your class method from here
    // Possibly using a static method or singleton pattern
    BroadcastSocket_SPI_ESP_Arduino_Slave::handleSPI_Interrupt();
}

#endif // BROADCAST_SOCKET_SPI_ESP_ARDUINO_SLAVE_HPP
