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

#include "JsonTalker.h"         // Includes the ArduinoJson Library
#include "BroadcastSocket.h"    // MUST include the full definition!


BroadcastSocket* JsonTalker::_socket = nullptr;
bool JsonTalker::_is_led_on = false;


void JsonTalker::set_delay(uint8_t delay) {
    return _socket->set_max_delay(delay);
}

uint8_t JsonTalker::get_delay() {
    return _socket->get_max_delay();
}

long JsonTalker::get_total_drops() {
    return _socket->get_drops_count();
}



bool JsonTalker::sendMessage(JsonObject json_message, bool as_reply) {
    if (_socket == nullptr) return false;
    json_message["f"] = _name;
    return _socket->sendMessage(json_message, as_reply);
}



