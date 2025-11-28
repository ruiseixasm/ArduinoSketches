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

volatile bool receiving_state = true;
volatile bool sending_state = false;


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
    if (receiving_state) {

        if (receiving_index < BUFFER_SIZE) {
            receiving_buffer[receiving_index++] = c;
            if (c == '\0') {
                receiving_state = false;

                // Prepare complete response BEFORE sending anything
                processCommand();

                // When master clocks next byte, we start sending
                SPDR = sending_state ? sending_buffer[sending_index++] : '\0';

            } else {
                SPDR = '\0'; // nothing to send yet
            }

        } else {
            // overflow
            receiving_index = 0;
            SPDR = '\0';
        }

    } else if (sending_state) {
        SPDR = sending_buffer[sending_index++];
        if (SPDR == '\0') {
            sending_state = false;
        }
    } else {
        
        // End of response
        SPDR = '\0';
        receiving_state = true;
        receiving_index = 0;
        sending_index = 0;
    }
}


void processCommand() {

    Serial.print("Received: ");
    Serial.println(receiving_buffer);

    if (strcmp(receiving_buffer, "LED_ON") == 0) {
        strcpy(sending_buffer, "OK_ON");
        digitalWrite(LED_PIN, HIGH);
        Serial.print("LED is ON");
        Serial.print(" | Sending: ");
        Serial.println(sending_buffer);
    }
    else if (strcmp(receiving_buffer, "LED_OFF") == 0) {
        strcpy(sending_buffer, "OK_OFF");
        digitalWrite(LED_PIN, LOW);
        Serial.print("LED is OFF");
        Serial.print(" | Sending: ");
        Serial.println(sending_buffer);
    }
    else {
        strcpy(sending_buffer, "BUZZ");
        Serial.print("Unknown command");
        Serial.print(" | Sending: ");
        Serial.println(sending_buffer);
    }

    sending_state = true;
}

void loop() {
    // Nothing here – all work done inside ISR
}

