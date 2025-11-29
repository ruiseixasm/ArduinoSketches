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
    uint8_t c = SPDR;    // The most important line!!

    // Serial.print("0. Slave received: 0x");
    // Serial.println(c, HEX);  // Debug what byte actually arrives
    
    if (c == START) {
        // Serial.println("1. Start receiving");
        receiving_state = true;
        receiving_index = 0;
        SPDR = ACK;  // Send acknowledgment back to Master
    } else if (c == END) {
        // Serial.println("2. End receiving");
        receiving_state = false;
        if (receiving_index > 0) {
            process_command = true;
            SPDR = receiving_index - 1; // Returns the total amount of bytes (excluding '\0')
        } else {
            SPDR = ERROR;   // No chars received
        }
    } else if (receiving_state) {

        if (receiving_index < BUFFER_SIZE) {
            if (c == '\0') {
                SPDR = receiving_index; // Returns the total amount of bytes (excluding '\0')
            } else {
                SPDR = ACK;  // Send acknowledgment back to Master
            }
            receiving_buffer[receiving_index++] = c;
        } else {
            receiving_state = false;
            SPDR = ERROR;     // Send acknowledgment back to Master
        }
    } else {
        SPDR = ERROR;
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

