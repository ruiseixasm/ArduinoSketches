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

uint8_t rx_buffer[DATA_SIZE];
uint8_t tx_buffer[DATA_SIZE];

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n=== SPI SLAVE ===");
    Serial.println("Protocol: L field indicates data availability");
    Serial.println("D=0: Master→Slave, L>0=Master has data");
    Serial.println("D=1: Slave→Master, L>0=Slave has data");
    
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
    
    Serial.println("Slave ready. 10% chance to have data each cycle.");
}

void loop() {
    // 10% chance Slave has data
    bool slave_has_data = (random(100) < 10);
    
    if (slave_has_data) {
        // Generate data
        snprintf((char*)tx_buffer, DATA_SIZE, 
                 "SlaveData_%lu:ADC=%d", millis(), analogRead(0));
    } else {
        memset(tx_buffer, 0, DATA_SIZE);
    }
    
    // Wait for master's command
    uint8_t cmd_byte;
    spi_slave_transaction_t t1 = {};
    t1.length = 8;
    t1.rx_buffer = &cmd_byte;
    t1.tx_buffer = nullptr;  // Slave doesn't send during command phase
    
    esp_err_t ret = spi_slave_transmit(VSPI_HOST, &t1, portMAX_DELAY);
    
    uint8_t direction = (cmd_byte >> 7) & 0x01;
    uint8_t length = cmd_byte & 0x7F;
    
    Serial.printf("\n[Slave] Cmd: 0x%02X (D=%d, L=%d) ", cmd_byte, direction, length);
    
    if (direction == 0) {
        // D=0: Master → Slave direction
        Serial.print("M→S - ");
        
        if (length > 0) {
            // Master indicates it has data (L>0)
            Serial.println("Master has data, receiving...");
            
            delayMicroseconds(50);
            
            // Receive 128 bytes from Master
            spi_slave_transaction_t t2 = {};
            t2.length = DATA_SIZE * 8;
            t2.rx_buffer = rx_buffer;
            t2.tx_buffer = nullptr;
            spi_slave_transmit(VSPI_HOST, &t2, portMAX_DELAY);
            
            // Show received data
            Serial.print("  Received: ");
            for (int i = 0; i < 40 && rx_buffer[i] != 0; i++) {
                Serial.print((char)rx_buffer[i]);
            }
            Serial.println();
            
        } else {
            // L=0: Master has no data
            Serial.println("Master has no data (just direction)");
        }
        
    } else {
        // D=1: Slave → Master direction
        Serial.print("S→M - ");
        
        // IMPORTANT: Slave responds with its own L value
        uint8_t slave_response_byte;
        if (slave_has_data) {
            slave_response_byte = 0x81;  // D=1, L=1 (Slave has data)
            Serial.println("I have data, sending...");
        } else {
            slave_response_byte = 0x80;  // D=1, L=0 (Slave has no data)
            Serial.println("I have no data");
        }
        
        // Send response byte
        spi_slave_transaction_t t2 = {};
        t2.length = 8;
        t2.rx_buffer = nullptr;
        t2.tx_buffer = &slave_response_byte;
        spi_slave_transmit(VSPI_HOST, &t2, portMAX_DELAY);
        
        if (slave_has_data) {
            delayMicroseconds(50);
            
            // Send 128 bytes
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
