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
#ifndef BROADCAST_SPI_ESP_ARDUINO_MASTER_HPP
#define BROADCAST_SPI_ESP_ARDUINO_MASTER_HPP


#include <BroadcastSocket.h>
#include <SPI.h>

// #define BROADCAST_SPI_DEBUG
// #define BROADCAST_SPI_DEBUG_1
// #define BROADCAST_SPI_DEBUG_2
// #define BROADCAST_SPI_DEBUG_NEW
// #define BROADCAST_SPI_DEBUG_TIMING


#define send_delay_us 10
#define receive_delay_us 18
#define TALKIE_MAX_NAMES 8


class S_Broadcast_SPI_ESP_Arduino_Master : public BroadcastSocket {
public:

	// The Socket class description shouldn't be greater than 35 chars
	// {"m":7,"f":"","s":3,"b":1,"t":"","i":58485,"0":1,"1":"","2":11,"c":11266} <-- 128 - (73 + 2*10) = 35
    const char* class_description() const override { return "Broadcast_SPI_ESP_Arduino_Master"; }

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


    // Constructor
    S_Broadcast_SPI_ESP_Arduino_Master(int* ss_pins, uint8_t ss_pins_count) : BroadcastSocket() {
            
		_ss_pins = ss_pins;
		_ss_pins_count = ss_pins_count;
		_max_delay_ms = 0;  // SPI is sequencial, no need to control out of order packages
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
						_startTransmission(new_message);
					}
				}
			}
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
			
			#ifdef BROADCAST_SPI_DEBUG_TIMING
			Serial.print(" | ");
			Serial.print(millis() - _reference_time);
			#endif

			const char* message_buffer = json_message._read_buffer();
			size_t message_length = json_message.get_length();

			sendBroadcastSPI(_ss_pins, _ss_pins_count, message_buffer, message_length);
			
			#ifdef BROADCAST_SPI_DEBUG
			Serial.println(F("\t\t\t\t\tsend4: --> Broadcast sent to all pins -->"));
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

	
    // Specific methods associated to Arduino SPI as Master
	
    bool sendBroadcastSPI(int* ss_pins, uint8_t ss_pins_count, const char* message_buffer, size_t length) {
		
		#ifdef BROADCAST_SPI_ARDUINO2X_MASTER_DEBUG_1
		Serial.print(F("\tSending on pin: "));
		Serial.println(ss_pin);
		#endif

		if (length > TALKIE_BUFFER_SIZE) {
			
			#ifdef BROADCAST_SPI_ARDUINO2X_MASTER_DEBUG_1
			Serial.println(F("\tlength > TALKIE_BUFFER_SIZE"));
			#endif

			return false;
		}

		// SENDS IN BROADCAST MODE, NO REPLY CONTROL (c = ...) !!

		if (length > 0) {	// Don't send empty strings
			
			for (uint8_t ss_pin_i = 0; ss_pin_i < ss_pins_count; ss_pin_i++) {
				digitalWrite(ss_pins[ss_pin_i], LOW);
			}

			for (uint8_t receive_sends = 0; receive_sends < 2; ++receive_sends) {
				delayMicroseconds(send_delay_us);
				_spi_instance->transfer(TALKIE_SB_RECEIVE);
			}

			for (size_t sending_index = 0; sending_index < length; ++sending_index) {
				delayMicroseconds(send_delay_us);
				_spi_instance->transfer(message_buffer[sending_index]);
			}

			for (uint8_t end_sends = 0; end_sends < 2; ++end_sends) {
				delayMicroseconds(send_delay_us);
				_spi_instance->transfer(TALKIE_SB_END);
			}

			delayMicroseconds(5);
			for (uint8_t ss_pin_i = 0; ss_pin_i < ss_pins_count; ss_pin_i++) {
				digitalWrite(ss_pins[ss_pin_i], HIGH);
			}
			
        	return true;
		}
        return false;
    }


    size_t receiveSPI(int ss_pin, char* message_buffer, size_t buffer_size = TALKIE_BUFFER_SIZE) {
        size_t length = 0;	// No interrupts, so, not volatile
        uint8_t c;			// Avoid using 'char' while using values above 127

		#ifdef BROADCAST_SPI_ARDUINO2X_MASTER_DEBUG_2
		Serial.print(F("\tReceiving on pin: "));
		Serial.println(ss_pin);
		#endif

		size_t receiving_index = 0;
		for (uint8_t receive_tries = 0; receiving_index == 0 && receive_tries < 3; receive_tries++) {
			
			digitalWrite(ss_pin, LOW);
			
			for (uint8_t get_ready_tries = 0; get_ready_tries < 3; ++get_ready_tries) {
				delayMicroseconds(receive_delay_us);
				c = _spi_instance->transfer(TALKIE_SB_START);
				if (c == TALKIE_SB_READY) {
					
					while (1) {
						
						if (receiving_index < buffer_size) {
							delayMicroseconds(receive_delay_us);
							c = _spi_instance->transfer(TALKIE_SB_SEND);

							if (c < 128) {
								message_buffer[receiving_index++] = c;
								continue;
							} else if (c == TALKIE_SB_END) {
								for (uint8_t end_tries = 0; end_tries < 5; ++end_tries) {
									delayMicroseconds(receive_delay_us);
									c = _spi_instance->transfer(TALKIE_SB_END);
									if (c == TALKIE_SB_DONE) {
										length = receiving_index;
										receiving_index = 1;	// Finish transmission
										goto close_transmission;
									}
								}
							} else if (c == TALKIE_SB_NONE || c == TALKIE_SB_FULL) {
								receiving_index = 1;	// Finish transmission
								goto close_transmission;
							}
							_spi_instance->transfer(TALKIE_SB_ERROR);
							receiving_index = 0;	// Retry transmission
							goto close_transmission;
						} else {
							delayMicroseconds(receive_delay_us);
							_spi_instance->transfer(TALKIE_SB_FULL);
							receiving_index = 1;	// Finish transmission
							goto close_transmission;
						}
					}
				}
			}

			close_transmission:
            delayMicroseconds(5);
            digitalWrite(ss_pin, HIGH);
        }
        return length;
    }
	

public:

    // Move ONLY the singleton instance method to subclass
    static S_Broadcast_SPI_ESP_Arduino_Master& instance(int* ss_pins, uint8_t ss_pins_count) {
        static S_Broadcast_SPI_ESP_Arduino_Master instance(ss_pins, ss_pins_count);

        return instance;
    }


    virtual void begin(SPIClass* spi_instance) {
		
		_spi_instance = spi_instance;
		initiate();
    }
};



#endif // BROADCAST_SPI_ESP_ARDUINO_MASTER_HPP
