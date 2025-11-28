// SPI Master Code - Pure String Commands
#include <SPI.h>

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
    SPI.setClockDivider(SPI_CLOCK_DIV16);
    SPI.setDataMode(SPI_MODE0);
    
    // Initialize pins
    pinMode(SS_PIN, OUTPUT);
    digitalWrite(SS_PIN, HIGH);
    pinMode(BUZZ_PIN, OUTPUT);
    digitalWrite(BUZZ_PIN, LOW);
    
    // Initialize serial
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\nSPI Master Initialized - String Mode");
}

void loop() {
    // Send string commands directly
    sendString("LED_ON");
    delay(2000);
    
    sendString("LED_OFF");
    delay(2000);
}


void sendString(const char* command) {
    char c; // Always able to receive (FULL DUPLEX)
    receiving_buffer[0] = '\0'; // Avoids garbage printing
    receiving_state = false;
    receiving_index = 0;
  
    digitalWrite(SS_PIN, LOW);
    delayMicroseconds(50);
    
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
        delayMicroseconds(10);
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
                delayMicroseconds(10);
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

