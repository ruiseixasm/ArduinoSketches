/*
JsonTalkie - Json Talkie is intended for direct IoT communication.
Original Copyright (c) 2025 Rui Seixas Monteiro. All right reserved.
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.
https://github.com/ruiseixasm/JsonTalkie
*/
#ifndef MASTER_CLASS_HPP
#define MASTER_CLASS_HPP

#include <SPI.h>
#include <ArduinoJson.h>    // Include ArduinoJson Library


// Pin definitions
extern const int BUZZ_PIN;  // Declare as external (defined elsewhere)

#define BUFFER_SIZE 128
char _receiving_buffer[BUFFER_SIZE] = {'\0'};


// To make this value the minimum possible, always place the setting SPDR on top in the Slave code (SPDR =)
#define send_delay_us 10
#define receive_delay_us 10

class Master_class
{
public:
    
    enum MessageCode : uint8_t {
        START   = 0xF0, // Start of transmission
        END     = 0xF1, // End of transmission
        ACK     = 0xF2, // Acknowledge
        NACK    = 0xF3, // Not acknowledged
        READY   = 0xF4, // Slave has response ready
        ERROR   = 0xF5, // Error frame
        RECEIVE = 0xF6, // Asks the receiver to start receiving
        SEND    = 0xF7, // Asks the receiver to start sending
        NONE    = 0xF8, // Means nothing to send
        FULL    = 0xF9  // Signals the buffer as full
    };

private:

    int _ss_pin = 10;

    size_t sendString(const char* command) {
        size_t length = 0;	// No interrupts, so, not volatile
        uint8_t c; // Avoid using 'char' while using values above 127

        for (size_t s = 0; length == 0 && s < 3; s++) {
    
            digitalWrite(_ss_pin, LOW);
            delayMicroseconds(5);

            // Asks the Slave to start receiving
            c = SPI.transfer(RECEIVE);
            delayMicroseconds(send_delay_us);
            
            for (uint8_t i = 0; i < BUFFER_SIZE + 1; i++) { // Has to let '\0' pass, thus the (+ 1)
                if (i > 0) {
                    if (command[i - 1] == '\0') {
                        c = SPI.transfer(END);
                        if (c == '\0') {
                            length = i;
                            break;
                        } else {
                            length = 0;
                            break;
                        }
                    } else {
                        c = SPI.transfer(command[i]);	// Receives the command[i - 1]
                    }
                    if (c != command[i - 1]) {    // Excludes NACK situation
                        length = 0;
                        break;
                    }
                } else if (command[0] != '\0') {
                    c = SPI.transfer(command[0]);	// Doesn't check first char
                } else {
                    length = 1; // Nothing sent
                    break;
                }
                delayMicroseconds(send_delay_us);
                if (c == NACK) {
                    length = 0;
                    break;
                }
            }

            if (length == 0) {
                SPI.transfer(ERROR);
                // _receiving_buffer[0] = '\0'; // Implicit char
            } else if (command[length - 1] != '\0') {
                SPI.transfer(FULL);
                Serial.println("FULL");
                // _receiving_buffer[0] = '\0';
                length = 1; // Avoids another try
            }

            delayMicroseconds(5);
            digitalWrite(_ss_pin, HIGH);

            if (length > 0) {
                if (length > 1) {
                    Serial.println("Command successfully sent");
                } else {
                    Serial.println("\tNothing sent");
                }
            } else {
                Serial.print("\t\tCommand NOT successfully sent on try: ");
                Serial.println(s + 1);
                Serial.println("\t\tBUZZER activated for 10ms!");
                digitalWrite(BUZZ_PIN, HIGH);
                delay(10);  // Buzzer on for 10ms
                digitalWrite(BUZZ_PIN, LOW);
                delay(500);
            }
        }

        if (length > 0)
            length--;   // removes the '\0' from the length as final value
        return length;
    }


    size_t receiveString() {
        size_t length = 0;	// No interrupts, so, not volatile
        uint8_t c; // Avoid using 'char' while using values above 127
        _receiving_buffer[0] = '\0'; // Avoids garbage printing

        for (size_t r = 0; length == 0 && r < 3; r++) {
    
            digitalWrite(_ss_pin, LOW);
            delayMicroseconds(5);

            // Asks the Slave to start receiving
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
                        length = 0;
                        break;
                    } else {
                        _receiving_buffer[i] = c;
                    }
                } else {
                    c = SPI.transfer('\0');   // Dummy char, not intended to be processed (Slave _sending_state == true)
                    if (c < 128) {   // Only accepts ASCII chars
                        _receiving_buffer[0] = c;   // First char received
                    } else if (c == NONE) {
                        _receiving_buffer[0] = '\0'; // Sets receiving as nothing (for prints)
                        length = 1;
                        break;
                    } else {    // Includes NACK (implicit)
                        length = 0;
                        break;
                    }
                }
            }

            if (length == 0) {
                SPI.transfer(ERROR);    // Results from ERROR or NACK send by the Slave and makes Slave reset to NONE
                _receiving_buffer[0] = '\0'; // Implicit char
            } else if (_receiving_buffer[length - 1] != '\0') {
                SPI.transfer(FULL);
                _receiving_buffer[0] = '\0';
                length = 1; // Avoids another try
            }

            delayMicroseconds(5);
            digitalWrite(_ss_pin, HIGH);

            if (length > 0) {
                if (length > 1) {
                    Serial.print("Received message: ");
                    Serial.println(_receiving_buffer);
                } else {
                    Serial.println("\tNothing received");
                }
            } else {
                Serial.print("\t\tMessage NOT successfully received on try: ");
                Serial.println(r + 1);
                Serial.println("\t\tBUZZER activated for 2 x 10ms!");
                digitalWrite(BUZZ_PIN, HIGH);
                delay(10);  // Buzzer on for 10ms
                digitalWrite(BUZZ_PIN, LOW);
                delay(200);
                digitalWrite(BUZZ_PIN, HIGH);
                delay(10);  // Buzzer on for 10ms
                digitalWrite(BUZZ_PIN, LOW);
                delay(500);
            }
        }

        if (length > 0)
            length--;   // removes the '\0' from the length as final value
        return length;
    }


    bool acknowledgeReady() {
        uint8_t c; // Avoid using 'char' while using values above 127
        bool acknowledge = false;

        for (size_t a = 0; !acknowledge && a < 3; a++) {
    
            digitalWrite(_ss_pin, LOW);
            delayMicroseconds(5);

            // Asks the Slave to acknowledge readiness
            SPI.transfer(ACK);
            delayMicroseconds(send_delay_us);
            c = SPI.transfer(ACK);  // When the response is collected
            
            if (c == READY) {
                acknowledge = true;
            }
            
            delayMicroseconds(5);
            digitalWrite(_ss_pin, HIGH);

        }

        if (acknowledge) {
            Serial.println("Slave is ready!");
        } else {
            Serial.println("Slave is NOT ready!");
        }

        return acknowledge;
    }


public:

    Master_class(int ss_pin = 10) {
        // Initialize SPI
        SPI.begin();
        SPI.setClockDivider(SPI_CLOCK_DIV4);    // Only affects the char transmission
        SPI.setDataMode(SPI_MODE0);
        SPI.setBitOrder(MSBFIRST);  // EXPLICITLY SET MSB FIRST! (OTHERWISE is LSB)
        // Enable the SS pin
        pinMode(ss_pin, OUTPUT);
        digitalWrite(ss_pin, HIGH);
        // Sets the class SS pin
        _ss_pin = ss_pin;
    }

    ~Master_class() {
        // This returns the pin to exact power-on state:
        pinMode(_ss_pin, INPUT);
        digitalWrite(_ss_pin, LOW);  // Important: disables any pull-up
    }

    
    bool test() {

        if (acknowledgeReady()) {

            size_t length = 0;

            // ON cycle
            length = sendString("{'t':'Nano','m':2,'n':'ON','f':'Talker-9f','i':3540751170,'c':24893}");
            if (length == 0) return false;
            delay(1000);
            length = sendString("");    // Testing sending nothing at all
            delay(1000);
            length = receiveString();
            if (length == 0) return false;
            length = receiveString();   // Testing that receiving nothing also works
            if (length > 0) return false;

            // OFF cycle
            length = sendString("{'t':'Nano','m':2,'n':'OFF','f':'Talker-9f','i':3540751170,'c':24893}");
            if (length == 0) return false;
            delay(1000);
            length = sendString("");
            delay(1000);
            length = receiveString();
            if (length == 0) return false;
            length = receiveString();
            if (length > 0) return false;

        }

        return true;
    }

};


#endif // MASTER_CLASS_HPP
