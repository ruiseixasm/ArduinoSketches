// SPI Master Code - Pure String Commands
#include <SPI.h>


// How to Use Sigrok PulseView Software | Logic Analyzer
//     https://sigrok.org/wiki/Downloads
//     Saleae Logic - 24MHz - 8CH - fx2lafw (generic driver for FX2 based LAs) (fx2lafw)
//     Capture: 5M samples @ 12MHz


// Pin definitions
const int SS_PIN = 10;  // Slave Select pin


void setup() {

    // SPI_CLOCK_DIV2 = 8MHz (Arduino Nano 16MHz / 2)
    // SPI_CLOCK_DIV4 = 4MHz (Arduino Nano 16MHz / 4) ← This is what you want
    // SPI_CLOCK_DIV8 = 2MHz (Arduino Nano 16MHz / 8)
    // SPI_CLOCK_DIV16 = 1MHz (Arduino Nano 16MHz / 16) ← What you had

    // Initialize SPI
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV4);    // Only affects the char transmission
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);  // EXPLICITLY SET MSB FIRST! (OTHERWISE is LSB)
    
    // Initialize pins
    pinMode(SS_PIN, OUTPUT);
    digitalWrite(SS_PIN, HIGH);
    
    // Initialize serial
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\nSPI Master Initialized - Protocol Communication");
}

unsigned int delays[] = {0, 1, 2, 3, 4};

void loop() {
    for (size_t delay_i = 0; delay_i < 5; delay_i++) {
        sendBytes(delays[delay_i]);
        delay(500);
    }
}


bool sendBytes(unsigned int delay_us) {
    
    // WARNING:
    //     AVOID PLACING Serial.print CALLS HERE BECAUSE IT WILL DELAY 
    //     THE POSSIBILITY OF SPI CAPTURE AND RESPONSE IN TIME !!!

    // char is signed by default on most Arduino platforms (-128 to +127)
    // char c; // DON'T USE char BECAUSE BECOMES SIGNED!!
    uint8_t c; // Always able to receive (FULL DUPLEX)

    digitalWrite(SS_PIN, LOW);
    delayMicroseconds(delay_us);

    for (uint8_t next_byte = 0x0; next_byte < 0x6; next_byte++) {
        // Signals the start of the transmission
        SPI.transfer(next_byte + (uint8_t)delay_us);
        delayMicroseconds(delay_us);
    }
    
    delayMicroseconds(delay_us);
    digitalWrite(SS_PIN, HIGH);
}

