// SPI Slave Code - Pure String Commands
#include <SPI.h>

// Pin definitions
const int LED_PIN = 2;
const int SS_PIN = 10;

#define BUFFER_SIZE 32
char commandBuffer[BUFFER_SIZE];
byte bufferIndex = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize SPI as slave
  SPCR |= _BV(SPE);
  SPCR |= _BV(SPIE);
  SPI.attachInterrupt();
  
  Serial.begin(115200);
  delay(500);
  Serial.println("/n/nSPI Slave Ready - String Mode");
}

// SPI interrupt - receive characters
ISR (SPI_STC_vect) {
  char receivedChar = SPDR;
  
  if (receivedChar == '\0') {
    // End of string - process command
    commandBuffer[bufferIndex] = '\0';
    bufferIndex = 0;
    
    // Process the complete command
    if (strlen(commandBuffer) > 0) {
      processCommand(commandBuffer);
    }
  } else if (bufferIndex < BUFFER_SIZE - 1) {
    // Store character in buffer
    commandBuffer[bufferIndex++] = receivedChar;
  }
}

void processCommand(const char* command) {
  Serial.print("Received: ");
  Serial.println(command);
  
  if (strcmp(command, "LED_ON") == 0) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("LED turned ON");
  }
  else if (strcmp(command, "LED_OFF") == 0) {
    digitalWrite(LED_PIN, LOW);
    Serial.println("LED turned OFF");
  }
  else {
    Serial.println("\t\t\t\t\t\tUnknown command");
  }
}

void loop() {
  // Nothing needed here - everything happens in ISR
}

