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
#ifndef SPI_SLAVE_CLASS_HPP
#define SPI_SLAVE_CLASS_HPP

#include <Arduino.h>
#include <SPI.h>
#include "driver/spi_slave.h"


#define PIN_MOSI 23
#define PIN_MISO 19
#define PIN_SCLK 18
#define PIN_SS   5


class SPISlave_ESP32 {
public:
    typedef void (*ByteHandler)(uint8_t rx, uint8_t &tx);

private:
    spi_slave_transaction_t trans;
    uint8_t rxByte;
    uint8_t txByte;

    static void spi_callback(spi_slave_transaction_t *t) {
        SPISlave_ESP32 *self = (SPISlave_ESP32*)t->user;

        uint8_t out = 0xFF;
        if (self->byteHandler)
            self->byteHandler(self->rxByte, out);

        self->txByte = out;
    }

    ByteHandler byteHandler = nullptr;

public:
    SPISlave_ESP32() {}

    void onByte(ByteHandler h) {
        byteHandler = h;
    }

    void begin() {
        spi_bus_config_t buscfg = {
            .mosi_io_num = PIN_MOSI,
            .miso_io_num = PIN_MISO,
            .sclk_io_num = PIN_SCLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 1
        };

        spi_slave_interface_config_t slvcfg = {
            .mode = 0,
            .spics_io_num = PIN_SS,
            .queue_size = 1,
            .flags = 0,
            .post_setup_cb = NULL,
            .post_trans_cb = spi_callback
        };

        spi_slave_initialize(VSPI_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);

        memset(&trans, 0, sizeof(trans));
        trans.length = 8;
        trans.rx_buffer = &rxByte;
        trans.tx_buffer = &txByte;
        trans.user = this;

        queue();
    }

    void queue() {
        spi_slave_queue_trans(VSPI_HOST, &trans, portMAX_DELAY);
    }

    void loop() {
        spi_slave_transaction_t *ret_trans;

        if (spi_slave_get_trans_result(VSPI_HOST, &ret_trans, 0) == ESP_OK) {
            queue();  // queue next byte
        }
    }
};


#endif // SPI_SLAVE_CLASS_HPP
