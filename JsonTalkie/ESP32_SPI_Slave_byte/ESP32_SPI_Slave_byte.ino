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

uint8_t _rx_buffer[DATA_SIZE] __attribute__((aligned(4)));
uint8_t _tx_buffer[DATA_SIZE] __attribute__((aligned(4)));
uint8_t _cmd_byte __attribute__((aligned(4)));

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
    bool slave_has_data = (random(100) < 10);
    
    memset(_tx_buffer, 0, DATA_SIZE);
    if (slave_has_data) {
        snprintf((char*)_tx_buffer, DATA_SIZE, "SlaveData_%lu", millis());
    }
    
    spi_slave_transaction_t t1 = {};
    t1.length = 8;
    t1.rx_buffer = &_cmd_byte;
    t1.tx_buffer = nullptr;
    spi_slave_transmit(VSPI_HOST, &t1, portMAX_DELAY);
    
    uint8_t d = (_cmd_byte >> 7) & 0x01;
    uint8_t l = _cmd_byte & 0x7F;
    
    Serial.printf("\n[Slave] Cmd: 0x%02X D=%d L=%d ", _cmd_byte, d, l);
    
    if (d == 0) {
        if (l > 0) {
            Serial.printf("Receiving %d bytes\n", l);
            delayMicroseconds(100);
            
            spi_slave_transaction_t t2 = {};
            t2.length = DATA_SIZE * 8;
            t2.rx_buffer = _rx_buffer;
            t2.tx_buffer = nullptr;
            spi_slave_transmit(VSPI_HOST, &t2, portMAX_DELAY);
            
            Serial.print("Received: ");
            for (int i = 0; i < l && i < 40; i++) {
                char c = _rx_buffer[i];
                if (c >= 32 && c <= 126) Serial.print(c);
                else Serial.printf("[%02X]", c);
            }
            Serial.println();
        } else {
            Serial.println("Master ping");
        }
    } else {
        uint8_t response_byte __attribute__((aligned(4)));
        int data_len = strlen((char*)_tx_buffer);
        if (data_len > 127) data_len = 127;
        
        if (slave_has_data) {
            response_byte = 0x80 | data_len;
            Serial.printf("Sending %d bytes\n", data_len);
        } else {
            response_byte = 0x80;
            Serial.println("No data");
        }
        
        spi_slave_transaction_t t2 = {};
        t2.length = 8;
        t2.rx_buffer = nullptr;
        t2.tx_buffer = &response_byte;
        spi_slave_transmit(VSPI_HOST, &t2, portMAX_DELAY);
        
        if (slave_has_data) {
            delayMicroseconds(100);
            spi_slave_transaction_t t3 = {};
            t3.length = DATA_SIZE * 8;
            t3.rx_buffer = nullptr;
            t3.tx_buffer = _tx_buffer;
            spi_slave_transmit(VSPI_HOST, &t3, portMAX_DELAY);
        }
    }
}
