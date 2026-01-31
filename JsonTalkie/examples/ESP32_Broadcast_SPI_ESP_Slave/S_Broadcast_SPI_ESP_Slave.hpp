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
#ifndef BROADCAST_SPI_ESP_SLAVE_HPP
#define BROADCAST_SPI_ESP_SLAVE_HPP


#include <BroadcastSocket.h>
extern "C" {
    #include "driver/spi_slave.h"
}


// #define BROADCAST_SPI_DEBUG
// #define BROADCAST_SPI_DEBUG_TIMING



class S_Broadcast_SPI_ESP_Slave : public BroadcastSocket {
public:

	// The Socket class description shouldn't be greater than 35 chars
	// {"m":7,"f":"","s":3,"b":1,"t":"","i":58485,"0":1,"1":"","2":11,"c":11266} <-- 128 - (73 + 2*10) = 35
    const char* class_description() const override { return "Broadcast_SPI_ESP_Slave"; }


	#ifdef BROADCAST_SPI_DEBUG_TIMING
	unsigned long _reference_time = millis();
	#endif

protected:

	enum SpiState {
		WAIT_CMD,
		RX_PAYLOAD,
		TX_PAYLOAD
	};

	bool _initiated = false;
	const spi_host_device_t _host;

	uint8_t _rx_buffer[TALKIE_BUFFER_SIZE] __attribute__((aligned(4)));
	uint8_t _tx_buffer[TALKIE_BUFFER_SIZE] __attribute__((aligned(4)));
	uint8_t _cmd_byte __attribute__((aligned(4)));

	SpiState _spi_state = WAIT_CMD;
	spi_slave_transaction_t _transaction;
	uint8_t _send_length = 0;


    // Constructor
    S_Broadcast_SPI_ESP_Slave(spi_host_device_t host) : BroadcastSocket(), _host(host) {
            
		_max_delay_ms = 0;  // SPI is sequencial, no need to control out of order packages
	}


    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    void _receive() override {

		static int stacked_transmissions = 0;

		if (_initiated) {
			
			// Checks if the queued element was consumed and needs to be processed
    		spi_slave_transaction_t *ret;
    		esp_err_t err = spi_slave_get_trans_result(_host, &ret, 0);
			if (err != ESP_OK) {
				// if (err == ESP_ERR_TIMEOUT) {
				// 	// The transaction we queued (via queue_cmd/queue_rx/queue_tx) 
				// 	// either never got queued properly or got dropped!
				// 	// The SPI communication pipe is EMPTY!
				// 	queue_cmd();  // ← MUST re-queue to restore communication!
				// }
				return;
			}

			// At this point a queued element is consumed, as to queue a new one afterwards !

			/* === SPI "ISR" === */

			const bool beacon = (_cmd_byte >> 7) & 0x01;
			const uint8_t cmd_length = _cmd_byte & 0x7F;

			switch (_spi_state) {

				case WAIT_CMD:
				{
					if (beacon) {
						if (cmd_length > 0 && cmd_length == _send_length) {	// beacon
							
							queue_tx(cmd_length);
							
							// #ifdef BROADCAST_SPI_DEBUG
							// 	Serial.printf("\n[CMD] 0x%02X beacon=%d len=%u\n", _cmd_byte, beacon, cmd_length);
							// #endif
							
						} else {
							queue_cmd();
						}

					} else if (cmd_length > 0 && cmd_length <= TALKIE_BUFFER_SIZE) {

						queue_rx(cmd_length);
						
						// #ifdef BROADCAST_SPI_DEBUG
						// 	Serial.printf("\n[CMD] 0x%02X beacon=%d len=%u\n",
						// 		_cmd_byte, beacon, cmd_length);
						// #endif

					} else {

						#ifdef BROADCAST_SPI_DEBUG
							Serial.println("Master ping");
						#endif

						queue_cmd();
					}
				}
				break;
				
				case RX_PAYLOAD:
				{

					// #ifdef BROADCAST_SPI_DEBUG
					// 	Serial.printf("Received %u bytes: ", cmd_length);
					// 	for (uint8_t i = 0; i < cmd_length; i++) {
					// 		char c = _rx_buffer[i];
					// 		if (c >= 32 && c <= 126) Serial.print(c);
					// 		else Serial.printf("[%02X]", c);
					// 	}
					// 	Serial.println();
					// #endif

					if (stacked_transmissions < 5) {

						JsonMessage new_message(
							reinterpret_cast<const char*>( _rx_buffer ),
							static_cast<size_t>( cmd_length )
						);

						// Needs the queue a new command, otherwise nothing is processed again (lock)
						// Real scenario if at this moment a payload is still in the queue to be sent and now
						// has no queue to be picked up
						queue_cmd();	// After the reading above to avoid _rx_buffer corruption
						
						stacked_transmissions++;
						_startTransmission(new_message);
						stacked_transmissions--;
						
					} else {

						// Shouldn't process more than 5 messages at once
						queue_cmd();
					}
				}
				break;
				
				case TX_PAYLOAD:
				{
					// #ifdef BROADCAST_SPI_DEBUG
					// 	Serial.printf("Sent %u bytes\n", cmd_length);
					// #endif

					_send_length = 0;	// payload was sent
					queue_cmd();
				}
				break;
			}
		}
    }

    
    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    bool _send(const JsonMessage& json_message) override {

		if (_initiated) {
			
			const uint16_t start_waiting = (uint16_t)millis();
			while (_send_length > 0) {
				// There is NO need to do any of this
    			// yield();          // or vTaskDelay(1)
    			// vTaskDelay(1);   // ← allows SPI driver + DMA completion

				_receive();	// keeps processing pending messages, mainly the ones pooled to be sent
				if ((uint16_t)millis() - start_waiting > 1 * 1000) {

					#ifdef BROADCAST_SPI_DEBUG
						Serial.println(F("\t_unlockSendingBuffer: NOT available sending buffer"));
					#endif

					return false;
				}
			}
			
			_send_length = (uint8_t)json_message.serialize_json(
				reinterpret_cast<char*>( _tx_buffer ),
				TALKIE_BUFFER_SIZE
			);
			return true;
		}
        return false;
    }

	
    // Specific methods associated to ESP SPI as Slave
	
	void queue_cmd() {
		_spi_state = WAIT_CMD;
    	static uint8_t length_latched  __attribute__((aligned(4)));       // persists across calls
		spi_slave_transaction_t *t = &_transaction;
		t->length    = 1 * 8;	// Bytes to bits
		// Full-Duplex
		t->rx_buffer = &_cmd_byte;
		// If you see 80 on the Master side it means the Slave wasn't given the time to respond!
		length_latched = _send_length;	// Avoids a racing to a shared variable (no race) (stable copy)
		t->tx_buffer = &length_latched;	// <-- EXTREMELY IMPORTANT LINE
		// THUS, NO NEED TO BE VOLATILE
		// t->tx_buffer = const_cast<const uint8_t*>(&_send_length);
		// t->tx_buffer = (const void*)(&_send_length);	// Also works
		spi_slave_queue_trans(_host, t, portMAX_DELAY);
	}

	void queue_rx(uint8_t len) {
		_spi_state = RX_PAYLOAD;
		spi_slave_transaction_t *t = &_transaction;
		t->length    = (size_t)len * 8;
		// Half-Duplex
		t->rx_buffer = _rx_buffer;
		t->tx_buffer = nullptr;
		spi_slave_queue_trans(_host, t, portMAX_DELAY);
	}

	void queue_tx(uint8_t len) {
		_spi_state = TX_PAYLOAD;
		spi_slave_transaction_t *t = &_transaction;
		t->length    = (size_t)len * 8;
		// Half-Duplex
		t->rx_buffer = nullptr;
		t->tx_buffer = _tx_buffer;
		spi_slave_queue_trans(_host, t, portMAX_DELAY);
	}


public:

    // Move ONLY the singleton instance method to subclass
    static S_Broadcast_SPI_ESP_Slave& instance(spi_host_device_t host = HSPI_HOST) {
        static S_Broadcast_SPI_ESP_Slave instance(host);

        return instance;
    }


    void begin(int mosi_io_num, int miso_io_num, int sclk_io_num, int spics_io_num) {
		
		spi_bus_config_t buscfg = {};
		buscfg.mosi_io_num = mosi_io_num;
		buscfg.miso_io_num = miso_io_num;
		buscfg.sclk_io_num = sclk_io_num;
		buscfg.max_transfer_sz = TALKIE_BUFFER_SIZE;
		
		// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/spi_master.html

		spi_slave_interface_config_t slvcfg = {};
		slvcfg.mode = 0;
		slvcfg.spics_io_num = spics_io_num;
		slvcfg.queue_size = 1;	// It's just 128 bytes maximum, so, a queue of 1 is all it needs

		spi_slave_initialize(_host, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
		queue_cmd();   // always armed
		_initiated = true;

		#ifdef BROADCAST_SPI_DEBUG
			Serial.println("Slave ready");
		#endif
    }

};


#endif // BROADCAST_SPI_ESP_SLAVE_HPP
