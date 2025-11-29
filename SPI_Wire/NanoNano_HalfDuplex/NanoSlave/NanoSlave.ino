// SPI Slave Code - Pure String Commands
#include <SPI.h>


enum MessageCode : uint8_t {
    START   = 0xF0, // Start of transmission
    END     = 0xF1, // End of transmission
    ACK     = 0xF2, // Acknowledge
    NACK    = 0xF3, // Not acknowledged
    READY   = 0xF4, // Slave has response ready
    ERROR   = 0xF5, // Error frame
    RECEIVE = 0xF7, // Asks the receiver to start receiving
    SEND    = 0xF6  // Asks the receiver to start sending
};


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
// 0/0; 0/1; 1/0; 1/1 combinations (4)
//     0/0: Received, to be processed;
//     0/1: Sending, can't receive wile sending
//     1/0: Receiving, wont process anything until concludes reception


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

    // One is always able to receive (receiving buffer always available)
    if (receiving_index < BUFFER_SIZE) {
        if (c == 0xFF) {    // Sending call
            Serial.println("1b: Sending call");
            receiving_state = false;    // End of receiving
        } else {
            receiving_buffer[receiving_index++] = c;
            if (c == '\0') {
                Serial.print("1a: Receiving buffer: ");
                Serial.println(receiving_buffer);
                receiving_index = 0;    // In order to be received again
            }
        }
    } else {    // overflow
        receiving_index = 0;
        receiving_buffer[receiving_index] = '/0';   // Makes sure receiving Buffer is clean
    }
    SPDR = '/0';    // Always empty the sending buffer

    // At this tage the receiving buffer can be used again
    if (sending_state) {    // THE SLAVE HAS THE OPPORTUNITY TO SEND SOMETHING (FULL DUPLEX)
        c = sending_buffer[sending_index++];
        if (c == '\0') {
            sending_state = false;
            receiving_state = true;
            Serial.println(c);
        } else {
            Serial.print(c);
        }
        SPDR = c;
    } else if (!receiving_state) {  // Has something in the receiving Buffer
        Serial.println("2: Processing commands!");
        processCommand();   // The ONLY one that writes on sending Buffer
        sending_state = true;
        sending_index = 0;
    } else {    
        Serial.println("0: Nothing to be SENT!");
    }   // else: Nothing to be sent
}


void processCommand() {

    Serial.print("3: Processed command: ");
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
}

void loop() {
    // Nothing here – all work done inside ISR
}

