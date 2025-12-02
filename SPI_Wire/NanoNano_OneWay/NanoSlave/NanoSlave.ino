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
    NONE    = 0xF8, // Means nothing to send
    FULL    = 0xF9, // Signals the buffer as full
    
    VOID    = 0xFF  // MISO floating (0xFF) → no slave responding
}


// Pin definitions
const int LED_PIN = 2;
const int SS_PIN = 10;

#define BUFFER_SIZE 128
char receiving_buffer[BUFFER_SIZE];
volatile byte receiving_index = 0;
volatile bool receiving_state = false;

// For loop flags
volatile bool process_command = false;


void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\nSPI Slave Ready - One Way Communication");

    
    Serial.print("LED_PIN: ");
    Serial.println(LED_PIN);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

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

    // char is signed by default on most Arduino platforms (-128 to +127)
    // char c = SPDR;    // DON'T USE char BECAUSE BECOMES SIGNED!!
    uint8_t c = SPDR;    // Avoid using 'char' while using values above 127

    if (c == START) {
        receiving_state = true;
        receiving_index = 0;
    } else if (c == END) {
        receiving_state = false;
        if (receiving_index > 0) {
            process_command = true;
        }
    } else if (receiving_state) {
        if (receiving_index < BUFFER_SIZE) {
            receiving_buffer[receiving_index++] = c;
        } else {
            receiving_state = false;
        }
    }
}


void processCommand() {

    Serial.print("Processed command: ");
    Serial.println(receiving_buffer);

    if (strcmp(receiving_buffer, "LED_ON") == 0) {
        digitalWrite(LED_PIN, HIGH);
        Serial.println("LED is ON");
    }
    else if (strcmp(receiving_buffer, "LED_OFF") == 0) {
        digitalWrite(LED_PIN, LOW);
        Serial.println("LED is OFF");
    }
    else {
        Serial.println("Unknown command");
    }
}

void loop() {
    // HEAVY PROCESSING SHALL BE IN THE LOOP

    if (process_command) {
        processCommand();   // Called only once!
        process_command = false;    // Critical to avoid repeated calls over the ISR function
    }
}

