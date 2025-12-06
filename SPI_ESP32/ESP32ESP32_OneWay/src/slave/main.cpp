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
  
  // Configure SPI slave - using proper struct initialization
  spi_bus_config_t buscfg;
  buscfg.mosi_io_num = VSPI_MOSI;
  buscfg.miso_io_num = VSPI_MISO;
  buscfg.sclk_io_num = VSPI_SCK;
  buscfg.quadwp_io_num = -1;
  buscfg.quadhd_io_num = -1;
  
  spi_slave_interface_config_t slvcfg;
  slvcfg.mode = 0;
  slvcfg.spics_io_num = VSPI_SS;
  slvcfg.queue_size = 1;
  slvcfg.flags = 0;
  
  spi_slave_initialize(VSPI_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
  
  Serial.println("Slave Ready");
}

void loop() {
  uint8_t rx_data[1] = {0};
  
  spi_slave_transaction_t trans;
  memset(&trans, 0, sizeof(trans));
  trans.length = 8 * sizeof(rx_data);
  trans.rx_buffer = rx_data;
  
  // Wait for data from master
  if (spi_slave_transmit(VSPI_HOST, &trans, portMAX_DELAY) == ESP_OK) {
    if (rx_data[0] == 1) {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED ON");
    } else if (rx_data[0] == 0) {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED OFF");
    }
  }
}

