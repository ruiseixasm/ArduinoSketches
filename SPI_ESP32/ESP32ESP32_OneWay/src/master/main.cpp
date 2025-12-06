#include <Arduino.h>
#include <driver/spi_master.h>

// HSPI pins for ESP32 Master
#define HSPI_MOSI   13  // Data to slave
#define HSPI_SCK    14  // Clock
#define HSPI_SS     15  // Slave Select

// Built-in LED for feedback
#define LED_PIN 2

spi_device_handle_t spi;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("ESP32 Master - Simple SPI LED Control");
  
  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Configure SS pin
  pinMode(HSPI_SS, OUTPUT);
  digitalWrite(HSPI_SS, HIGH);
  
  // ESP32 SPI Master configuration
  spi_bus_config_t buscfg = {
    .mosi_io_num = HSPI_MOSI,
    .miso_io_num = -1,  // Not used for one-way
    .sclk_io_num = HSPI_SCK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 0
  };
  
  spi_device_interface_config_t devcfg = {
    .mode = 0,  // SPI mode 0
    .clock_speed_hz = 1000000,  // 1 MHz
    .spics_io_num = HSPI_SS,
    .flags = 0,
    .queue_size = 1
  };
  
  // Initialize SPI bus and device
  spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
  spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
  
  Serial.println("Master ready!");
}

void sendCommand(uint8_t command) {
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  
  t.length = 8;  // 8 bits
  t.tx_buffer = &command;
  
  // Select slave and send command
  spi_device_transmit(spi, &t);
}

void loop() {  
  // Blink LED to show it's alive
  static unsigned long lastBlink = 0;
  static int command = 0;
  if (millis() - lastBlink > 1000) {
	digitalWrite(LED_PIN, HIGH);
    lastBlink = millis();
	command = 1 - command;
	if (command == 1) {
		Serial.println("Sending: LED ON (1)");
      	sendCommand(1);
	} else {
		Serial.println("Sending: LED OFF (0)");
      	sendCommand(0);
	}
	delay(200);
	digitalWrite(LED_PIN, LOW);
  }
  
  delay(10);
}

