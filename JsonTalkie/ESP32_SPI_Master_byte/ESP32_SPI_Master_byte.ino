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

spi_device_handle_t spi;
uint8_t data_buffer[DATA_SIZE];

void setup() {
    Serial.begin(115200);
    delay(2000);
    randomSeed(analogRead(0));
    
    Serial.println("\n=== SPI MASTER ===");
    Serial.println("Protocol: L field = sender data availability");
    Serial.println("If Master has data: D=0, L>0");
    Serial.println("If Master has NO data: D=1, L>0 (poll Slave)");
    
    // HSPI Master config
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
    
    esp_err_t ret = spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        Serial.printf("SPI Bus init failed: 0x%X\n", ret);
        while(1);
    }
    
    ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
    if (ret != ESP_OK) {
        Serial.printf("SPI Device add failed: 0x%X\n", ret);
        while(1);
    }
    
    Serial.println("Master ready. Starting 1-second cycles...");
}

// Send 1 byte
void send1Byte(uint8_t data) {
    spi_transaction_t t = {};
    t.length = 8;
    t.tx_buffer = &data;
    t.rx_buffer = nullptr;
    spi_device_transmit(spi, &t);
}

// Send 128 bytes
void send128Bytes() {
    spi_transaction_t t = {};
    t.length = DATA_SIZE * 8;
    t.tx_buffer = data_buffer;
    t.rx_buffer = nullptr;
    spi_device_transmit(spi, &t);
}

// Receive 1 byte
uint8_t receive1Byte() {
    uint8_t response;
    spi_transaction_t t = {};
    t.length = 8;
    t.tx_buffer = nullptr;
    t.rx_buffer = &response;
    spi_device_transmit(spi, &t);
    return response;
}

// Receive 128 bytes
void receive128Bytes() {
    spi_transaction_t t = {};
    t.length = DATA_SIZE * 8;
    t.tx_buffer = nullptr;
    t.rx_buffer = data_buffer;
    spi_device_transmit(spi, &t);
}

void loop() {
    static uint32_t cycle = 0;
    
    Serial.printf("\n=== Cycle %lu ===\n", cycle++);
    
    // 10% chance Master has data
    bool master_has_data = (random(100) < 10);
    
    uint8_t cmd_byte;
    
    if (master_has_data) {
        // Master has data → D=0, L>0
        cmd_byte = 0x01;  // D=0, L=1 (Master has data)
        Serial.println("[Master] Has data: D=0, L=1 (M→S)");
        
        // Prepare data
        snprintf((char*)data_buffer, DATA_SIZE,
                 "MasterData_%lu:Value=%d", millis(), random(10000));
        
        // Send command byte
        send1Byte(cmd_byte);
        delayMicroseconds(50);
        
        // Send 128 bytes
        send128Bytes();
        Serial.print("  Sent: ");
        for (int i = 0; i < 40 && data_buffer[i] != 0; i++) {
            Serial.print((char)data_buffer[i]);
        }
        Serial.println();
        
    } else {
        // Master has NO data → D=1, L>0 (poll Slave)
        cmd_byte = 0x81;  // D=1, L=1 (Master polling, expects Slave response)
        Serial.println("[Master] No data: D=1, L=1 (poll Slave)");
        
        // Send command byte
        send1Byte(cmd_byte);
        delayMicroseconds(50);
        
        // Get Slave's response byte
        uint8_t slave_response = receive1Byte();
        uint8_t slave_direction = (slave_response >> 7) & 0x01;
        uint8_t slave_length = slave_response & 0x7F;
        
        Serial.printf("  Slave response: 0x%02X (D=%d, L=%d) ", 
                     slave_response, slave_direction, slave_length);
        
        if (slave_direction == 1 && slave_length > 0) {
            // Slave indicates it has data (D=1, L>0)
            Serial.println("Slave has data, receiving...");
            
            delayMicroseconds(50);
            receive128Bytes();
            
            Serial.print("  Received: ");
            for (int i = 0; i < 40 && data_buffer[i] != 0; i++) {
                Serial.print((char)data_buffer[i]);
            }
            Serial.println();
            
        } else if (slave_direction == 1 && slave_length == 0) {
            // Slave has no data (D=1, L=0)
            Serial.println("Slave has no data");
            
        } else {
            // Unexpected response
            Serial.printf("Unexpected response from Slave!\n");
        }
    }
    
    // Clear buffer
    memset(data_buffer, 0, DATA_SIZE);
    
    // 1-second cycle
    delay(1000);
}
