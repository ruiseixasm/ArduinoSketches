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

#define FRAME_SIZE 129
#define MAX_PAYLOAD 128

#define HSPI_CS 5

spi_device_handle_t spi;
uint8_t tx_frame[FRAME_SIZE];
uint8_t rx_frame[FRAME_SIZE];

void setup() {
  Serial.begin(115200);

  // SPI bus configuration
  spi_bus_config_t buscfg = {};
  buscfg.mosi_io_num = 23;
  buscfg.miso_io_num = 19;
  buscfg.sclk_io_num = 18;
  buscfg.quadwp_io_num = -1;
  buscfg.quadhd_io_num = -1;
  buscfg.max_transfer_sz = FRAME_SIZE;

  // SPI device configuration
  spi_device_interface_config_t devcfg = {};
  devcfg.clock_speed_hz = 1 * 1000 * 1000; // 1 MHz
  devcfg.mode = 0;
  devcfg.spics_io_num = HSPI_CS;
  devcfg.queue_size = 1;

  spi_bus_initialize(HSPI_HOST, &buscfg, 1);
  spi_bus_add_device(HSPI_HOST, &devcfg, &spi);

  Serial.println("MASTER ready");
}

void sendFrame(const char *msg) {
  uint8_t len = strlen(msg);
  if (len > MAX_PAYLOAD) len = MAX_PAYLOAD;

  tx_frame[0] = len;
  memset(&tx_frame[1], 0, MAX_PAYLOAD);
  memcpy(&tx_frame[1], msg, len);

  // MASTER -> SLAVE broadcast
  spi_transaction_t t = {};
  t.length = FRAME_SIZE * 8;
  t.tx_buffer = tx_frame;
  t.rx_buffer = nullptr; // ignore slave during broadcast
  spi_device_transmit(spi, &t);

  Serial.print("MASTER sent: ");
  Serial.write(&tx_frame[1], len);
  Serial.println();
}

uint8_t pollSlave(char *out) {
  // length=0 signals slave we are in receive mode
  tx_frame[0] = 0;
  memset(&tx_frame[1], 0, MAX_PAYLOAD);

  spi_transaction_t t = {};
  t.length = FRAME_SIZE * 8;
  t.tx_buffer = tx_frame;
  t.rx_buffer = rx_frame;
  spi_device_transmit(spi, &t);

  uint8_t len = rx_frame[0];
  if (len > MAX_PAYLOAD) len = 0;
  memcpy(out, &rx_frame[1], len);
  return len;
}

void loop() {
  // Step 1: Broadcast message to slave
  sendFrame("Hello from MASTER");

  delay(10); // give slave time to prepare reply

  // Step 2: Poll slave
  char buffer[MAX_PAYLOAD];
  uint8_t len = pollSlave(buffer);

  if (len) {
    Serial.print("MASTER received: ");
    Serial.write(buffer, len);
    Serial.println();
  }

  delay(2000);
}
