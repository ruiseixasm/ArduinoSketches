// SPI Slave Code - Pure String Commands
#include <SPI.h>


enum MessageCode : uint8_t {
    START   = 0xF0, // Start of transmission
    END     = 0xF1, // End of transmission
    ACK     = 0xF2, // Acknowledge
    NACK    = 0xF3, // Not acknowledged
    READY   = 0xF4, // Slave has response ready
    ERROR   = 0xF5  // Error frame
};


// Pin definitions
const int LED_PIN = 2;
const int SS_PIN = 10;

#define BUFFER_SIZE 128
char receiving_buffer[BUFFER_SIZE];
volatile byte receiving_index = 0;
volatile bool receiving_state = false;


void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize SPI as slave
  SPCR |= _BV(SPE);
  SPCR |= _BV(SPIE);
  SPI.attachInterrupt();
  
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\nSPI Slave Ready - One Way Communication");
}


// ------------------------------
// SPI Interrupt
// ------------------------------
ISR(SPI_STC_vect) {

    // char c = SPDR;    // DON'T USE char BECAUSE BECOMES SIGNED!!
    uint8_t c = SPDR;    // The most important line!!

    Serial.print("0. Slave received: 0x");
    Serial.println(c, HEX);  // Debug what byte actually arrives
    
    if (c == START) {
        Serial.println("1. Start receiving");
        receiving_state = true;
        receiving_index = 0;
        SPDR = ACK;  // Send acknowledgment back to Master
    } else if (c == END) {
        Serial.println("2. End receiving");
        receiving_state = false;
        if (receiving_index > 0) {
            processCommand();
        }
        SPDR = ACK;  // Send acknowledgment back to Master
    } else if (receiving_state) {

        if (receiving_index < BUFFER_SIZE) {
            receiving_buffer[receiving_index++] = c;
        } else {
            receiving_state = false;
            receiving_index = 0;
            SPDR = ERROR;  // Buffer overflow
        }
    } else {
        SPDR = NACK;  // Not in receiving state
    }
}


void processCommand() {

    Serial.print("3. Processed command: ");
    Serial.println(receiving_buffer);

    if (strcmp(receiving_buffer, "LED_ON") == 0) {
        digitalWrite(LED_PIN, HIGH);
        Serial.print("LED is ON");
    }
    else if (strcmp(receiving_buffer, "LED_OFF") == 0) {
        digitalWrite(LED_PIN, LOW);
        Serial.print("LED is OFF");
    }
    else {
        Serial.println("Unknown command");
    }
}

void loop() {
    // Nothing here – all work done inside ISR
}

