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

#define VSPI_CS 5
#define FRAME_SIZE 129
#define MAX_PAYLOAD 128

// DMA-capable, 4-byte aligned
uint8_t rx_frame[FRAME_SIZE] __attribute__((aligned(4)));
uint8_t tx_frame[FRAME_SIZE] __attribute__((aligned(4)));

void setup() {
	Serial.begin(115200);

	// SPI bus configuration for VSPI
	spi_bus_config_t buscfg = {};
	buscfg.mosi_io_num = 23; // master MOSI → slave MOSI
	buscfg.miso_io_num = 19; // master MISO → slave MISO
	buscfg.sclk_io_num = 18; // clock
	buscfg.quadwp_io_num = -1;
	buscfg.quadhd_io_num = -1;
	buscfg.max_transfer_sz = FRAME_SIZE;

	// SPI slave interface configuration
	spi_slave_interface_config_t slvcfg = {};
	slvcfg.spics_io_num = VSPI_CS;
	slvcfg.queue_size = 1;

	spi_slave_initialize(VSPI_HOST, &buscfg, &slvcfg, 1);

	// prefill tx_frame with default reply
	const char *reply = "Hello from SLAVE!";
	tx_frame[0] = strlen(reply);
	memset(&tx_frame[1], 0, MAX_PAYLOAD);
	memcpy(&tx_frame[1], reply, strlen(reply));

	Serial.println("SLAVE ready on VSPI");
}

void loop() {
	// Wait for master transaction
	spi_slave_transaction_t t = {};
	t.length = FRAME_SIZE * 8;
	t.rx_buffer = rx_frame;
	t.tx_buffer = tx_frame;
	spi_slave_transmit(VSPI_HOST, &t, portMAX_DELAY);

	uint8_t len = rx_frame[0];

	if (len > 0) {
	// Master broadcast received → print payload as ASCII
	Serial.print("SLAVE received: ");
	for (int i = 1; i <= len; i++) {
		char c = rx_frame[i];
		if (c >= 32 && c <= 126) Serial.print(c);
		else Serial.print('.');	// non-printable → dot
	}
	Serial.println();

	// prepare tx_frame for next master poll
	const char *reply = "Hello from SLAVE!";
	tx_frame[0] = strlen(reply);
	memset(&tx_frame[1], 0, MAX_PAYLOAD);
	memcpy(&tx_frame[1], reply, strlen(reply));
	} else {
	// Master sent length=0 → indicates TX buffer was consumed
	// optional: clear tx_frame
	// tx_frame[0] = 0;
	}
}
