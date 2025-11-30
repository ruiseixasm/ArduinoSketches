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
char _receiving_buffer[BUFFER_SIZE] = {'\0'};


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

size_t sendString(const char* command) {
    size_t length = 0;	// No interrupts, so, not volatile
    uint8_t c; // Avoid using 'char' while using values above 127

    for (size_t s = 0; length == 0 && s < 3; s++) {
  
        digitalWrite(SS_PIN, LOW);
        delayMicroseconds(5);

        // Asks the receiver to start receiving
        c = SPI.transfer(RECEIVE);
        delayMicroseconds(send_delay_us);
        
        // RECEIVE message code
        for (uint8_t i = 0; i < BUFFER_SIZE + 1; i++) { // Has to let '\0' pass, thus the (+ 1)
            if (i > 0) {
				if (command[i - 1] == '\0') {
					c = SPI.transfer(END);
					if (c == '\0') {
						length = i;
						break;
					} else {
						SPI.transfer(ERROR);
						length = 0;
						break;
					}
				} else {
					c = SPI.transfer(command[i]);	// Receives the command[i - 1]
				}
                if (c != command[i - 1]) {    // Excludes NACK situation
					SPI.transfer(ERROR);
                    length = 0;
					break;
				}
            } else {
                c = SPI.transfer(command[0]);	// Doesn't check first char
            }
            delayMicroseconds(send_delay_us);
            if (c == NACK) {
                length = 0;
                break;
            }
        }

        delayMicroseconds(5);
        digitalWrite(SS_PIN, HIGH);

        if (length > 0) {
            Serial.println("Command successfully sent");
        } else {
            Serial.print("Command NOT successfully sent on try: ");
            Serial.println(s + 1);
            Serial.println("BUZZER activated for 10ms!");
            digitalWrite(BUZZ_PIN, HIGH);
            delay(10);  // Buzzer on for 10ms
            digitalWrite(BUZZ_PIN, LOW);
        }
    }

    if (length > 0)
        length--;   // removes the '\0' from the length as final value
    return length;
}


#define receive_delay_us 10 // Receive needs more time to be processed

size_t receiveString() {
    size_t length = 0;	// No interrupts, so, not volatile
    uint8_t c; // Avoid using 'char' while using values above 127
    _receiving_buffer[0] = '\0'; // Avoids garbage printing

    for (size_t r = 0; length == 0 && r < 3; r++) {
  
        digitalWrite(SS_PIN, LOW);
        delayMicroseconds(5);

        // Asks the receiver to start receiving
        c = SPI.transfer(SEND);
        delayMicroseconds(send_delay_us);
            
        // Starts to receive all chars here
        for (uint8_t i = 0; i < BUFFER_SIZE; i++) {	// First char is a control byte
            delayMicroseconds(receive_delay_us);
            if (i > 0) {    // The first response is discarded
                c = SPI.transfer(_receiving_buffer[i - 1]);
                if (c == END) {
                    length = i;
                    break;
                } else if (c == ERROR || c == NACK) {
                    _receiving_buffer[0] = '\0'; // Implicit char
                    length = 0;
                    break;
                } else {
                    _receiving_buffer[i] = c;
                }
            } else {
                c = SPI.transfer('\0');   // Dummy char, not intended to be processed (Slave _sending_state == true)
                if (c == NONE) {
                    _receiving_buffer[0] = '\0'; // Implicit char
                    length = 1;
                    break;
                } else if (c == NACK) {
                    _receiving_buffer[0] = '\0'; // Implicit char
                    length = 0;
                    break;
                }
                _receiving_buffer[0] = c;   // First char received
            }
        }

        delayMicroseconds(5);
        digitalWrite(SS_PIN, HIGH);

        if (length > 0) {
            if (length > 1) {
                Serial.print("Received message: ");
                Serial.println(_receiving_buffer);
            } else {
                Serial.println("Nothing received");
            }
        } else {
            Serial.print("Message NOT successfully received on try: ");
            Serial.println(r + 1);
            Serial.println("BUZZER activated for 2 x 10ms!");
            digitalWrite(BUZZ_PIN, HIGH);
            delay(10);  // Buzzer on for 10ms
            digitalWrite(BUZZ_PIN, LOW);
            delay(200);
            digitalWrite(BUZZ_PIN, HIGH);
            delay(10);  // Buzzer on for 10ms
            digitalWrite(BUZZ_PIN, LOW);
        }
    }

    if (length > 0)
        length--;   // removes the '\0' from the length as final value
    return length;
}


