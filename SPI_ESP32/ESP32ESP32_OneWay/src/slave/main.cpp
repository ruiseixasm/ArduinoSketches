#include <Arduino.h>
#include <soc/spi_reg.h>
#include <soc/spi_struct.h>
#include <driver/gpio.h>

// VSPI pins for ESP32 Slave
#define VSPI_MOSI   23  // Data from master
#define VSPI_SCK    18  // Clock from master
#define VSPI_SS      5  // Slave Select from master

// LED to control
#define LED_PIN 2

// Simple bit-banged SPI slave receiver
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("ESP32 Slave - Simple SPI Receiver");
  
  // Setup LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Configure SPI pins
  pinMode(VSPI_SS, INPUT_PULLUP);
  pinMode(VSPI_SCK, INPUT);
  pinMode(VSPI_MOSI, INPUT);
  
  Serial.println("Slave ready! Waiting for commands...");
}

uint8_t readSPIByte() {
  uint8_t byteReceived = 0;
  
  // Wait for SS to go low (master selects us)
  while(digitalRead(VSPI_SS) == HIGH) {
    delayMicroseconds(1);
  }
  
  // Read 8 bits on SCK rising edges
  for(int i = 7; i >= 0; i--) {
    // Wait for clock low
    while(digitalRead(VSPI_SCK) == HIGH) {
      delayMicroseconds(1);
    }
    
    // Wait for clock high (read on rising edge)
    while(digitalRead(VSPI_SCK) == LOW) {
      delayMicroseconds(1);
    }
    
    // Read MOSI bit
    if(digitalRead(VSPI_MOSI) == HIGH) {
      byteReceived |= (1 << i);
    }
  }
  
  // Wait for SS to go high (master deselects us)
  while(digitalRead(VSPI_SS) == LOW) {
    delayMicroseconds(1);
  }
  
  return byteReceived;
}

void loop() {
  // Check if master is selecting us
  if(digitalRead(VSPI_SS) == LOW) {
    // Read the byte
    uint8_t command = readSPIByte();
    
    // Process command
    if(command == 1) {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("Received: LED ON (1)");
    } 
    else if(command == 0) {
      digitalWrite(LED_PIN, LOW);
      Serial.println("Received: LED OFF (0)");
    }
    else {
      Serial.print("Unknown command: ");
      Serial.println(command);
    }
  }
  
  delay(10);
}

