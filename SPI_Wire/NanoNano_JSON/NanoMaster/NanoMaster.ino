// SPI Master Code - Pure String Commands
#include <SPI.h>
#include <ArduinoJson.h>    // Include ArduinoJson Library


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
char receiving_buffer[BUFFER_SIZE] = {'\0'};
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
    receiveString();    // Before because sendString results in a command that takes time
    sendString("{'t':'Nano','m':2,'n':'ON','f':'Talker-9f','i':3540751170,'c':24893}");
    delay(2000);

    receiveString();    // Before because sendString results in a command that takes time
    sendString("{'t':'Nano','m':2,'n':'OFF','f':'Talker-9f','i':3540751170,'c':24893}");
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
        delayMicroseconds(5);

        // Asks the receiver to start receiving
        SPI.transfer(RECEIVE);
        delayMicroseconds(send_delay_us);
        
        // RECEIVE message code
        for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
            if (i > 0) {
                if (SPI.transfer(command[i]) != command[i - 1])
                    successfully_sent = false;
            } else {
                SPI.transfer(command[0]);
            }
            delayMicroseconds(send_delay_us);
            // Don't make '\0' implicit in order to not have to change the SPDR on the slave side!!
            if (command[i] == '\0') break;
        }

        if (SPI.transfer(END) != '\0')  // Because the last char is always '\0'
            successfully_sent = false;

        delayMicroseconds(5);
        digitalWrite(SS_PIN, HIGH);

        if (successfully_sent) {
            Serial.println("Command successfully sent");
        } else {
            Serial.print("Command NOT successfully sent on try: ");
            Serial.println(s + 1);
            // Serial.println("BUZZER activated for 10ms!");
            // digitalWrite(BUZZ_PIN, HIGH);
            // delay(10);  // Buzzer on for 10ms
            // digitalWrite(BUZZ_PIN, LOW);
        }
    }

    return successfully_sent;
}


#define receive_delay_us 10 // Receive needs more time to be processed

bool receiveString() {
    uint8_t c; // Avoid using 'char' while using values above 127
    receiving_buffer[0] = '\0'; // Avoids garbage printing
    receiving_state = false;
    receiving_index = 0;


    bool successfully_received = false; // It has to start as false to enter in the next loop

    for (size_t r = 0; !successfully_received && r < 3; r++) {
  
        successfully_received = true;

        digitalWrite(SS_PIN, LOW);
        delayMicroseconds(5);

        // Asks the receiver to start receiving
        c = SPI.transfer(SEND);
        delayMicroseconds(send_delay_us);
            
        // Starts to receive all chars here
        for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
            delayMicroseconds(receive_delay_us);
            if (i > 0) {    // The first response is discarded
                c = SPI.transfer(receiving_buffer[i - 1]);
                if (c == END) {
                    receiving_buffer[i] = '\0'; // Implicit char
                    receiving_index = i;
                    break;
                } else if (c == ERROR) {
                    receiving_buffer[0] = '\0'; // Implicit char
                    receiving_index = 0;
                    successfully_received = false;
                    break;
                } else {
                    receiving_buffer[i] = c;
                }
            } else {
                c = SPI.transfer('\0');   // Dummy char, not intended to be processed
                if (c == NONE) {
                    receiving_buffer[0] = '\0'; // Implicit char
                    receiving_index = 0;
                    break;
                }
                receiving_buffer[0] = c;   // Dummy char, not intended to be processed
            }
        }

        delayMicroseconds(5);
        digitalWrite(SS_PIN, HIGH);

        if (successfully_received) {
            if (receiving_index > 0) {
                Serial.print("Received message: ");
                Serial.println(receiving_buffer);
            } else {
                Serial.println("Nothing received");
            }
        } else {
            Serial.print("Message NOT successfully received on try: ");
            Serial.println(r + 1);
            // Serial.println("BUZZER activated for 2 x 10ms!");
            // digitalWrite(BUZZ_PIN, HIGH);
            // delay(10);  // Buzzer on for 10ms
            // digitalWrite(BUZZ_PIN, LOW);
            // delay(100);
            // digitalWrite(BUZZ_PIN, HIGH);
            // delay(10);  // Buzzer on for 10ms
            // digitalWrite(BUZZ_PIN, LOW);
        }
    }

    return successfully_received;
}


