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
    
    // WARNING:
    //     AVOID PLACING Serial.print CALLS HERE BECAUSE IT WILL DELAY 
    //     THE POSSIBILITY OF SPI CAPTURE AND RESPONSE IN TIME !!!

    // char is signed by default on most Arduino platforms (-128 to +127)
    // char c; // DON'T USE char BECAUSE BECOMES SIGNED!!
    uint8_t c; // Always able to receive (FULL DUPLEX)
  
    digitalWrite(SS_PIN, LOW);
    delayMicroseconds(5);

    // Signals the start of the transmission
    c = SPI.transfer(START);
    // Serial.print("Response to START: 0x");
    // Serial.println(c, HEX);  // See what we actually get
    // if (c == ACK) {
    //     Serial.println("Receiver acknowledged!");
    // } else {
    //     Serial.println("Receiver NOT acknowledged!");
    // }
    delayMicroseconds(5);
    
    // Send command
    int i = 0;
    while (command[i] != '\0') {
        SPI.transfer(command[i]);   // requests a char (ALSO)
        i++;
        delayMicroseconds(5);
    }
    SPI.transfer('\0'); // requests a char (ALSO)
    delayMicroseconds(5);

    // Signals the end of the transmission
    SPI.transfer(END);
    
    delayMicroseconds(5);
    digitalWrite(SS_PIN, HIGH);
    
    Serial.print("Sent: ");
    Serial.println(command);
    
}

