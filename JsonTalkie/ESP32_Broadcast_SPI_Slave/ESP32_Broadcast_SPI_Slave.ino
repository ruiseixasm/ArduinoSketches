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
#define DATA_SIZE 128
// Convert 3000ms to FreeRTOS ticks
#define TIMEOUT_MS 3000

uint8_t rx_buffer[DATA_SIZE] __attribute__((aligned(4)));
uint8_t tx_buffer[DATA_SIZE] __attribute__((aligned(4)));
uint8_t cmd_byte __attribute__((aligned(4)));
uint8_t sending_length __attribute__((aligned(4))) = 0;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = 23;
    buscfg.miso_io_num = 19;
    buscfg.sclk_io_num = 18;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = DATA_SIZE;
    
    spi_slave_interface_config_t slvcfg = {};
    slvcfg.mode = 0;
    slvcfg.spics_io_num = VSPI_CS;
    slvcfg.queue_size = 3;
    
    spi_slave_initialize(VSPI_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
    Serial.println("Slave ready");
}


void loop() {
	
	// Convert milliseconds to FreeRTOS ticks
	const TickType_t timeout_ticks = TIMEOUT_MS / portTICK_PERIOD_MS;

    if (sending_length == 0) {
        int data_len = 0;
        bool slave_has_data = (random(100) < 10);
        if (slave_has_data) {
            data_len = snprintf((char*)tx_buffer, DATA_SIZE, "SlaveData_%lu", millis());
            if (data_len > 127) data_len = 127;
            sending_length = (uint8_t)data_len;
        }
    }

    spi_slave_transaction_t t = {};
    t.length = 1 * 8;	// Bytes to bits
    t.rx_buffer = &cmd_byte;
    t.tx_buffer = &sending_length;  // Always send it's size
    spi_slave_transmit(VSPI_HOST, &t, timeout_ticks);
    

    bool beacon = (bool)((cmd_byte >> 7) & 0x01);
    uint8_t received_length = cmd_byte & ~0b10000000;
    if (!beacon) {	// Master sends
		
    	Serial.printf("\n[From Master] Cmd: 0x%02X Beacon=0 L=%d ", cmd_byte, received_length);
    
        if (received_length > 0) {
			
			if (received_length <= DATA_SIZE) {
	
				Serial.printf("Receiving %d bytes\n", received_length);
				delayMicroseconds(100);
				
				t.length = received_length * 8;	// Bytes to bits
				t.rx_buffer = rx_buffer;
				t.tx_buffer = nullptr;
				spi_slave_transmit(VSPI_HOST, &t, timeout_ticks);
				
				Serial.print("Received: ");
				for (int i = 0; i < received_length; i++) {
					char c = rx_buffer[i];
					if (c >= 32 && c <= 126) Serial.print(c);
					else Serial.printf("[%02X]", c);
				}
				Serial.println();
			}
        } else {
            Serial.println("Master ping");
        }

    } else if (received_length > 0) {    // Beacon with length
        
    	Serial.printf("\n[From Slave] Cmd: 0x%02X Beacon=1 L=%d ", cmd_byte, received_length);
    
        delayMicroseconds(100);
        t.length = (size_t)received_length * 8;	// Bytes to bits
        t.rx_buffer = nullptr;
        t.tx_buffer = tx_buffer;
        spi_slave_transmit(VSPI_HOST, &t, timeout_ticks);

        Serial.printf("Sending %d bytes\n", received_length);
        sending_length = 0;    // Payload sent
    } else {
        Serial.println("No data to be sent");
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

