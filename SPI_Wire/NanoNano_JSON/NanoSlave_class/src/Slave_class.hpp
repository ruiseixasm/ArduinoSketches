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
#ifndef SLAVE_CLASS_HPP
#define SLAVE_CLASS_HPP

#include <Arduino.h>
#include <SPI.h>
#include <avr/interrupt.h>
#include <ArduinoJson.h>


// Pin definitions - define these in your main sketch
#ifndef GREEN_LED_PIN
#define GREEN_LED_PIN 2
#endif

#ifndef YELLOW_LED_PIN
#define YELLOW_LED_PIN 21
#endif

#define BUFFER_SIZE 128


class Slave_class
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
        FULL    = 0xF9, // Signals the buffer as full
        
        VOID    = 0xFF  // MISO floating (0xFF) → no slave responding
    };

private:
    // Static instance for ISR access
    static Slave_class* _instance;
    
    // Buffers and state variables
    char _receiving_buffer[BUFFER_SIZE];
    char _sending_buffer[BUFFER_SIZE];
    volatile uint8_t _buffer_index;
    volatile MessageCode _transmission_mode;
    volatile bool _process_message;

    
    
    // Actual interrupt handler
    void handleSPI_Interrupt() {

        // WARNING 1:
        //     AVOID PLACING HEAVY CODE OR CALL HERE. THIS INTERRUPTS THE LOOP!

        // WARNING 2:
        //     AVOID PLACING Serial.print CALLS HERE BECAUSE IT WILL DELAY 
        //     THE POSSIBILITY OF SPI CAPTURE AND RESPONSE IN TIME !!!

        // WARNING 3:
        //     THE SETTING OF THE `SPDR` VARIABLE SHALL ALWAYS BE ON TOP, FIRSTLY THAN ALL OTHERS!

        uint8_t c = SPDR;    // Avoid using 'char' while using values above 127

        if (c < 128) {  // Only ASCII chars shall be transmitted as data

            // switch O(1) is more efficient than an if-else O(n) sequence because the compiler uses a jump table

            switch (_transmission_mode) {
                case RECEIVE:
                    if (_buffer_index < BUFFER_SIZE) {
                        // Returns same received char as receiving confirmation (no need to set SPDR)
                        _receiving_buffer[_buffer_index++] = c;
                    } else {
                        SPDR = FULL;    // ALWAYS ON TOP
                        _transmission_mode = NONE;
                    }
                    break;
                case SEND:
                    if (_buffer_index < BUFFER_SIZE) {
                        SPDR = _sending_buffer[_buffer_index++];    // This way avoids the critical path bellow (in advance)
                        // Boundary safety takes the most toll, that's why SPDR typical scenario is given in advance
                        if (_buffer_index > 2) {    // Two positions of delay (note the _buffer_index++ above)
                            uint8_t previous_c = _sending_buffer[_buffer_index - 3];
                            if (previous_c != c) {
                                SPDR = ERROR;
                                _transmission_mode = NONE;  // Makes sure no more communication is done, regardless
                            } else if (previous_c == '\0') {
                                SPDR = END;     // Main reason for transmission fail (critical path) (one in many though)
                                _transmission_mode = NONE;
                                _sending_buffer[0] = '\0';   // Makes sure the sending buffer is marked as empty (NONE next time)
                            }
                        }
                    } else {
                        SPDR = FULL;
                        _transmission_mode = NONE;
                    }
                    break;
                default:
                    SPDR = NACK;
            }

        } else {    // It's a control message 0xFX
            
            // switch O(1) is more efficient than an if-else O(n) sequence because the compiler uses a jump table

            switch (c) {
                case RECEIVE:
                    SPDR = ACK;
                    _transmission_mode = RECEIVE;
                    _buffer_index = 0;
                    break;
                case SEND:
                    if (_sending_buffer[0] == '\0') {
                        SPDR = NONE;    // Nothing to send
                    } else {    // Starts sending right away, so, no ACK
                        SPDR = _sending_buffer[0];
                        _transmission_mode = SEND;
                        _buffer_index = 1;  // Skips the sent 0
                    }
                    break;
                case END:
                    SPDR = ACK;
                    _transmission_mode = NONE;
                    _process_message = true;
                    break;
                case ACK:
                    SPDR = READY;
                    break;
                case ERROR:
                case FULL:
                    SPDR = ACK;
                    _transmission_mode = NONE;
                    break;
                default:
                    SPDR = NACK;
            }
        }
    }


    void processMessage() {

        Serial.print("Processed command: ");
        Serial.println(_receiving_buffer);

        // JsonDocument in the stack makes sure its memory is released (NOT GLOBAL)
        #if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument message_doc;
        #else
        StaticJsonDocument<BUFFER_SIZE> message_doc;
        #endif

        DeserializationError error = deserializeJson(message_doc, _receiving_buffer, BUFFER_SIZE);
        if (error) {
            return false;
        }
        JsonObject json_message = message_doc.as<JsonObject>();

        const char* command_name = json_message["n"].as<const char*>();


        if (strcmp(command_name, "ON") == 0) {
            digitalWrite(GREEN_LED_PIN, HIGH);
            json_message["n"] = "OK_ON";
            size_t length = serializeJson(json_message, _sending_buffer, BUFFER_SIZE);
            Serial.print("LED is ON");
            Serial.print(" | Sending: ");
            Serial.println(_sending_buffer);

        } else if (strcmp(command_name, "OFF") == 0) {
            digitalWrite(GREEN_LED_PIN, LOW);
            json_message["n"] = "OK_OFF";
            size_t length = serializeJson(json_message, _sending_buffer, BUFFER_SIZE);
            Serial.print("LED is OFF");
            Serial.print(" | Sending: ");
            Serial.println(_sending_buffer);
        } else {
            json_message["n"] = "BUZZ";
            size_t length = serializeJson(json_message, _sending_buffer, BUFFER_SIZE);
            Serial.print("Unknown command");
            Serial.print(" | Sending: ");
            Serial.println(_sending_buffer);
        }
    }


    void initSPISlave() {  // FIX 3: Add missing method definition
        pinMode(MISO, OUTPUT);  // MISO must be OUTPUT for Slave to send data!
        
        // Initialize SPI as slave - EXPLICIT MSB FIRST
        SPCR = 0;  // Clear register
        SPCR |= _BV(SPE);    // SPI Enable
        SPCR |= _BV(SPIE);   // SPI Interrupt Enable  
        SPCR &= ~_BV(DORD);  // MSB First (DORD=0 for MSB first)
        SPCR &= ~_BV(CPOL);  // Clock polarity 0
        SPCR &= ~_BV(CPHA);  // Clock phase 0 (MODE0)
    }


public:

    Slave_class() {

        _instance = this;  // Set static instance
        
        // Initialize buffers
        _receiving_buffer[0] = '\0';
        _sending_buffer[0] = '\0';
        _buffer_index = 0;
        _transmission_mode = NONE;
        _process_message = false;

        // Initialize pins
        pinMode(GREEN_LED_PIN, OUTPUT);
        digitalWrite(GREEN_LED_PIN, LOW);
        pinMode(YELLOW_LED_PIN, OUTPUT);
        digitalWrite(YELLOW_LED_PIN, LOW);

        // Setup SPI as slave
        initSPISlave();
    }

    ~Slave_class() {
        if (_instance == this) {
            // Disable SPI interrupt
            SPCR &= ~(1 << SPIE);
            _instance = nullptr;
        }

        // This returns the pin to exact power-on state:
        pinMode(GREEN_LED_PIN, INPUT);
        digitalWrite(GREEN_LED_PIN, LOW);  // Important: disables any pull-up

        pinMode(YELLOW_LED_PIN, INPUT);
        digitalWrite(YELLOW_LED_PIN, LOW);
    }

    
    // Static ISR wrapper (called by hardware)
    static void isrWrapper() {
        if (_instance) {
            _instance->handleSPI_Interrupt();
        }
    }
    

    void process() {
        if (_process_message) {
            processMessage();   // Called only once!
            _process_message = false;    // Critical to avoid repeated calls over the ISR function
        }
    }

};


// Initialize static member
Slave_class* Slave_class::_instance = nullptr;


#endif // SLAVE_CLASS_HPP
