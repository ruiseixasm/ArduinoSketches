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
#include <ArduinoJson.h>
#include "SPISlave_ESP32.hpp"

#include <SPI.h>

// ESP32 doesn't have AVR registers, so we need ESP32 SPI slave
// But we'll keep the Arduino SPI include for compatibility


// #define SLAVE_CLASS_DEBUG

#define BUFFER_SIZE 128

// Pin definitions - define these in your main sketch
#ifndef GREEN_LED_PIN
#define GREEN_LED_PIN 2
#endif



SPISlave_ESP32 spi;

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

    // SAME STATIC BUFFERS
    static char _receiving_buffer[BUFFER_SIZE];
    static char _sending_buffer[BUFFER_SIZE];
    static volatile uint8_t _receiving_index;
    static volatile uint8_t _sending_index;
    static volatile MessageCode _transmission_mode;
    static volatile bool _process_message;


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
        // ESP32 VERSION (instead of AVR SPCR)
        // Set MISO as OUTPUT
        pinMode(19, OUTPUT);  // ESP32 VSPI MISO pin
        
        // Initialize SPI with Arduino library
        SPI.begin();
        SPI.setBitOrder(MSBFIRST);
        SPI.setDataMode(SPI_MODE0);

        // Note: ESP32 Arduino SPI doesn't have SPCR or SPIE
        // For slave mode, we need ESP32's native SPI slave driver
        // But we'll keep the interface similar
    }

    
public:

    Slave_class() {

        pinMode(GREEN_LED_PIN, OUTPUT);

        spi.onByte(spiByteHandler);
        spi.begin();
    }

    ~Slave_class() {
        // Cleanup
        SPI.end();
        
        pinMode(GREEN_LED_PIN, INPUT);
        digitalWrite(GREEN_LED_PIN, LOW);
    }


    // Actual interrupt handler
    static void spiByteHandler(uint8_t c, uint8_t &reply) {

        // WARNING 1:
        //     AVOID PLACING HEAVY CODE OR CALL HERE. THIS INTERRUPTS THE LOOP!

        // WARNING 2:
        //     AVOID PLACING Serial.print CALLS HERE BECAUSE IT WILL DELAY 
        //     THE POSSIBILITY OF SPI CAPTURE AND RESPONSE IN TIME !!!

        // WARNING 3:
        //     THE SETTING OF THE `SPDR` VARIABLE SHALL ALWAYS BE ON TOP, FIRSTLY THAN ALL OTHERS!


        // your AVR ISR code — unchanged — but using:
        //   c      = received byte
        //   reply  = byte to send back

        // Everything else stays the same logic.
        // Replace:
        //      SPDR = X;
        // with:
        //      reply = X;


        if (c < 128) {  // Only ASCII chars shall be transmitted as data

            // switch O(1) is more efficient than an if-else O(n) sequence because the compiler uses a jump table

            switch (_transmission_mode) {
                case RECEIVE:
                    if (_receiving_index < BUFFER_SIZE) {
                        // Returns same received char as receiving confirmation (no need to set SPDR)
                        _receiving_buffer[_receiving_index++] = c;
                    } else {
                        reply = FULL;    // ALWAYS ON TOP
                        _transmission_mode = NONE;
                    }
                    break;
                case SEND:
					if (_sending_index < BUFFER_SIZE) {
						reply = _sending_buffer[_sending_index];		// This way avoids being the critical path (in advance)
						if (_receiving_index > _sending_index) {	// Less missed sends this way
							reply = END;	// All chars have been checked
							break;
						}
					} else {
						reply = FULL;
						_transmission_mode = NONE;
						break;
					}
					// Starts checking 2 indexes after
					if (_sending_index > 1) {    // Two positions of delay
						if (c != _sending_buffer[_receiving_index]) {   // Also checks '\0' char
							reply = ERROR;
							_transmission_mode = NONE;  // Makes sure no more communication is done, regardless
							break;
						}
						_receiving_index++; // Starts checking after two sent
					}
					// Only increments if NOT at the end of the string being sent
					if (_sending_buffer[_sending_index] != '\0') {
						_sending_index++;
					}
                    break;
                default:
                    reply = NACK;
            }

        } else {    // It's a control message 0xFX
            
            // switch O(1) is more efficient than an if-else O(n) sequence because the compiler uses a jump table

            switch (c) {
                case RECEIVE:
                    reply = ACK;
                    _transmission_mode = RECEIVE;
                    _receiving_index = 0;
                    break;
                case SEND:
                    if (_sending_buffer[0] == '\0') {
                        reply = NONE;    // Nothing to send
                    } else {    // Starts sending right away, so, no ACK
                        reply = _sending_buffer[0];
                        _transmission_mode = SEND;
                        _sending_index = 1;  // Skips to the next char
                        _receiving_index = 0;
                    }
                    break;
                case END:
                    reply = ACK;
                    if (_transmission_mode == RECEIVE) {
                        _process_message = true;
                    } else if (_transmission_mode == SEND) {
                        _sending_buffer[0] = '\0';  // Makes sure the sending buffer is marked as empty (NONE next time)
                    }
                    _transmission_mode = NONE;
                    break;
                case ACK:
                    reply = READY;
                    break;
                case ERROR:
                case FULL:
                    reply = ACK;
                    _transmission_mode = NONE;
                    break;
                default:
                    reply = NACK;
            }
        }
    }

	
    void process() {
        spi.loop();              // Process SPI bytes
        if (_process_message) {  // If JSON message completed
            _process_message = false;
            processMessage();
        }
    }


    // NOTE: handleSPI_Interrupt() removed - ESP32 uses callback system
    // Your AVR ISR logic would need to be adapted to ESP32's transaction-based system
    // For external logic analyzer, you'd need to implement byte-by-byte handling
    // similar to your AVR code but using ESP32 SPI slave driver
};


// Initialize static members
char Slave_class::_receiving_buffer[BUFFER_SIZE] = {'\0'};
char Slave_class::_sending_buffer[BUFFER_SIZE] = {'\0'};

volatile uint8_t Slave_class::_receiving_index = 0;
volatile uint8_t Slave_class::_sending_index = 0;
volatile Slave_class::MessageCode Slave_class::_transmission_mode = Slave_class::NONE;
volatile bool Slave_class::_process_message = false;



#endif // SLAVE_CLASS_HPP
