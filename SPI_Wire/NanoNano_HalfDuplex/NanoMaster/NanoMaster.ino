// SPI Master Code - Pure String Commands
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
const int BUZZ_PIN = 2; // External BUZZER pin
const int SS_PIN = 10;  // Slave Select pin

#define BUFFER_SIZE 128
char receiving_buffer[BUFFER_SIZE];
byte receiving_index = 0;   // No interrupts, so, not volatile
bool receiving_state = true;


void setup() {
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
    Serial.println("\n\nSPI Master Initialized - Half-Duplex Mode");
}

void loop() {
    // Send string commands directly
    sendString("LED_ON");
    delay(2000);
    
    sendString("LED_OFF");
    delay(2000);
}


#define micro_delay 10

void sendString(const char* command) {
    uint8_t c; // Avoid using 'char' while using values above 127
    receiving_buffer[0] = '\0'; // Avoids garbage printing
    receiving_state = false;
    receiving_index = 0;
  
    digitalWrite(SS_PIN, LOW);
    delayMicroseconds(micro_delay);
    
    // Send command
    int i = 0;
    while (command[i] != '\0') {
        c = SPI.transfer(command[i]);   // requests a char (ALSO)
        if (c != '\0') {    // In case something was received
            receiving_state = true;
            receiving_buffer[receiving_index++] = c;
        } else if (receiving_index > 0) {
            receiving_state = false;    // Received everything
        }
        i++;
        delayMicroseconds(micro_delay);
    }
    c = SPI.transfer('\0'); // requests a char (ALSO)

    if (receiving_state) {  // In case the data wasn't all received above

        do {
            Serial.print(c);
            receiving_buffer[receiving_index++] = c;
            Serial.print(c);
            if (c == '\0') {
                break;
            } else if  (receiving_index < BUFFER_SIZE) {
                delayMicroseconds(micro_delay);
                c = SPI.transfer(0xFF);  // Special char to collect the receiving messages (Valid ASCII goes up to 127 (7F))
            }
        } while(receiving_index < BUFFER_SIZE);
    }
    
    delayMicroseconds(50);
    digitalWrite(SS_PIN, HIGH);
    
    Serial.print("\nSent: ");
    Serial.print(command);
    Serial.print(" | Received: ");
    Serial.println(receiving_buffer);
    
    // NEW CODE: Check if response is "BUZZ" and activate buzzer
    if (strcmp(receiving_buffer, "BUZZ") == 0) {
        digitalWrite(BUZZ_PIN, HIGH);
        delay(100);  // Buzzer on for 200ms
        digitalWrite(BUZZ_PIN, LOW);
        Serial.println("BUZZER activated for 200ms!");
    }
  
}

