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

uint8_t rx_buffer[DATA_SIZE] __attribute__((aligned(4)));
uint8_t tx_buffer[DATA_SIZE] __attribute__((aligned(4)));
uint8_t cmd_byte __attribute__((aligned(4)));
uint8_t sending_length __attribute__((aligned(4))) = 0;

spi_slave_transaction_t cmd_trans;
spi_slave_transaction_t data_trans;

enum SpiState {
    WAIT_CMD,
    RX_PAYLOAD,
    TX_PAYLOAD
};

SpiState spi_state = WAIT_CMD;

void queue_cmd_transaction() {
    memset(&cmd_trans, 0, sizeof(cmd_trans));
    cmd_trans.length    = 8;
    cmd_trans.rx_buffer = &cmd_byte;
    cmd_trans.tx_buffer = &sending_length;   // always report availability
    spi_slave_queue_trans(VSPI_HOST, &cmd_trans, portMAX_DELAY);
}

void queue_rx_transaction(uint8_t len) {
    memset(&data_trans, 0, sizeof(data_trans));
    data_trans.length    = len * 8;
    data_trans.rx_buffer = rx_buffer;
    spi_slave_queue_trans(VSPI_HOST, &data_trans, portMAX_DELAY);
}

void queue_tx_transaction(uint8_t len) {
    memset(&data_trans, 0, sizeof(data_trans));
    data_trans.length    = len * 8;
    data_trans.tx_buffer = tx_buffer;
    spi_slave_queue_trans(VSPI_HOST, &data_trans, portMAX_DELAY);
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

    queue_cmd_transaction();   // ARM IMMEDIATELY
}

void loop() {
    spi_slave_transaction_t *ret;

    /* --- generate data opportunistically --- */
    if (sending_length == 0 && random(100) < 10) {
        int len = snprintf((char*)tx_buffer, DATA_SIZE,
                           "SlaveData_%lu", millis());
        if (len > 127) len = 127;
        sending_length = (uint8_t)len;
    }

    /* --- check for completed SPI transaction --- */
    if (spi_slave_get_trans_result(VSPI_HOST, &ret, 0) != ESP_OK) {
        return; // nothing finished yet
    }

    /* --- "ISR" logic --- */
    if (spi_state == WAIT_CMD) {
        bool beacon = (cmd_byte & 0x80);
        uint8_t received_length = cmd_byte & 0x7F;

        Serial.printf("\n[CMD] 0x%02X beacon=%d len=%u\n",
                      cmd_byte, beacon, received_length);

        if (!beacon) {
            if (received_length > 0 && received_length <= DATA_SIZE) {
                spi_state = RX_PAYLOAD;
                queue_rx_transaction(received_length);
                return;
            } else {
                Serial.println("Master ping");
            }
        } else {
            if (received_length > 0) {
                spi_state = TX_PAYLOAD;
                queue_tx_transaction(received_length);
                return;
            } else {
                Serial.println("Beacon, no data requested");
            }
        }

        queue_cmd_transaction();
    }

    else if (spi_state == RX_PAYLOAD) {
        Serial.print("Received: ");
        for (int i = 0; i < (cmd_byte & 0x7F); i++) {
            char c = rx_buffer[i];
            if (c >= 32 && c <= 126) Serial.print(c);
            else Serial.printf("[%02X]", c);
        }
        Serial.println();

        spi_state = WAIT_CMD;
        queue_cmd_transaction();
    }

    else if (spi_state == TX_PAYLOAD) {
        Serial.printf("Sent %u bytes\n", sending_length);
        sending_length = 0;

        spi_state = WAIT_CMD;
        queue_cmd_transaction();
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

