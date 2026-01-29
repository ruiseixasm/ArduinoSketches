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

#include <Arduino.h>
#include "driver/spi_slave.h"

#define VSPI_CS   5
#define DATA_SIZE 128

enum SpiState {
    WAIT_CMD,
    RX_PAYLOAD,
    TX_PAYLOAD
};

uint8_t _rx_buffer[DATA_SIZE] __attribute__((aligned(4)));
uint8_t _tx_buffer[DATA_SIZE] __attribute__((aligned(4)));
uint8_t _cmd_byte __attribute__((aligned(4)));
uint8_t _sending_length __attribute__((aligned(4))) = 0;

spi_slave_transaction_t _cmd_trans;
spi_slave_transaction_t _data_trans;

SpiState _spi_state = WAIT_CMD;
uint8_t _active_length = 0;


static inline void queue_cmd() {
    spi_slave_transaction_t *t = &_cmd_trans;
    t->length    = 8;
    t->rx_buffer = &_cmd_byte;
    t->tx_buffer = &_sending_length;
    spi_slave_queue_trans(VSPI_HOST, t, portMAX_DELAY);
}

static inline void queue_rx(uint8_t len) {
    spi_slave_transaction_t *t = &_data_trans;
    t->length    = (size_t)len * 8;
    t->rx_buffer = _rx_buffer;
    t->tx_buffer = nullptr;
    spi_slave_queue_trans(VSPI_HOST, t, portMAX_DELAY);
}

static inline void queue_tx(uint8_t len) {
    spi_slave_transaction_t *t = &_data_trans;
    t->length    = (size_t)len * 8;
    t->rx_buffer = nullptr;
    t->tx_buffer = _tx_buffer;
    spi_slave_queue_trans(VSPI_HOST, t, portMAX_DELAY);
}


void setup() {
    Serial.begin(115200);
    delay(2000);

    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = 23;
    buscfg.miso_io_num = 19;
    buscfg.sclk_io_num = 18;
    buscfg.max_transfer_sz = DATA_SIZE;

    spi_slave_interface_config_t slvcfg = {};
    slvcfg.mode = 0;
    slvcfg.spics_io_num = VSPI_CS;
    slvcfg.queue_size = 3;

    spi_slave_initialize(VSPI_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
    Serial.println("Slave ready");

    queue_cmd();   // always armed
}


void loop() {
    spi_slave_transaction_t *ret;

    /* === ORIGINAL DATA GENERATION LOGIC (UNCHANGED) === */
    if (_sending_length == 0) {
        int data_len = 0;
        bool slave_has_data = (random(100) < 10);
        if (slave_has_data) {
            data_len = snprintf((char*)_tx_buffer,
                                DATA_SIZE,
                                "SlaveData_%lu",
                                millis());
            if (data_len > 127) data_len = 127;
            _sending_length = (uint8_t)data_len;
        }
    }

    if (spi_slave_get_trans_result(VSPI_HOST, &ret, 0) != ESP_OK) {
        return;
    }

    /* === SPI "ISR" === */

	switch (_spi_state) {

		case WAIT_CMD:
		{
			bool beacon = (_cmd_byte >> 7) & 0x01;
			uint8_t received_length = _cmd_byte & 0x7F;

			Serial.printf("\n[CMD] 0x%02X beacon=%d len=%u\n",
						_cmd_byte, beacon, received_length);

			if (!beacon) {  // master → slave
				if (received_length > 0 && received_length <= DATA_SIZE) {
					_active_length = received_length;
					_spi_state = RX_PAYLOAD;
					queue_rx(received_length);
					return;
				} else {
					Serial.println("Master ping");
				}

			} else if (received_length > 0 && received_length == _sending_length) {	// beacon
				_active_length = received_length;
				_spi_state = TX_PAYLOAD;
				queue_tx(received_length);
				return;
			}

			queue_cmd();
		}
		break;
		
		case RX_PAYLOAD:
		{
			Serial.printf("Received %u bytes: ", _active_length);
			for (uint8_t i = 0; i < _active_length; i++) {
				char c = _rx_buffer[i];
				if (c >= 32 && c <= 126) Serial.print(c);
				else Serial.printf("[%02X]", c);
			}
			Serial.println();

			_spi_state = WAIT_CMD;
			queue_cmd();
		}
		break;
		
		case TX_PAYLOAD:
		{
			Serial.printf("Sent %u bytes\n", _active_length);
			_sending_length = 0;

			_spi_state = WAIT_CMD;
			queue_cmd();
		}
		break;
		
		default: break;
	}
}



// // LOGIC TO BE USED BY SPI SLAVES

// // once
// queue_idle_transaction();

// // forever
// if (spi_slave_get_trans_result(host, &t, 0) == ESP_OK) {
//     process(t);
//     queue_next_transaction();
// }

