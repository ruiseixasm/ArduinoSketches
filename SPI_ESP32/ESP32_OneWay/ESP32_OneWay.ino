#include <Arduino.h>

extern "C" {
  #include "driver/spi_master.h"
  #include "driver/spi_slave.h"
  #include "driver/gpio.h"
  #include "esp_err.h"
}

// MASTER pins (HSPI)
#define HSPI_MOSI 13
#define HSPI_SCLK 14
#define HSPI_SS   15

// SLAVE pins (VSPI)
#define VSPI_MOSI 23
#define VSPI_SCLK 18
#define VSPI_SS    5

#define LED_PIN    2

uint8_t slave_rx[1];
uint8_t slave_tx[1] = {0};

spi_device_handle_t hspi = NULL;

void setup_spi_slave() {
  Serial.println("Init SPI SLAVE (VSPI) ...");

  spi_bus_config_t buscfg;
  memset(&buscfg, 0, sizeof(buscfg));
  buscfg.mosi_io_num = VSPI_MOSI;
  buscfg.miso_io_num = -1;
  buscfg.sclk_io_num = VSPI_SCLK;
  buscfg.quadwp_io_num = -1;
  buscfg.quadhd_io_num = -1;
  buscfg.max_transfer_sz = 0;

  spi_slave_interface_config_t slvcfg;
  memset(&slvcfg, 0, sizeof(slvcfg));
  slvcfg.mode = 0;
  slvcfg.spics_io_num = VSPI_SS;
  slvcfg.queue_size = 3;
  slvcfg.flags = 0;
  slvcfg.post_setup_cb = NULL;
  slvcfg.post_trans_cb = NULL;

  esp_err_t ret = spi_slave_initialize(VSPI_HOST, &buscfg, &slvcfg, 0);
  if (ret != ESP_OK) {
    Serial.print("spi_slave_initialize failed: ");
    Serial.println(ret);
  } else {
    Serial.println("SPI SLAVE initialized.");
  }
}

void setup_spi_master() {
  Serial.println("Init SPI MASTER (HSPI) ...");

  spi_bus_config_t buscfg;
  memset(&buscfg, 0, sizeof(buscfg));
  buscfg.mosi_io_num = HSPI_MOSI;
  buscfg.miso_io_num = -1;
  buscfg.sclk_io_num = HSPI_SCLK;
  buscfg.quadwp_io_num = -1;
  buscfg.quadhd_io_num = -1;
  buscfg.max_transfer_sz = 0;

  spi_device_interface_config_t devcfg;
  memset(&devcfg, 0, sizeof(devcfg));
  devcfg.command_bits = 0;
  devcfg.address_bits = 0;
  devcfg.dummy_bits = 0;
  devcfg.mode = 0;
  devcfg.duty_cycle_pos = 0;
  devcfg.cs_ena_posttrans = 0;
  devcfg.cs_ena_pretrans = 0;
  devcfg.clock_speed_hz = 1 * 1000 * 1000;
  devcfg.spics_io_num = HSPI_SS;
  devcfg.queue_size = 3;
  devcfg.flags = 0;
  devcfg.pre_cb = NULL;
  devcfg.post_cb = NULL;

  esp_err_t ret = spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
  if (ret != ESP_OK) {
    Serial.print("spi_bus_initialize(HSPI) failed: ");
    Serial.println(ret);
    return;
  }

  ret = spi_bus_add_device(HSPI_HOST, &devcfg, &hspi);
  if (ret != ESP_OK) {
    Serial.print("spi_bus_add_device failed: ");
    Serial.println(ret);
  } else {
    Serial.println("SPI MASTER initialized.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  setup_spi_slave();
  setup_spi_master();

  Serial.println("READY. Connect HSPI->VSPI jumpers and open serial monitor.");
}

// Helper: master sends a single byte
void master_send_byte(uint8_t v) {
  if (!hspi) {
    Serial.println("Master device handle NULL!");
    return;
  }
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  t.length = 8;
  t.tx_buffer = &v;
  esp_err_t ret = spi_device_transmit(hspi, &t);
  if (ret != ESP_OK) {
    Serial.print("spi_device_transmit failed: ");
    Serial.println(ret);
  }
}

// Helper: slave blocks waiting for 1 byte
bool slave_wait_and_handle() {
  spi_slave_transaction_t t;
  memset(&t, 0, sizeof(t));
  t.length = 8;
  t.tx_buffer = slave_tx;
  t.rx_buffer = slave_rx;
  esp_err_t ret = spi_slave_transmit(VSPI_HOST, &t, 0); // 0 -> non-blocking wait; try  portMAX_DELAY for blocking
  if (ret == ESP_OK) {
    uint8_t cmd = slave_rx[0];
    if (cmd == 0x01) digitalWrite(LED_PIN, HIGH);
    else digitalWrite(LED_PIN, LOW);
    Serial.print("SLAVE recv: 0x");
    Serial.println(cmd, HEX);
    return true;
  } else if (ret == ESP_ERR_TIMEOUT) {
    // no data (non-blocking)
    return false;
  } else {
    // other error
    // note: some cores may not accept 0 timeout; if you see errors, switch to portMAX_DELAY
    // Serial.print("spi_slave_transmit err: "); Serial.println(ret);
    return false;
  }
}

unsigned long last_master = 0;
uint8_t next_value = 1;

void loop() {
  unsigned long now = millis();

  // Master sends every 2s
  if (now - last_master >= 2000) {
    master_send_byte(next_value);
    Serial.print("MASTER sent: 0x"); Serial.println(next_value, HEX);
    next_value ^= 1;
    last_master = now;
  }

  // Try to read slave (non-blocking)
  slave_wait_and_handle();

  delay(5); // small yield to avoid busy-loop; adjust if needed
}

