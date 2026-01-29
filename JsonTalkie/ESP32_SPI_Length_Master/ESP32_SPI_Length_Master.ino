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
#include "driver/spi_master.h"

#define HSPI_CS 15
#define DATA_SIZE 128

spi_device_handle_t _spi;
uint8_t _data_buffer[DATA_SIZE] __attribute__((aligned(4)));

void send1Byte(uint8_t data) {
    static uint8_t tx_byte __attribute__((aligned(4))) = 0;
    tx_byte = data;
    spi_transaction_t t = {};
    t.length = 1 * 8;	// Bytes to bits
    t.tx_buffer = &tx_byte;
    t.rx_buffer = nullptr;
    spi_device_transmit(_spi, &t);
}

uint8_t sendBeacon() {
    static uint8_t rx_byte __attribute__((aligned(4))) = 0;
    spi_transaction_t t = {};
    t.length = 1 * 8;	// Bytes to bits
    t.tx_buffer = nullptr;
    t.rx_buffer = &rx_byte;
    spi_device_transmit(_spi, &t);
    return rx_byte;
}

void sendLengthBytes(size_t length) {

	if (length > DATA_SIZE) return;
	
    Serial.print("sendLengthBytes() first 10: ");
    for(int i = 0; i < length; i++) {
        Serial.printf("%02X ", _data_buffer[i]);
    }
    Serial.println();

    spi_transaction_t t = {};
    t.length = length * 8;	// Bytes to bits
    t.tx_buffer = _data_buffer;
    t.rx_buffer = nullptr;
    spi_device_transmit(_spi, &t);
}

void receivePayload(size_t length) {
	
	if (length > DATA_SIZE) return;
	
    spi_transaction_t t = {};
    t.length = length * 8;	// Bytes to bits
    t.tx_buffer = nullptr;
    t.rx_buffer = _data_buffer;
    spi_device_transmit(_spi, &t);
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    randomSeed(analogRead(0));
    
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = 13;
    buscfg.miso_io_num = 12;
    buscfg.sclk_io_num = 14;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = DATA_SIZE;
    
    spi_device_interface_config_t devcfg = {};
    devcfg.clock_speed_hz = 1000000;
    devcfg.mode = 0;
    devcfg.spics_io_num = HSPI_CS;
    devcfg.queue_size = 3;
    
    spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    spi_bus_add_device(HSPI_HOST, &devcfg, &_spi);
    
    Serial.println("Master ready");
}

void loop() {
    static uint32_t cycle = 0;
    Serial.printf("\n=== Cycle %lu ===\n", cycle++);
    
    if (random(100) < 10) {
		
        int len = snprintf((char*)_data_buffer, 128, "MasterData_%lu:Value=%ld", millis(), random(10000));
        if (len > 127) len = 127;
        
        send1Byte(len); // D=0, L=len
        delayMicroseconds(200);
        sendLengthBytes(len);
        
        Serial.printf("[From Master] Sent %d bytes\n", len);
    } else {
        send1Byte(0b10000000); // D=1, L=0
        delayMicroseconds(200);
        
        uint8_t response = sendBeacon();
        uint8_t d = (response >> 7) & 0x01;
        uint8_t l = response & 0x7F;
        
        Serial.printf("[From Beacon] Slave: 0x%02X D=%d L=%d\n", response, d, l);
        
        if (d == 1 && l > 0) {
            delayMicroseconds(200);
            receivePayload(l);
            Serial.print("Received: ");
            for (int i = 0; i < l; i++) {
                Serial.print((char)_data_buffer[i]);
            }
            Serial.println();
        }
    }
    
    delay(1000);
}
