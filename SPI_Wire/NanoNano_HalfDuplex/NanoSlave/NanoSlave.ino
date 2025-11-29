// SPI Slave Code - Pure String Commands
#include <SPI.h>


enum MessageCode : uint8_t {
    START   = 0xF0, // Start of transmission
    END     = 0xF1, // End of transmission
    ACK     = 0xF2, // Acknowledge
    NACK    = 0xF3, // Not acknowledged
    READY   = 0xF4, // Slave has response ready
    ERROR   = 0xF5, // Error frame
    RECEIVE = 0xF6, // Asks the receiver to start receiving
    SEND    = 0xF7, // Asks the receiver to start sending
    NONE    = 0xF8  // Means nothing to send
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
volatile bool process_message = false;


void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\nSPI Slave Ready - Half-Duplex Mode");

    Serial.print("LED_PIN: ");
    Serial.println(LED_PIN);

    pinMode(MISO, OUTPUT);  // MISO must be OUTPUT for Slave to send data!

    // Initialize SPI as slave - EXPLICIT MSB FIRST
    SPCR = 0;  // Clear register
    SPCR |= _BV(SPE);    // SPI Enable
    SPCR |= _BV(SPIE);   // SPI Interrupt Enable  
    SPCR &= ~_BV(DORD);  // MSB First (DORD=0 for MSB first)
    SPCR &= ~_BV(CPOL);  // Clock polarity 0
    SPCR &= ~_BV(CPHA);  // Clock phase 0 (MODE0)

    // // Initialize SPI as slave
    // SPCR |= _BV(SPE);
    // SPCR |= _BV(SPIE);

    SPI.attachInterrupt();  
}



// ------------------------------
// SPI Interrupt
// ------------------------------
ISR(SPI_STC_vect) {

    // AVOID PLACING HEAVY CODE OR CALL HERE. THIS INTERRUPTS THE LOOP!

    // WARNING:
    //     AVOID PLACING Serial.print CALLS HERE BECAUSE IT WILL DELAY 
    //     THE POSSIBILITY OF SPI CAPTURE AND RESPONSE IN TIME !!!

    uint8_t c = SPDR;    // Avoid using 'char' while using values above 127
    uint8_t last_message = SEND;


    if (c == RECEIVE) {
        receiving_state = true;
        receiving_index = 0;
    } else if (c == END) {
        receiving_state = false;
        if (receiving_index > 0) {
            process_message = true;
        }
    } else if (receiving_state) {
        if (receiving_index < BUFFER_SIZE) {
            receiving_buffer[receiving_index++] = c;
        } else {
            receiving_state = false;
        }
    } else if (c == SEND) {
        sending_index = 0;
        last_message = sending_buffer[sending_index++];
        if (last_message == '\0') {
            SPDR = NONE;    // Nothing to send
        } else {
            SPDR = last_message;
            process_message = true;
        }
    } else if (sending_state) {
        if (c != last_message) {
            SPDR = ERROR;
            process_message = false;
        }
        last_message = sending_buffer[sending_index++];
        if (last_message == '\0') {
            SPDR = END;     // Nothing more to send (spares extra send, '\0' implicit)
        } else {
            SPDR = last_message;
        }
    }
}


void processMessage() {

    Serial.print("Processed command: ");
    Serial.println(receiving_buffer);


    if (strcmp(receiving_buffer, "LED_ON") == 0) {
        digitalWrite(LED_PIN, HIGH);
        strcpy(sending_buffer, "OK_ON");
        Serial.print("LED is ON");
        Serial.print(" | Sending: ");
        Serial.println(sending_buffer);
    } else if (strcmp(receiving_buffer, "LED_OFF") == 0) {
        digitalWrite(LED_PIN, LOW);
        strcpy(sending_buffer, "OK_OFF");
        Serial.print("LED is OFF");
        Serial.print(" | Sending: ");
        Serial.println(sending_buffer);
    } else {
        strcpy(sending_buffer, "BUZZ");
        Serial.print("Unknown command");
        Serial.print(" | Sending: ");
        Serial.println(sending_buffer);
    }
}

void loop() {
    // HEAVY PROCESSING SHALL BE IN THE LOOP
    if (process_message) {
        processMessage();   // Called only once!
        process_message = false;    // Critical to avoid repeated calls over the ISR function
    }
}

