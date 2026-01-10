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

#include "BroadcastSocket_EtherCard.h"


JsonMessage BroadcastSocket_EtherCard::_json_message;
char* BroadcastSocket_EtherCard::_ptr_received_buffer = nullptr;

uint8_t BroadcastSocket_EtherCard::_source_ip[4] = {0};
size_t BroadcastSocket_EtherCard::_data_length = 0;
// ===== [SELF IP] =====
uint8_t BroadcastSocket_EtherCard::_local_ip[4] = {0};

