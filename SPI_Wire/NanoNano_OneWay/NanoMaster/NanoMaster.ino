// SPI Master Code - Pure String Commands
#include <SPI.h>


// How to Use Sigrok PulseView Software | Logic Analyzer
//     https://sigrok.org/wiki/Downloads
//     Saleae Logic - 24MHz - 8CH - fx2lafw (generic driver for FX2 based LAs) (fx2lafw)
//     Capture: 5M samples @ 12MHz


enum MessageCode : uint8_t {
    START   = 0xF0, // Start of transmission
    END     = 0xF1, // End of transmission
    ACK     = 0xF2, // Acknowledge
    NACK    = 0xF3, // Not acknowledged
    READY   = 0xF4, // Slave has response ready
    ERROR   = 0xF5  // Error frame
};


// Pin definitions
const int BUZZ_PIN = 2; // External BUZZER pin
const int SS_PIN = 10;  // Slave Select pin


void setup() {
    // Initialize SPI
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV16);
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);  // EXPLICITLY SET MSB FIRST! (OTHERWISE is LSB)
    
    // Initialize pins
    pinMode(SS_PIN, OUTPUT);
    digitalWrite(SS_PIN, HIGH);
    pinMode(BUZZ_PIN, OUTPUT);
    digitalWrite(BUZZ_PIN, LOW);
    
    // Initialize serial
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\nSPI Master Initialized - One Way Communication");
}

void loop() {
    // Send string commands directly
    sendString("LED_ON");
    delay(2000);
    
    sendString("LED_OFF");
    delay(2000);
}


void sendString(const char* command) {
    
    // char is signed by default on most Arduino platforms (-128 to +127)
    // char c; // DON'T USE char BECAUSE BECOMES SIGNED!!
    uint8_t c; // Always able to receive (FULL DUPLEX)
  
    digitalWrite(SS_PIN, LOW);
    delayMicroseconds(50);

    // Signals the start of the transmission
    c = SPI.transfer(START);
    Serial.print("Response to START: 0x");
    Serial.println(c, HEX);  // See what we actually get
    if (c == ACK) {
        Serial.println("Receiver acknowledged!");
    } else {
        Serial.println("Receiver NOT acknowledged!");
    }
    delayMicroseconds(10);
    
    // Send command
    int i = 0;
    while (command[i] != '\0') {
        SPI.transfer(command[i]);   // requests a char (ALSO)
        i++;
        delayMicroseconds(10);
    }
    SPI.transfer('\0'); // requests a char (ALSO)
    delayMicroseconds(10);

    // Signals the end of the transmission
    SPI.transfer(END);
    
    delayMicroseconds(50);
    digitalWrite(SS_PIN, HIGH);
    
    Serial.print("Sent: ");
    Serial.println(command);
    
}

