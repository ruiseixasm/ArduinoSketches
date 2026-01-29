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

// LED_BUILTIN is already defined by ESP32 platform
// Typically GPIO2 for most ESP32 boards
#ifndef LED_BUILTIN
  #define LED_BUILTIN 2  // Fallback definition if not already defined
#endif

#define HSPI_CS 15
#define DATA_SIZE 128

spi_device_handle_t spi;
uint8_t data_buffer[DATA_SIZE] __attribute__((aligned(4)));

int spi_cs_pins[] = {4, HSPI_CS};


void broadcastLength(int* ss_pins, uint8_t ss_pins_count, uint8_t length) {
    uint8_t tx_byte __attribute__((aligned(4))) = 0b01111111 & length;
    spi_transaction_t t = {};
    t.length = 1 * 8;	// Bytes to bits
    t.tx_buffer = &tx_byte;
    t.rx_buffer = nullptr;

    for (uint8_t ss_pin_i = 0; ss_pin_i < ss_pins_count; ss_pin_i++) {
        digitalWrite(ss_pins[ss_pin_i], LOW);
    }
	delayMicroseconds(20);  // CS setup time
    
    spi_device_polling_transmit(spi, &t);

    delayMicroseconds(10);	// Ensure transmission is complete before releasing CS
    for (uint8_t ss_pin_i = 0; ss_pin_i < ss_pins_count; ss_pin_i++) {
        digitalWrite(ss_pins[ss_pin_i], HIGH);
    }
}

void broadcastPayload(int* ss_pins, uint8_t ss_pins_count, uint8_t length) {

	if (length > DATA_SIZE) return;
	
    Serial.print("broadcastPayload() first 10: ");
    for(int i = 0; i < length; i++) {
        Serial.printf("%02X ", data_buffer[i]);
    }
    Serial.println();

    spi_transaction_t t = {};
    t.length = (size_t)length * 8;	// Bytes to bits
    t.tx_buffer = data_buffer;
    t.rx_buffer = nullptr;

    for (uint8_t ss_pin_i = 0; ss_pin_i < ss_pins_count; ss_pin_i++) {
        digitalWrite(ss_pins[ss_pin_i], LOW);
    }
    spi_device_polling_transmit(spi, &t);
    for (uint8_t ss_pin_i = 0; ss_pin_i < ss_pins_count; ss_pin_i++) {
        digitalWrite(ss_pins[ss_pin_i], HIGH);
    }
}


uint8_t sendBeacon(int ss_pin, uint8_t length = 0) {
    uint8_t tx_byte __attribute__((aligned(4))) = 0b10000000 | length;
    uint8_t rx_byte __attribute__((aligned(4))) = 0;
    spi_transaction_t t = {};
    t.length = 1 * 8;	// Bytes to bits
    t.tx_buffer = &tx_byte;
    t.rx_buffer = &rx_byte;

    digitalWrite(ss_pin, LOW);
    spi_device_transmit(spi, &t);
    digitalWrite(ss_pin, HIGH);

    return rx_byte;
}

void receivePayload(int ss_pin, uint8_t length = 0) {
	
	if (length > DATA_SIZE) return;
	
    spi_transaction_t t = {};
    t.length = (size_t)length * 8;	// Bytes to bits
    t.tx_buffer = nullptr;
    t.rx_buffer = data_buffer;
    
    digitalWrite(ss_pin, LOW);
    spi_device_transmit(spi, &t);
    digitalWrite(ss_pin, HIGH);
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
    
	// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/spi_master.html

    spi_device_interface_config_t devcfg = {};
    devcfg.clock_speed_hz = 1000000;
    devcfg.mode = 0;
    devcfg.queue_size = 3;
    devcfg.spics_io_num = -1,  // DISABLE hardware CS completely! (Broadcast)
    
    
    spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
    
    for (uint8_t ss_pin_i = 0; ss_pin_i < sizeof(spi_cs_pins)/sizeof(int); ss_pin_i++) {
        pinMode(spi_cs_pins[ss_pin_i], OUTPUT);
    }

    // Initialize pins FIRST before anything else
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); // Start with LED on for signalling MASTER
    
    Serial.println("Master ready");
}


void loop() {
    static uint32_t cycle = 0;
    Serial.printf("\n=== Cycle %lu ===\n", cycle++);
    
    if (random(100) < 10) {
		
        int len = snprintf((char*)data_buffer, 128, "MasterData_%lu:Value=%ld", millis(), random(10000));
        if (len > 127) len = 127;
        
        Serial.printf("\n[From Master] Slave: 0x%02X Beacon=0 L=%d\n", len, len);
        broadcastLength(spi_cs_pins, sizeof(spi_cs_pins)/sizeof(int), (uint8_t)len); // D=0, L=len
        delayMicroseconds(200);
        broadcastPayload(spi_cs_pins, sizeof(spi_cs_pins)/sizeof(int), (uint8_t)len);
        
        Serial.printf("[To Slave] Sent %d bytes\n", len);

    } else {
        
        for (uint8_t ss_pin_i = 0; ss_pin_i < sizeof(spi_cs_pins)/sizeof(int); ss_pin_i++) {

			uint8_t l = sendBeacon(spi_cs_pins[ss_pin_i]);
            Serial.printf("\n[From Beacon to pin %d |1] Slave: 0x%02X Beacon=1 L=%d\n", spi_cs_pins[ss_pin_i], 0b10000000, 0);
            
            if (l > 0) {
                delayMicroseconds(200);
                sendBeacon(spi_cs_pins[ss_pin_i], l);
            	Serial.printf("[From Beacon to pin %d |2] Slave: 0x%02X Beacon=1 L=%d\n", spi_cs_pins[ss_pin_i], 0b10000000 | l, l);
                delayMicroseconds(200);
                receivePayload(spi_cs_pins[ss_pin_i], l);
                Serial.print("[From Slave] Received: ");
                for (int i = 0; i < l; i++) {
                    Serial.print((char)data_buffer[i]);
                }
                Serial.println();
            }
        }
    }
    delay(1000);
}
