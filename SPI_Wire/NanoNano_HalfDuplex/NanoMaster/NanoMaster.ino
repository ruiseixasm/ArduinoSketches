// SPI Master Code - Pure String Commands
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


#define send_delay_us 10

bool sendString(const char* command) {
    uint8_t c; // Avoid using 'char' while using values above 127
    receiving_buffer[0] = '\0'; // Avoids garbage printing
    receiving_state = false;
    receiving_index = 0;

    
    bool successfully_sent = false; // It has to start as false to enter in the next loop

    for (size_t s = 0; !successfully_sent && s < 3; s++) {
  
        successfully_sent = true;

        digitalWrite(SS_PIN, LOW);
        delayMicroseconds(send_delay_us);

        // Asks the receiver to start receiving
        SPI.transfer(RECEIVE);
        delayMicroseconds(send_delay_us);
        
        // RECEIVE message code
        uint8_t last_sent = RECEIVE;
        for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
            if (SPI.transfer(command[i]) != last_sent)
                successfully_sent = false;
            last_sent = command[i];
            delayMicroseconds(send_delay_us);
            if (command[i] == '\0') break;
        }

        if (SPI.transfer(END) != '\0')
            successfully_sent = false;
        delayMicroseconds(send_delay_us);

        digitalWrite(SS_PIN, HIGH);

        if (successfully_sent) {
            Serial.println("Command successfully sent");
        } else {
            digitalWrite(BUZZ_PIN, HIGH);
            delay(10);  // Buzzer on for 10ms
            digitalWrite(BUZZ_PIN, LOW);
            Serial.print("Command NOT successfully sent on try: ");
            Serial.println(s + 1);
            Serial.println("BUZZER activated for 10ms!");
        }
    }

    return successfully_sent;
}


#define receive_delay_us 10

bool receiveString() {
    uint8_t c; // Avoid using 'char' while using values above 127
    receiving_buffer[0] = '\0'; // Avoids garbage printing
    receiving_state = false;
    receiving_index = 0;


    bool successfully_received = true;

    
    digitalWrite(SS_PIN, LOW);
    delayMicroseconds(receive_delay_us);

    // Asks the receiver to start sending
    SPI.transfer(SEND);
    delayMicroseconds(receive_delay_us);
    
    // Starts to receive all chars here
    uint8_t last_received = SEND;
    for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
        last_received = SPI.transfer(last_received);
        delayMicroseconds(receive_delay_us);
        if (last_received == NONE || last_received == END) {
            receiving_buffer[i] = '\0'; // Implicit char
            receiving_index = i;
            break;
        } else if (last_received == ERROR) {
            successfully_received = false;
            break;
        } else {
            receiving_buffer[i] = last_received;
        }
    }

    digitalWrite(SS_PIN, HIGH);

    if (successfully_received) {
        if (receiving_index > 0) {
            Serial.print("Received message: ");
            Serial.println(receiving_buffer);
        } else {
            Serial.println("Nothing received");
        }
    } else {
        digitalWrite(BUZZ_PIN, HIGH);
        delay(10);  // Buzzer on for 10ms
        digitalWrite(BUZZ_PIN, LOW);
        Serial.println("Message NOT successfully received");
        Serial.println("BUZZER activated for 10ms!");
    }

    return successfully_received;
}


