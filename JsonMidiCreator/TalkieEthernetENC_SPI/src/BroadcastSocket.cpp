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


#include "BroadcastSocket.h"  // MUST include the full definition!


char BroadcastSocket::_receiving_buffer[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};
char BroadcastSocket::_sending_buffer[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};

uint8_t BroadcastSocket::_talker_count = 0;
bool BroadcastSocket::_control_timing = false;
uint32_t BroadcastSocket::_last_local_time = 0;
uint32_t BroadcastSocket::_last_remote_time = 0;
uint16_t BroadcastSocket::_drops_count = 0;
uint8_t BroadcastSocket::_max_delay_ms = 5;

