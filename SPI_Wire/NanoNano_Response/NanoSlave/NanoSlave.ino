// SPI Slave Code - Pure String Commands
#include <SPI.h>

// Pin definitions
const int LED_PIN = 2;
const int SS_PIN = 10;

#define BUFFER_SIZE 32
char commandBuffer[BUFFER_SIZE];
byte bufferIndex = 0;

// Response handling
char responseBuffer[BUFFER_SIZE];
bool responseReady = false;
byte responseIndex = 0;
bool isReceivingCommand = true;  // Track if we're receiving command or sending response

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize SPI as slave
  SPCR |= _BV(SPE);
  SPCR |= _BV(SPIE);
  SPI.attachInterrupt();
  
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\nSPI Slave Ready - String Mode");
}


// SPI interrupt - receive characters
ISR (SPI_STC_vect) {
  char receivedChar = SPDR;
  
  if (isReceivingCommand) {
    // We're receiving a command from Master
    if (receivedChar == '\0') {
      // End of command - process it
      commandBuffer[bufferIndex] = '\0';
      bufferIndex = 0;
      isReceivingCommand = false;
      
      // Process command and prepare response
      if (strlen(commandBuffer) > 0) {
        processCommand(commandBuffer);
      }
      
      // Send first character of response
      if (responseReady && responseBuffer[0] != '\0') {
        SPDR = responseBuffer[0];
        responseIndex = 1;
      } else {
        SPDR = '\0';
      }
    } else if (bufferIndex < BUFFER_SIZE - 1) {
      // Store character in buffer
      commandBuffer[bufferIndex++] = receivedChar;
      SPDR = 0; // Send 0 while receiving command
    } else {
      // Buffer full - reset
      bufferIndex = 0;
      SPDR = 0;
    }
  } else {
    
    // We're sending response to Master
    if (responseReady && responseIndex < strlen(responseBuffer)) {
        SPDR = responseBuffer[responseIndex++];
    } else {
        // End of response - reset everything for next command
        SPDR = '\0';
        responseReady = false;
        responseIndex = 0;
        isReceivingCommand = true;
        bufferIndex = 0;  // ADD THIS: Reset command buffer
    }
  }
}


void processCommand(const char* command) {
  Serial.print("Received: ");
  Serial.println(command);
  
  if (strcmp(command, "LED_ON") == 0) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("LED turned ON");
    strcpy(responseBuffer, "OK_ON");
    responseReady = true;
    responseIndex = 0;
  }
  else if (strcmp(command, "LED_OFF") == 0) {
    digitalWrite(LED_PIN, LOW);
    Serial.println("LED turned OFF");
    strcpy(responseBuffer, "OK_OFF");
    responseReady = true;
    responseIndex = 0;
  }
  else {
    Serial.println("Unknown command");
    strcpy(responseBuffer, "BUZZ");
    responseReady = true;
    responseIndex = 0;
  }
}

void loop() {
  // Nothing needed here - everything happens in ISR
}

