// SPI Slave Code
#include <SPI.h>

// Define commands
#define LED_ON  0x01
#define LED_OFF 0x00

// Pin definitions
const int LED_PIN = 2;     // External LED pin
const int SS_PIN = 10;     // Slave Select pin

// Response codes
#define RESPONSE_ACK 0xAA
#define RESPONSE_NAK 0x55

volatile byte receivedCommand = 0;
volatile bool commandReceived = false;

void setup() {
  // Initialize pins
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize SPI as slave
  SPCR |= _BV(SPE);        // Enable SPI
  SPCR |= _BV(SPIE);       // Enable SPI interrupt
  
  // Initialize serial for debugging
  Serial.begin(115200);
  delay(500);   // Give some time to serial port start up
  Serial.println("\n\n\nSPI Slave Initialized - Waiting for commands...");
  
  // Turn on SPI interrupt
  SPI.attachInterrupt();
}

// SPI interrupt routine
ISR (SPI_STC_vect) {
  receivedCommand = SPDR;  // Read received byte
  commandReceived = true;
}

void loop() {
  if (commandReceived) {
    processCommand(receivedCommand);
    commandReceived = false;
  }
}

void processCommand(byte command) {
  switch (command) {
    case LED_ON:
      digitalWrite(LED_PIN, HIGH);
      Serial.println("Received: LED ON");
      SPDR = RESPONSE_ACK;  // Send acknowledgment
      break;
      
    case LED_OFF:
      digitalWrite(LED_PIN, LOW);
      Serial.println("Received: LED OFF");
      SPDR = RESPONSE_ACK;  // Send acknowledgment
      break;
      
    default:
      Serial.println("Received: Unknown command");
      SPDR = RESPONSE_NAK;  // Send negative acknowledgment
      break;
  }
}
