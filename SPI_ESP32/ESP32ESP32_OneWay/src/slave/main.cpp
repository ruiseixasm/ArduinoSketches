#include <Arduino.h>
#include <driver/spi_slave.h>

#define VSPI_MOSI   23
#define VSPI_MISO   19
#define VSPI_SCK    18
#define VSPI_SS      5
#define LED_PIN      2

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  spi_bus_config_t buscfg = {};
  buscfg.mosi_io_num = VSPI_MOSI;
  buscfg.miso_io_num = -1; // not used
  buscfg.sclk_io_num = VSPI_SCK;
  buscfg.quadwp_io_num = -1;
  buscfg.quadhd_io_num = -1;

  spi_slave_interface_config_t slvcfg = {};
  slvcfg.spics_io_num = VSPI_SS;
  slvcfg.mode = 0;
  slvcfg.queue_size = 1;
  slvcfg.flags = 0;

  if (spi_slave_initialize(VSPI_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO) != ESP_OK) {
    Serial.println("SPI Slave init failed");
  } else {
    Serial.println("Slave Ready");
  }
}

void loop() {
  uint8_t rx_data = 0;

  spi_slave_transaction_t trans = {};
  trans.length = 8;          // 8 bits = 1 byte
  trans.rx_buffer = &rx_data; // buffer to receive into

  // Wait for master to send
  esp_err_t ret = spi_slave_transmit(VSPI_HOST, &trans, portMAX_DELAY);
  if (ret == ESP_OK) {
    digitalWrite(LED_PIN, rx_data ? HIGH : LOW);
    Serial.print("LED ");
    Serial.println(rx_data ? "ON" : "OFF");
  }
}

