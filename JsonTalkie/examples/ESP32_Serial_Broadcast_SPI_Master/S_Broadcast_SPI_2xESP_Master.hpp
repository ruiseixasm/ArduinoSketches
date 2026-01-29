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
#ifndef BROADCAST_SPI_ESP2X_MASTER_HPP
#define BROADCAST_SPI_ESP2X_MASTER_HPP


#include <BroadcastSocket.h>
#include "driver/spi_master.h"


// #define BROADCAST_SPI_DEBUG
// #define BROADCAST_SPI_DEBUG_1
// #define BROADCAST_SPI_DEBUG_2
// #define BROADCAST_SPI_DEBUG_NEW
// #define BROADCAST_SPI_DEBUG_TIMING



class S_Broadcast_SPI_2xESP_Master : public BroadcastSocket {
public:

	// The Socket class description shouldn't be greater than 35 chars
	// {"m":7,"f":"","s":3,"b":1,"t":"","i":58485,"0":1,"1":"","2":11,"c":11266} <-- 128 - (73 + 2*10) = 35
    const char* class_description() const override { return "Broadcast_SPI_2xESP_Master"; }


	#ifdef BROADCAST_SPI_DEBUG_TIMING
	unsigned long _reference_time = millis();
	#endif

protected:

	bool _initiated = false;
    int* _spi_cs_pins;
    uint8_t _ss_pins_count = 0;
	
	spi_device_handle_t _spi;
	uint8_t _data_buffer[TALKIE_BUFFER_SIZE] __attribute__((aligned(4)));


    // Constructor
    S_Broadcast_SPI_2xESP_Master(int* ss_pins, uint8_t ss_pins_count) : BroadcastSocket() {
            
		_spi_cs_pins = ss_pins;
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
					
					// size_t length = receiveSPI(_ss_pins[ss_pin_i], message_buffer);
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

			// sendBroadcastSPI(_ss_pins, _ss_pins_count, message_buffer, message_length);
			
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


	bool initiate(int mosi_io_num, int miso_io_num, int sclk_io_num) {
		

		return _initiated;
	}

	
    // Specific methods associated to Arduino SPI as Master
	
	void broadcastLength(int* ss_pins, uint8_t ss_pins_count, uint8_t length) {
		uint8_t tx_byte __attribute__((aligned(4))) = 0b01111111 & length;
		spi_transaction_t t = {};
		t.length = 1 * 8;	// Bytes to bits
		t.tx_buffer = &tx_byte;
		t.rx_buffer = nullptr;

		for (uint8_t ss_pin_i = 0; ss_pin_i < ss_pins_count; ss_pin_i++) {
			digitalWrite(ss_pins[ss_pin_i], LOW);
		}    
		spi_device_transmit(_spi, &t);
		for (uint8_t ss_pin_i = 0; ss_pin_i < ss_pins_count; ss_pin_i++) {
			digitalWrite(ss_pins[ss_pin_i], HIGH);
		}
	}

	void broadcastPayload(int* ss_pins, uint8_t ss_pins_count, uint8_t length) {

		if (length > TALKIE_BUFFER_SIZE) return;
		
		Serial.print("broadcastPayload() first 10: ");
		for(int i = 0; i < length; i++) {
			Serial.printf("%02X ", _data_buffer[i]);
		}
		Serial.println();

		spi_transaction_t t = {};
		t.length = (size_t)length * 8;	// Bytes to bits
		t.tx_buffer = _data_buffer;
		t.rx_buffer = nullptr;

		for (uint8_t ss_pin_i = 0; ss_pin_i < ss_pins_count; ss_pin_i++) {
			digitalWrite(ss_pins[ss_pin_i], LOW);
		}
		spi_device_transmit(_spi, &t);
		for (uint8_t ss_pin_i = 0; ss_pin_i < ss_pins_count; ss_pin_i++) {
			digitalWrite(ss_pins[ss_pin_i], HIGH);
		}
	}


	uint8_t sendBeacon(int ss_pin, uint8_t length = 0) {
		uint8_t tx_byte __attribute__((aligned(4))) = 0b10000000 | length;
		uint8_t rx_byte __attribute__((aligned(4))) = 0;
		spi_transaction_t t = {};
		t.length = 1 * 8;	// Bytes to bits
		t.tx_buffer = &tx_byte;
		t.rx_buffer = &rx_byte;

		digitalWrite(ss_pin, LOW);
		spi_device_transmit(_spi, &t);
		digitalWrite(ss_pin, HIGH);

		return rx_byte;
	}

	void receivePayload(int ss_pin, uint8_t length = 0) {
		
		if (length > TALKIE_BUFFER_SIZE) return;
		
		spi_transaction_t t = {};
		t.length = (size_t)length * 8;	// Bytes to bits
		t.tx_buffer = nullptr;
		t.rx_buffer = _data_buffer;
		
		digitalWrite(ss_pin, LOW);
		spi_device_transmit(_spi, &t);
		digitalWrite(ss_pin, HIGH);
	}


public:

    // Move ONLY the singleton instance method to subclass
    static S_Broadcast_SPI_2xESP_Master& instance(int* ss_pins, uint8_t ss_pins_count) {
        static S_Broadcast_SPI_2xESP_Master instance(ss_pins, ss_pins_count);

        return instance;
    }


    virtual void begin(int mosi_io_num, int miso_io_num, int sclk_io_num) {
		
		// ================== CONFIGURE SS PINS ==================
		// CRITICAL: Configure all SS pins as outputs and set HIGH
		for (uint8_t i = 0; i < _ss_pins_count; i++) {
			pinMode(_spi_cs_pins[i], OUTPUT);
			digitalWrite(_spi_cs_pins[i], HIGH);
			delayMicroseconds(10); // Small delay between pins
		}

		spi_bus_config_t buscfg = {};
		buscfg.mosi_io_num = mosi_io_num;
		buscfg.miso_io_num = miso_io_num;
		buscfg.sclk_io_num = sclk_io_num;
		buscfg.quadwp_io_num = -1;
		buscfg.quadhd_io_num = -1;
		buscfg.max_transfer_sz = TALKIE_BUFFER_SIZE;
		
		// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/spi_master.html

		spi_device_interface_config_t devcfg = {};
		devcfg.clock_speed_hz = 4000000;  // 4 MHz - Sweet spot!
		devcfg.mode = 0;
		devcfg.queue_size = 3;
		devcfg.spics_io_num = -1,  // DISABLE hardware CS completely! (Broadcast)
		
		
		spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
		spi_bus_add_device(HSPI_HOST, &devcfg, &_spi);
		
		for (uint8_t ss_pin_i = 0; ss_pin_i < sizeof(_spi_cs_pins)/sizeof(int); ss_pin_i++) {
			pinMode(_spi_cs_pins[ss_pin_i], OUTPUT);
			digitalWrite(_spi_cs_pins[ss_pin_i], HIGH);
		}

		_initiated = true;

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
    }
};



#endif // BROADCAST_SPI_ESP2X_MASTER_HPP
