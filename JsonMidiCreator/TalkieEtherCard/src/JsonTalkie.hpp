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


/**
 * @file JsonTalkie.hpp
 * @brief This file aggregates all the JsonTalkie libraries by including them.
 * 
 * @author Rui Seixas Monteiro
 * @date Created: 2026-01-11
 * @version 4.0.0
 */

#ifndef JSON_TALKIE_HPP
#define JSON_TALKIE_HPP

#include <Arduino.h>        // Needed for Serial given that Arduino IDE only includes Serial in .ino files!
#include "TalkieCodes.hpp"
#include "JsonMessage.hpp"
#include "BroadcastSocket.h"
#include "TalkerManifesto.hpp"
#include "JsonTalker.h"
#include "MessageRepeater.hpp"


#endif // JSON_TALKIE_HPP
