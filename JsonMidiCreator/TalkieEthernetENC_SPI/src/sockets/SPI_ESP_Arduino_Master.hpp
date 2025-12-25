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
#ifndef SPI_ESP_ARDUINO_MASTER_VSPI_HPP
#define SPI_ESP_ARDUINO_MASTER_VSPI_HPP


#include <SPI.h>
#include <ArduinoJson.h>    // Include ArduinoJson Library to be used as a dictionary
#include "../BroadcastSocket.hpp"

// #define BROADCAST_SPI_DEBUG
// #define BROADCAST_SPI_DEBUG_1
// #define BROADCAST_SPI_DEBUG_2
#define BROADCAST_SPI_DEBUG_NEW


#define ENABLE_DIRECT_ADDRESSING


#define send_delay_us 10
#define receive_delay_us 10


#define MAX_NAMES 8
#define NAME_LEN  16   // includes '\0'


struct NameEntry {
    char name[NAME_LEN];
    uint8_t value;
};

class NameTable {
private:
    NameEntry _entries[MAX_NAMES];
    uint8_t _count = 0;

public:
    bool add(const char* name, uint8_t value) {
        if (_count >= MAX_NAMES)
            return false;

        // Reject too-long names
        size_t len = strlen(name);
        if (len >= NAME_LEN)
            return false;

        // Prevent duplicates
        for (uint8_t i = 0; i < _count; ++i) {
            if (strcmp(_entries[i].name, name) == 0)
                return false;
        }

        strcpy(_entries[_count].name, name);
        _entries[_count].value = value;
        ++_count;
        return true;
    }

    bool get_pin(const char* name, uint8_t& pin) const {
        for (uint8_t i = 0; i < _count; ++i) {
            if (strcmp(_entries[i].name, name) == 0) {
                pin = _entries[i].value;
                return true;
            }
        }
        return false;
    }

};


class SPI_ESP_Arduino_Master : public BroadcastSocket {
public:

    enum StatusByte : uint8_t {
        ACK     = 0xF0, // Acknowledge
        NACK    = 0xF1, // Not acknowledged
        READY   = 0xF2, // Slave is ready
        BUSY    = 0xF3, // Tells the Master to wait a little
        RECEIVE = 0xF4, // Asks the receiver to start receiving
        SEND    = 0xF5, // Asks the receiver to start sending
        NONE    = 0xF6, // Means nothing to send
        START   = 0xF7, // Start of transmission
        END     = 0xF8, // End of transmission
		LAST	= 0xF9,	// Asks for the last char
		DONE	= 0xFA,	// Marks the action as DONE
        ERROR   = 0xFB, // Error frame
        FULL    = 0xFC, // Signals the buffer as full
        
        VOID    = 0xFF  // MISO floating (0xFF) → no slave responding
    };


protected:

	SPIClass* _spi_instance;  // Pointer to SPI instance
	bool _initiated = false;
    int* _ss_pins;
    uint8_t _ss_pins_count = 0;
    uint8_t _actual_ss_pin = 15;	// GPIO15 for HSPI SCK
	// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
	NameTable _named_pins_table;


    // Constructor
    SPI_ESP_Arduino_Master(
		int* ss_pins, uint8_t ss_pins_count, JsonTalker* const* const json_talkers, uint8_t talker_count, SourceValue source_value = SourceValue::REMOTE
	) : BroadcastSocket(json_talkers, talker_count, source_value) {
            
        	_ss_pins = ss_pins;
        	_ss_pins_count = ss_pins_count;
            _max_delay_ms = 0;  // SPI is sequencial, no need to control out of order packages
        }

    
    // Specific methods associated to Arduino SPI as Master

	
    bool sendSPI(uint8_t length, int ss_pin) {
        uint8_t size = 0;	// No interrupts, so, not volatile
		
		#ifdef BROADCAST_SPI_DEBUG_1
		Serial.print(F("\tSending on pin: "));
		Serial.println(ss_pin);
		#endif

		if (length > BROADCAST_SOCKET_BUFFER_SIZE) {
			
			#ifdef BROADCAST_SPI_DEBUG_1
			Serial.println(F("\tlength > BROADCAST_SOCKET_BUFFER_SIZE"));
			#endif

			return false;
		}

		if (length > 0) {	// Don't send empty strings
			
			uint8_t c; // Avoid using 'char' while using values above 127

			for (uint8_t s = 0; size == 0 && s < 3; s++) {
		
				digitalWrite(ss_pin, LOW);
				delayMicroseconds(5);

				// Asks the Slave to start receiving
				c = _spi_instance->transfer(RECEIVE);

				if (c != VOID) {

					delayMicroseconds(12);	// Makes sure it's processed by the slave (12us) (critical path)
					c = _spi_instance->transfer(_sending_buffer[0]);

					if (c == READY) {	// Makes sure the Slave it's ready first
					
						for (uint8_t i = 1; i < length; i++) {
							delayMicroseconds(send_delay_us);
							c = _spi_instance->transfer(_sending_buffer[i]);	// Receives the echoed _sending_buffer[i - 1]
							if (c < 128) {
								// Offset of 2 picks all mismatches than an offset of 1
								if (i > 1 && c != _sending_buffer[i - 2]) {
									#ifdef BROADCAST_SPI_DEBUG_1
									Serial.print(F("\t\tERROR: Char mismatch at index: "));
									Serial.println(i - 2);
									#endif
									break;
								}
							} else if (c == FULL) {
								#ifdef BROADCAST_SPI_DEBUG_1
								Serial.println(F("\t\tERROR: Slave buffer overflow"));
								#endif
							} else {
								#ifdef BROADCAST_SPI_DEBUG_1
								Serial.print(F("\t\tERROR: Not an ASCII char at loop: "));
								Serial.println(i);
								#endif
								break;
							}
						}
						// Checks the last 2 chars still to be checked
						delayMicroseconds(12);    // Makes sure the Status Byte is sent
						c = _spi_instance->transfer(LAST);
						if (c == _sending_buffer[length - 2]) {
							delayMicroseconds(12);    // Makes sure the Status Byte is sent
							c = _spi_instance->transfer(END);
							if (c == _sending_buffer[length - 1]) {	// Last char
								size = length + 1;	// Just for error catch
								// Makes sure Slave does the respective sets
								for (uint8_t end_r = 0; c != DONE && end_r < 3; end_r++) {	// Makes sure the receiving buffer of the Slave is deleted, for sure!
									delayMicroseconds(10);
									c = _spi_instance->transfer(END);
								}
								#ifdef BROADCAST_SPI_DEBUG_1
								Serial.println(F("\t\tSend completed"));
								#endif
							} else {
								#ifdef BROADCAST_SPI_DEBUG_1
								Serial.print(F("\t\tERROR: Last char mismatch at index: "));
								Serial.println(length - 1);
								#endif
							}
						} else {
							#ifdef BROADCAST_SPI_DEBUG_1
							Serial.print(F("\t\tERROR: Penultimate Char mismatch at index: "));
							Serial.println(length - 2);
							#endif
						}
					} else if (c == BUSY) {
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tBUSY: Slave is busy, waiting a little."));
						#endif
						if (s < 2) {
							delay(2);	// Waiting 2ms
						}
					} else if (c == ERROR) {
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tERROR: Slave sent a transmission ERROR"));
						#endif
					} else if (c == RECEIVE) {
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tERROR: Received RECEIVE back, need to retry"));
						#endif
					} else {
						size = 1;	// Nothing to be sent
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.print(F("\t\tERROR: Device NOT ready wit the reply: "));
						Serial.println(c, HEX);
						#endif
					}

				} else {
					size = 1; // Avoids another try
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\t\tERROR: Received VOID"));
					#endif
				}

				if (size == 0) {
					delayMicroseconds(12);    // Makes sure the Status Byte is sent
					_spi_instance->transfer(ERROR);
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\t\t\tSent ERROR back to the Slave"));
					#endif
				}

				delayMicroseconds(5);
				digitalWrite(ss_pin, HIGH);

				if (size > 0) {
					#ifdef BROADCAST_SPI_DEBUG_1
					if (size > 1) {
						Serial.print("Sent message: ");
						Serial.write(_sending_buffer, length);
						Serial.println();
					} else {
						Serial.println("\tNothing sent");
					}
					#endif
				} else {
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.print("\t\tMessage NOT successfully sent on try: ");
					Serial.println(s + 1);
					#endif
				}
			}

        } else {
			#ifdef BROADCAST_SPI_DEBUG_1
			Serial.println(F("\t\tNothing to be sent"));
			#endif
			size = 1; // Nothing to be sent
		}

        if (size > 1) return true;
        return false;
    }


    uint8_t receiveSPI(int ss_pin) {
        uint8_t size = 0;	// No interrupts, so, not volatile
        uint8_t c; // Avoid using 'char' while using values above 127

		#ifdef BROADCAST_SPI_DEBUG_2
		Serial.print(F("\tReceiving on pin: "));
		Serial.println(ss_pin);
		#endif

        for (uint8_t r = 0; size == 0 && r < 3; r++) {
    
            digitalWrite(ss_pin, LOW);
            delayMicroseconds(5);

            // Asks the Slave to start receiving
            c = _spi_instance->transfer(SEND);
			
			if (c != VOID) {

				delayMicroseconds(12);	// Makes sure it's processed by the slave (12us) (critical path)
				c = _spi_instance->transfer('\0');   // Dummy char to get the ACK

				if (c == READY) {	// Makes sure the Slave it's ready first
					
					delayMicroseconds(receive_delay_us);
					c = _spi_instance->transfer('\0');   // Dummy char to get the ACK
					_receiving_buffer[0] = c;

					// Starts to receive all chars here
					for (uint8_t i = 1; c < 128 && i < BROADCAST_SOCKET_BUFFER_SIZE; i++) { // First i isn't a char byte
						delayMicroseconds(receive_delay_us);
						c = _spi_instance->transfer(_receiving_buffer[i - 1]);
						_receiving_buffer[i] = c;
						size = i;
					}
					if (c == LAST) {
						delayMicroseconds(receive_delay_us);    // Makes sure the Status Byte is sent
						c = _spi_instance->transfer(_receiving_buffer[size]);  // Replies the last char to trigger END in return
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tReceived LAST"));
						#endif
						if (c == END) {
							delayMicroseconds(10);	// Makes sure the Status Byte is sent
							c = _spi_instance->transfer(END);	// Replies the END to confirm reception and thus Slave buffer deletion
							for (uint8_t end_s = 0; c != DONE && end_s < 3; end_s++) {	// Makes sure the sending buffer of the Slave is deleted, for sure!
								delayMicroseconds(10);
								c = _spi_instance->transfer(END);
							}
							#ifdef BROADCAST_SPI_DEBUG_1
							Serial.println(F("\t\tReceive completed"));
							#endif
							size += 1;	// size equivalent to 'i + 2'
						} else {
							size = 0;	// Try again
							#ifdef BROADCAST_SPI_DEBUG_1
							Serial.println(F("\t\tERROR: END NOT received"));
							#endif
						}
					} else if (size == BROADCAST_SOCKET_BUFFER_SIZE) {
						delayMicroseconds(12);    // Makes sure the Status Byte is sent
						_spi_instance->transfer(FULL);
						size = 1;	// Try no more
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tFULL: Master buffer overflow"));
						#endif
					} else {
						size = 0;	// Try again
						#ifdef BROADCAST_SPI_DEBUG_1
						Serial.println(F("\t\tERROR: Receiving sequence wasn't followed"));
						#endif
					}
				} else if (c == NONE) {
					size = 1; // Nothing received
					#ifdef BROADCAST_SPI_DEBUG_2
					Serial.println(F("\t\tThere is nothing to be received"));
					#endif
				} else if (c == ERROR) {
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\t\tERROR: Transmission ERROR received from Slave"));
					#endif
				} else if (c == SEND) {
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\t\tERROR: Received SEND back, need to retry"));
					#endif
				} else if (c == FULL) {
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\t\tERROR: Slave buffer overflow"));
					#endif
				} else {
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.print(F("\t\tERROR: Device NOT ready, received status message: "));
					Serial.println(c, HEX);
					#endif
					size = 1; // Nothing received
				}

				if (size == 0) {
					delayMicroseconds(12);    // Makes sure the Status Byte is sent
					_spi_instance->transfer(ERROR);    // Results from ERROR or NACK send by the Slave and makes Slave reset to NONE
					#ifdef BROADCAST_SPI_DEBUG_1
					Serial.println(F("\t\t\tSent ERROR back to the Slave"));
					#endif
				}

			} else {
				#ifdef BROADCAST_SPI_DEBUG_1
				Serial.println(F("\t\tReceived VOID"));
				#endif
				size = 1; // Avoids another try
			}

            delayMicroseconds(5);
            digitalWrite(ss_pin, HIGH);

            if (size > 0) {
                #ifdef BROADCAST_SPI_DEBUG_1
                if (size > 1) {
                    Serial.print("Received message: ");
					Serial.write(_receiving_buffer, size - 1);
                    Serial.println();
                } else {
                	#ifdef BROADCAST_SPI_DEBUG_2
                    Serial.println("\tNothing received");
                	#endif
                }
                #endif
            } else {
                #ifdef BROADCAST_SPI_DEBUG_1
                Serial.print("\t\tMessage NOT successfully received on try: ");
                Serial.println(r + 1);
                #endif
            }
        }

        if (size > 0) size--;
        return size;
    }


    bool acknowledgeSPI(int ss_pin) {
        uint8_t c; // Avoid using 'char' while using values above 127
        bool acknowledge = false;

		#ifdef BROADCAST_SPI_DEBUG_1
		Serial.print(F("\tAcknowledging on pin: "));
		Serial.println(ss_pin);
		#endif

        for (uint8_t a = 0; !acknowledge && a < 3; a++) {
    
            digitalWrite(ss_pin, LOW);
            delayMicroseconds(5);

            // Asks the Slave to acknowledge readiness
            c = _spi_instance->transfer(ACK);

			if (c != VOID) {

				delayMicroseconds(12);
				c = _spi_instance->transfer(ACK);  // When the response is collected
				
				if (c == ACK) {
                	#ifdef BROADCAST_SPI_DEBUG_1
                	Serial.println(F("\t\tAcknowledged"));
					#endif
					acknowledge = true;
				}
				#ifdef BROADCAST_SPI_DEBUG_1
				else {
					Serial.println(F("\t\tNOT acknowledged"));
				}
				#endif
			}
            #ifdef BROADCAST_SPI_DEBUG_1
			else {
                Serial.println(F("\t\tReceived VOID"));
			}
			#endif

            delayMicroseconds(5);
            digitalWrite(ss_pin, HIGH);
        }

        #ifdef BROADCAST_SPI_DEBUG_1
        if (acknowledge) {
            Serial.println(F("Slave is ready!"));
        } else {
            Serial.println(F("Slave is NOT ready!"));
        }
        #endif

        return acknowledge;
    }


	// Allows the overriding class to peek at the received JSON message
	bool receivedJsonMessage(JsonMessage& new_json_message) override {

		if (BroadcastSocket::receivedJsonMessage(new_json_message)) {

			#ifdef BROADCAST_SPI_DEBUG
			Serial.print(F("\tcheckJsonMessage1: FROM name: "));
			Serial.println(new_json_message.get_from_name());
			Serial.print(F("\tcheckJsonMessage2: Saved actual named pin: "));
			Serial.println(_actual_ss_pin);
			#endif

			// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
			_named_pins_table.add(new_json_message.get_from_name(), _actual_ss_pin);

			#ifdef BROADCAST_SPI_DEBUG
			if (_named_pins_doc.isNull()) {
				Serial.println("\t\tERROR: The JsonDocument isn't initiated (still null)");
			}
			Serial.print(F("\tcheckJsonMessage3: Confirmed actual named pin: "));
			// Serial.println(_named_pins[from_name].as<uint8_t>());
			#endif

			return true;
		}
		return false;
	}

    
    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    bool send(const JsonMessage& new_json_message) override {

		if (_initiated && BroadcastSocket::send(new_json_message)) {	// Very important pre processing !!
			
			#ifdef BROADCAST_SPI_DEBUG
			Serial.print(F("\tsend1: Sent message: "));
			Serial.write(_sending_buffer, _sending_length);
			Serial.println();
			Serial.print(F("\tsend2: Sent length: "));
			Serial.println(_sending_length);
			#endif
			
			#ifdef ENABLE_DIRECT_ADDRESSING

			// *************** PARALLEL DEVELOPMENT WITH JSONMESSAGE (DONE) ***************
			bool as_reply = new_json_message.has_to_name();
			if (as_reply) {

				#ifdef BROADCAST_SPI_DEBUG
				Serial.println(F("\tsend3: json_message TO is a String"));
				#endif

				as_reply = _named_pins_table.get_pin(new_json_message.get_to_name(), &_actual_ss_pin);
			} else {
				#ifdef BROADCAST_SPI_DEBUG
				Serial.println(F("\tsend3: json_message TO is NOT a String or doesn't exist"));
				#endif
			}


			if (as_reply) {
				sendSPI(_sending_length, _actual_ss_pin);

				#ifdef BROADCAST_SPI_DEBUG
				Serial.print(F("\tsend4: --> Directly sent for the received pin --> "));
				Serial.println(_actual_ss_pin);
				#endif

			} else {    // Broadcast mode
				for (uint8_t ss_pin_i = 0; ss_pin_i < _ss_pins_count; ss_pin_i++) {
					sendSPI(_sending_length, _ss_pins[ss_pin_i]);
				}
				
				#ifdef BROADCAST_SPI_DEBUG
				Serial.println(F("\tsend4: --> Broadcast sent to all pins -->"));
				#endif

			}
			#else
			for (uint8_t ss_pin_i = 0; ss_pin_i < _ss_pins_count; ss_pin_i++) {
				sendSPI(_sending_length, _ss_pins[ss_pin_i]);
			}

			#ifdef BROADCAST_SPI_DEBUG
			Serial.println(F("\tsend4: --> Broadcast sent to all pins -->"));
			#endif

			#endif

			_sending_length = 0;	// Marks sending buffer available

			return true;
		}
        return false;
    }

	
    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    uint8_t receive() override {

		if (_initiated) {

			// Need to call homologous method in super class first
			uint8_t length = BroadcastSocket::receive(); // Very important to do or else it may stop receiving !!

			for (uint8_t ss_pin_i = 0; ss_pin_i < _ss_pins_count; ss_pin_i++) {
				length = receiveSPI(_ss_pins[ss_pin_i]);
				if (length > 0) {
					
					#ifdef BROADCAST_SPI_DEBUG
					Serial.print(F("\treceive1: Received message: "));
					Serial.write(_receiving_buffer, length);
					Serial.println();
					Serial.print(F("\treceive2: Received length: "));
					Serial.println(length);
					Serial.print(F("\t\t"));
					Serial.print(class_name());
					Serial.print(F(" is triggering the talkers with the received message from the SS pin: "));
					Serial.println(_ss_pins[ss_pin_i]);
					#endif

					_actual_ss_pin = static_cast<uint8_t>(_ss_pins[ss_pin_i]);
					_received_length = length;
					triggerTalkers();
				}
			}
			// Makes sure the _receiving_buffer is deleted with 0
			_received_length = 0;
		}
        return 0;   // Receives are all called internally in this method
    }


	bool initiate() {
		
		if (_spi_instance) {

			// Configure SPI settings
			_spi_instance->setDataMode(SPI_MODE0);
			_spi_instance->setBitOrder(MSBFIRST);  // EXPLICITLY SET MSB FIRST!
			_spi_instance->setFrequency(4000000); 	// 4MHz if needed (optional)
			// ====================================================
			
			// ================== CONFIGURE SS PINS ==================
			// CRITICAL: Configure all SS pins as outputs and set HIGH
			for (uint8_t i = 0; i < _ss_pins_count; i++) {
				pinMode(_ss_pins[i], OUTPUT);
				digitalWrite(_ss_pins[i], HIGH);
				delayMicroseconds(10); // Small delay between pins
			}

			_initiated = true;
			for (uint8_t ss_pin_i = 0; ss_pin_i < _ss_pins_count; ss_pin_i++) {
				if (!acknowledgeSPI(_ss_pins[ss_pin_i])) {
					_initiated = false;
					break;
				}
			}
			// _initiated is true at this point
			// deserializeJson() - loads from buffer (makes it an object)
			// to<JsonObject>() - creates empty object
			// to<JsonArray>() - creates empty array
			_named_pins = _named_pins_doc.to<JsonObject>();	// Convert null → empty object
		}

		#ifdef BROADCAST_SPI_DEBUG
		if (_initiated) {
			Serial.print(class_name());
			Serial.println(": initiate1: Socket initiated!");

			Serial.print(F("\tinitiate2: Total SS pins connected: "));
			Serial.println(_ss_pins_count);
			Serial.print(F("\t\tinitiate3: SS pins: "));
			
			for (uint8_t ss_pin_i = 0; ss_pin_i < _ss_pins_count; ss_pin_i++) {
				Serial.print(_ss_pins[ss_pin_i]);
				Serial.print(F(", "));
			}
			Serial.println();
		} else {
			Serial.println("initiate1: Socket NOT initiated!");
		}
	
		#endif

		return _initiated;
	}

public:

    // Move ONLY the singleton instance method to subclass
    static SPI_ESP_Arduino_Master& instance(
		int* ss_pins, uint8_t ss_pins_count, JsonTalker* const* const json_talkers, uint8_t talker_count, SourceValue source_value = SourceValue::REMOTE
	) {
        static SPI_ESP_Arduino_Master instance(ss_pins, ss_pins_count, json_talkers, talker_count, source_value);

        return instance;
    }

    const char* class_name() const override { return "SPI_ESP_Arduino_Master"; }


    virtual void begin(SPIClass* spi_instance) {
		
		_spi_instance = spi_instance;
		initiate();
    }
};



#endif // SPI_ESP_ARDUINO_MASTER_VSPI_HPP
