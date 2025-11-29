// SPI Slave Code - Pure String Commands
#include <SPI.h>


// Pin definitions
const int SS_PIN = 10;


void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\nSPI Slave Ready - One Way Communication");

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

    c += 0xF0;
    SPDR = c;
}

void loop() {
}

