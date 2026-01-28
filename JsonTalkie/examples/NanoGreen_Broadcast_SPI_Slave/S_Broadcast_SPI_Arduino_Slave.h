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
#ifndef BROADCAST_SPI_ARDUINO_SLAVE_HPP
#define BROADCAST_SPI_ARDUINO_SLAVE_HPP


#include <BroadcastSocket.h>
#include <SPI.h>


// #define BROADCAST_SPI_ARDUINO_SLAVE_DEBUG_RECEIVE
// #define BROADCAST_SPI_ARDUINO_SLAVE_DEBUG_SEND
// #define BROADCAST_SPI_ARDUINO_SLAVE_DEBUG_1
// #define BROADCAST_SPI_ARDUINO_SLAVE_DEBUG_2


class S_Broadcast_SPI_Arduino_Slave : public BroadcastSocket {
public:

	// The Socket class description shouldn't be greater than 35 chars
	// {"m":7,"f":"","s":3,"b":1,"t":"","i":58485,"0":1,"1":"","2":11,"c":11266} <-- 128 - (73 + 2*10) = 35
    const char* class_description() const override { return "Broadcast_SPI_Arduino_Slave"; }

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


protected:

	static char _received_buffer[TALKIE_BUFFER_SIZE];
	static char _sending_buffer[TALKIE_BUFFER_SIZE];

    volatile static size_t _receiving_index;
	volatile static size_t _received_length;
    volatile static size_t _sending_index;
	volatile static size_t _sending_length;
    volatile static StatusByte _transmission_mode;


    // Needed for the compiler, the base class is the one being called though
    // ADD THIS CONSTRUCTOR - it calls the base class constructor
    S_Broadcast_SPI_Arduino_Slave() : BroadcastSocket() {
            
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

	
    void _receive() override {

		if (_received_length) {

			JsonMessage new_message;
			if (new_message.deserialize_buffer(_received_buffer, _received_length)) {
				
				#ifdef BROADCAST_SPI_ARDUINO_SLAVE_DEBUG_RECEIVE
				Serial.print(F("\treceive1: Received message: "));
				Serial.write(_received_buffer, _received_length);
				Serial.println();
				Serial.print(F("\treceive1: Received length: "));
				Serial.println(_received_length);
				#endif
				
				_received_length = 0;	// Allows the device to receive more data
				_startTransmission(new_message);
				
			} else {
				_received_length = 0;	// Discards the data regardless
			}
		}
    }


    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    bool _send(const JsonMessage& json_message) override {

		#ifdef BROADCAST_SPI_ARDUINO_SLAVE_DEBUG_SEND
		Serial.print(F("\tsend1: Sent message: "));
		json_message.write_to(Serial);
		Serial.print(" | ");
		Serial.println(json_message.get_length());
		#endif

		uint16_t start_waiting = (uint16_t)millis();
		while (_sending_length) {
			if ((uint16_t)millis() - start_waiting > 1 * 1000) {

				#ifdef BROADCASTSOCKET_DEBUG
				Serial.println(F("\t_unlockSendingBuffer: NOT available sending buffer"));
				#endif

				return false;
			}
		}
		_sending_length = json_message.serialize_json(_sending_buffer, TALKIE_BUFFER_SIZE);
			
        return true;
    }


public:

    // Move ONLY the singleton instance method to subclass
    static S_Broadcast_SPI_Arduino_Slave& instance() {

        static S_Broadcast_SPI_Arduino_Slave instance;
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
		//     FOR FINALLY USAGE MAKE SURE TO COMMENT OUT THE BROADCAST_SPI_ARDUINO_SLAVE_DEBUG_1 AND BROADCAST_SPI_ARDUINO_SLAVE_DEBUG_1
		// 	   DEFINITIONS OR ELSE THE SLAVE WONT RESPOND IN TIME AND ERRORS WILL RESULT DUE TO IT!

        uint8_t c = SPDR;    // Avoid using 'char' while using values above 127

        if (c < 128 && _transmission_mode == TALKIE_SB_RECEIVE) {  // Only ASCII chars shall be transmitted as data

			if (_receiving_index < TALKIE_BUFFER_SIZE) {
				_received_buffer[_receiving_index++] = c;
			} else {
				_transmission_mode = TALKIE_SB_NONE;
				#ifdef BROADCAST_SPI_ARDUINO_SLAVE_DEBUG_1
				Serial.println(F("\t\tERROR: Slave buffer overflow"));
				#endif
			}

        } else {    // It's a control message 0xFX
            
            // switch O(1) is more efficient than an if-else O(n) sequence because the compiler uses a jump table

            switch (c) {
                case TALKIE_SB_RECEIVE:
					if (_received_length == 0) {
						_transmission_mode = TALKIE_SB_RECEIVE;
						_receiving_index = 0;
					} else {
						#ifdef BROADCAST_SPI_ARDUINO_SLAVE_DEBUG_1
						Serial.println(F("\t\tBUSY: I'm busy (TALKIE_SB_RECEIVE)"));
						#endif
					}
                    break;
                case TALKIE_SB_SEND:
					if (_sending_index < TALKIE_BUFFER_SIZE) {	// Safe code
						if (_sending_index < _sending_length) {
							SPDR = _sending_buffer[_sending_index++];
						} else {
							SPDR = TALKIE_SB_END;
						}
					} else {
						_sending_length = 0;	// Makes sure the sending buffer is zeroed
						_transmission_mode = TALKIE_SB_NONE;
						SPDR = TALKIE_SB_FULL;
					}
                    break;
                case TALKIE_SB_START:
					if (_transmission_mode == TALKIE_SB_SEND) {	// First char to be sent
						if (_sending_length == 0) {
							_transmission_mode = TALKIE_SB_NONE;
							SPDR = TALKIE_SB_NONE;
							#ifdef BROADCAST_SPI_ARDUINO_SLAVE_DEBUG_2
							Serial.println(F("\tNothing to be sent"));
							#endif
						} else if (_sending_index < TALKIE_BUFFER_SIZE) {	// Safe code
							SPDR = _sending_buffer[_sending_index++];
						} else {
							_sending_length = 0;	// Makes sure the sending buffer is zeroed
							_transmission_mode = TALKIE_SB_NONE;
							SPDR = TALKIE_SB_FULL;
						}
					} else {
						_transmission_mode = TALKIE_SB_SEND;
						_sending_index = 0;
						SPDR = TALKIE_SB_READY;	// Doing it at the end makes sure everything above was actually set
					}
                    break;
                case TALKIE_SB_END:
					if (_transmission_mode == TALKIE_SB_RECEIVE) {
						_received_length = _receiving_index;
						#ifdef BROADCAST_SPI_ARDUINO_SLAVE_DEBUG_1
						Serial.println(F("\tReceived message"));
						#endif
                    } else if (_transmission_mode == TALKIE_SB_SEND) {
                        _sending_length = 0;	// Makes sure the sending buffer is zeroed
						SPDR = TALKIE_SB_DONE;	// Doing it at the end makes sure everything above was actually set
						#ifdef BROADCAST_SPI_ARDUINO_SLAVE_DEBUG_1
						Serial.println(F("\tSent message"));
						#endif
					}
                    _transmission_mode = TALKIE_SB_NONE;
                    break;
                case TALKIE_SB_ACK:
                    SPDR = TALKIE_SB_ACK;
                    break;
				case TALKIE_SB_FULL:
					_transmission_mode = TALKIE_SB_NONE;
					_sending_length = 0;
					SPDR = TALKIE_SB_ACK;
                    break;
				case TALKIE_SB_ERROR:
					_transmission_mode = TALKIE_SB_NONE;
					SPDR = TALKIE_SB_ACK;
                    break;
                default:
                    SPDR = TALKIE_SB_NACK;
            }
        }
    }

};


#endif // BROADCAST_SPI_ARDUINO_SLAVE_HPP
