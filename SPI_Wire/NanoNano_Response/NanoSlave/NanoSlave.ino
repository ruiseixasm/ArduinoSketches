// SPI Slave Code - Pure String Commands
#include <SPI.h>

// Pin definitions
const int LED_PIN = 2;
const int SS_PIN = 10;

#define BUFFER_SIZE 128
char receiving_buffer[BUFFER_SIZE];
char sending_buffer[BUFFER_SIZE];

volatile byte receiving_index = 0;
volatile byte sending_index = 0;

volatile bool receiving_complete = false;
volatile bool sending_ready = false;


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


// ------------------------------
// SPI Interrupt
// ------------------------------
ISR(SPI_STC_vect) {
  char c = SPDR;    // The most important line!!

  // If we are still receiving a command
  if (!receiving_complete) {

    if (receiving_index < BUFFER_SIZE - 1) {
      receiving_buffer[receiving_index++] = c;
      if (c == '\0') {
        receiving_index = 0;
        receiving_complete = true;

        // Prepare complete response BEFORE sending anything
        processCommand();

        // When master clocks next byte, we start sending
        sending_index = 0;
        SPDR = sending_ready ? sending_buffer[sending_index++] : '\0';

      } else {
          SPDR = '\0'; // nothing to send yet
      }

    } else {
      // overflow
      receiving_index = 0;
      SPDR = '\0';
    }

  } else {
    // Send phase
    if (sending_ready) {
      SPDR = sending_buffer[sending_index++];
      if (SPDR == '\0') {
        sending_ready = false;
        receiving_complete = false;
        sending_index = 0;
      }
    } else {
      // End of response
      SPDR = '\0';
      receiving_complete = false;
      sending_index = 0;
    }
  }
}


void processCommand() {

  Serial.print("Received: ");
  Serial.println(receiving_buffer);

  if (strcmp(receiving_buffer, "LED_ON") == 0) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("LED is ON");
    strcpy(sending_buffer, "OK_ON");
  }
  else if (strcmp(receiving_buffer, "LED_OFF") == 0) {
    digitalWrite(LED_PIN, LOW);
    Serial.println("LED is OFF");
    strcpy(sending_buffer, "OK_OFF");
  }
  else {
    Serial.println("Unknown command");
    strcpy(sending_buffer, "BUZZ");
  }

  sending_ready = true;
}

void loop() {
  // Nothing here – all work done inside ISR
}

