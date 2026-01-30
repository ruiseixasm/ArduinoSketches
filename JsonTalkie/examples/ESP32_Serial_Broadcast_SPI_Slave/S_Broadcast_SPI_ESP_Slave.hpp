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
// #define BROADCAST_SPI_DEBUG_1
// #define BROADCAST_SPI_DEBUG_2
// #define BROADCAST_SPI_DEBUG_NEW
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
	uint8_t _sending_length __attribute__((aligned(4))) = 0;

	spi_slave_transaction_t _cmd_trans;
	spi_slave_transaction_t _data_trans;

	SpiState _spi_state = WAIT_CMD;
	uint8_t _active_length = 0;


    // Constructor
    S_Broadcast_SPI_ESP_Slave(spi_host_device_t host) : BroadcastSocket(), _host(host) {
            
		_max_delay_ms = 0;  // SPI is sequencial, no need to control out of order packages
	}


    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    void _receive() override {

		
    }

    
    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    bool _send(const JsonMessage& json_message) override {

		if (_initiated) {
			

			return true;
		}
        return false;
    }


	bool initiate(int mosi_io_num, int miso_io_num, int sclk_io_num) {
		

		return _initiated;
	}

	
    // Specific methods associated to ESP SPI as Slave
	
	void queue_cmd() {
		spi_slave_transaction_t *t = &_cmd_trans;
		t->length    = 8;
		t->rx_buffer = &_cmd_byte;
		t->tx_buffer = &_sending_length;
		spi_slave_queue_trans(_host, t, portMAX_DELAY);
	}

	void queue_rx(uint8_t len) {
		spi_slave_transaction_t *t = &_data_trans;
		t->length    = (size_t)len * 8;
		t->rx_buffer = _rx_buffer;
		t->tx_buffer = nullptr;
		spi_slave_queue_trans(_host, t, portMAX_DELAY);
	}

	void queue_tx(uint8_t len) {
		spi_slave_transaction_t *t = &_data_trans;
		t->length    = (size_t)len * 8;
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
		slvcfg.queue_size = 3;

		spi_slave_initialize(_host, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
		Serial.println("Slave ready");

		queue_cmd();   // always armed

		_initiated = true;
    }

};



#endif // BROADCAST_SPI_ESP_SLAVE_HPP
