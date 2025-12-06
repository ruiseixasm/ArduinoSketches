#include <Arduino.h>
#include <driver/spi_master.h>

#define HSPI_MOSI   13
#define HSPI_SCK    14
#define HSPI_SS     15
#define LED_PIN     2

spi_device_handle_t spi;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  
  // Configure SPI
  spi_bus_config_t buscfg = {};
  buscfg.mosi_io_num = HSPI_MOSI;
  buscfg.miso_io_num = -1;
  buscfg.sclk_io_num = HSPI_SCK;
  buscfg.quadwp_io_num = -1;
  buscfg.quadhd_io_num = -1;
  
  spi_device_interface_config_t devcfg = {};
  devcfg.mode = 0;
  devcfg.clock_speed_hz = 1000000;
  devcfg.spics_io_num = HSPI_SS;
  devcfg.queue_size = 1;
  
  spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
  spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
  
  Serial.println("Master Ready");
}

void loop() {
  static bool led_state = false;
  static uint32_t last_time = 0;
  
  if (millis() - last_time > 1000) {
    last_time = millis();
    led_state = !led_state;
    
    uint8_t data = led_state ? 1 : 0;
    
    spi_transaction_t trans = {};
    trans.length = 8;
    trans.tx_buffer = &data;
    
    spi_device_transmit(spi, &trans);
    
    digitalWrite(LED_PIN, HIGH);
    Serial.print("Sent: ");
    Serial.println(data);
    delay(100);
    digitalWrite(LED_PIN, LOW);
  }
  
  delay(10);
}

