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

// DMA-capable, 4-byte aligned buffers
uint8_t rx_buffer[DATA_SIZE] __attribute__((aligned(4)));
uint8_t tx_buffer[DATA_SIZE] __attribute__((aligned(4)));

// Add this ONE line: global aligned variable
uint8_t cmd_byte __attribute__((aligned(4)));

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n=== SPI SLAVE ===");
    
    // SPI Slave config
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
    
    esp_err_t ret = spi_slave_initialize(VSPI_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        Serial.printf("SPI Slave init failed: 0x%X\n", ret);
        while(1);
    }
    
    Serial.println("Slave ready.");
}

void loop() {
    // 10% chance Slave has data
    bool slave_has_data = (random(100) < 10);
    
    if (slave_has_data) {
        snprintf((char*)tx_buffer, DATA_SIZE, 
                 "SlaveData_%lu:ADC=%d", millis(), analogRead(0));
    } else {
        memset(tx_buffer, 0, DATA_SIZE);
    }
    
    // Wait for master's command - uses global aligned variable
    spi_slave_transaction_t t1 = {};
    t1.length = 8;
    t1.rx_buffer = &cmd_byte;  // <-- Uses global aligned variable
    t1.tx_buffer = nullptr;
    
    spi_slave_transmit(VSPI_HOST, &t1, portMAX_DELAY);
    
    uint8_t direction = (cmd_byte >> 7) & 0x01;
    uint8_t length = cmd_byte & 0x7F;
    
    Serial.printf("\n[Slave] Cmd: 0x%02X (D=%d, L=%d) ", cmd_byte, direction, length);
    
    if (direction == 0) {
        Serial.print("M→S - ");
        
        if (length > 0) {
            Serial.println("Master has data, receiving...");
            
            delayMicroseconds(50);
            
            spi_slave_transaction_t t2 = {};
            t2.length = DATA_SIZE * 8;
            t2.rx_buffer = rx_buffer;
            t2.tx_buffer = nullptr;
            spi_slave_transmit(VSPI_HOST, &t2, portMAX_DELAY);
            
            Serial.print("  Received: ");
            for (int i = 0; i < 40 && rx_buffer[i] != 0; i++) {
                Serial.print((char)rx_buffer[i]);
            }
            Serial.println();
            
        } else {
            Serial.println("Master has no data");
        }
        
    } else {
        Serial.print("S→M - ");
        
        uint8_t slave_response_byte = slave_has_data ? 0x81 : 0x80;
        Serial.println(slave_has_data ? "I have data" : "I have no data");
        
        spi_slave_transaction_t t2 = {};
        t2.length = 8;
        t2.rx_buffer = nullptr;
        t2.tx_buffer = &slave_response_byte;  // <-- This might also need alignment!
        spi_slave_transmit(VSPI_HOST, &t2, portMAX_DELAY);
        
        if (slave_has_data) {
            delayMicroseconds(50);
            
            spi_slave_transaction_t t3 = {};
            t3.length = DATA_SIZE * 8;
            t3.rx_buffer = nullptr;
            t3.tx_buffer = tx_buffer;
            spi_slave_transmit(VSPI_HOST, &t3, portMAX_DELAY);
            
            Serial.print("  Sent: ");
            for (int i = 0; i < 40 && tx_buffer[i] != 0; i++) {
                Serial.print((char)tx_buffer[i]);
            }
            Serial.println();
        }
    }
}
