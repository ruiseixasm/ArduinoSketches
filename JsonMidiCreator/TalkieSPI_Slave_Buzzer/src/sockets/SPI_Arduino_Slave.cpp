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

#include "SPI_Arduino_Slave.h"


char SPI_Arduino_Slave::_received_buffer[TALKIE_BUFFER_SIZE];
char SPI_Arduino_Slave::_sending_buffer[TALKIE_BUFFER_SIZE];

volatile uint8_t SPI_Arduino_Slave::_receiving_index = 0;
volatile uint8_t SPI_Arduino_Slave::_received_length = 0;
volatile uint8_t SPI_Arduino_Slave::_sending_index = 0;
volatile uint8_t SPI_Arduino_Slave::_validation_index = 0;
volatile uint8_t SPI_Arduino_Slave::_sending_length = 0;
volatile SPI_Arduino_Slave::StatusByte SPI_Arduino_Slave::_transmission_mode 
									= SPI_Arduino_Slave::StatusByte::TALKIE_SB_NONE;


// Define ISR at GLOBAL SCOPE (outside the class)
ISR(SPI_STC_vect) {
    // You need a way to call your class method from here
    // Possibly using a static method or singleton pattern
    SPI_Arduino_Slave::handleSPI_Interrupt();
}

