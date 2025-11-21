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

#define SOURCE_LIBRARY_MODE 1
//      0 - Arduino Library
//      1 - Project Library
//      2 - Arduino Copy Library


#if SOURCE_LIBRARY_MODE == 0
#include <JsonTalkie.hpp>
#include <sockets/BroadcastSocket_ESP8266.hpp>

#elif SOURCE_LIBRARY_MODE == 1
#include "src/JsonTalkie.hpp"
#include "src/sockets/BroadcastSocket_ESP8266.hpp"

#elif SOURCE_LIBRARY_MODE == 2
#include <Copy_JsonTalkie.hpp>
#include <sockets/BroadcastSocket_ESP8266.hpp>
#endif


JsonTalkie json_talkie_1;
JsonTalkie json_talkie_2;
auto& broadcast_socket = BroadcastSocket_ESP8266::instance();
WiFiUDP udp;

const char ssid[] = "wifiName";
const char password[] = "wifiPassword";


// Network settings
#define PORT 5005   // UDP port



// MANIFESTO DEFINITION

// Define the commands (stored in RAM)
JsonTalkie::Device device_1 = {
    "ESP66_1", "I do a 500ms buzz for a given duration!"
};
JsonTalkie::Device device_2 = {
    "ESP66_2", "I turn light On and Off!"
};

bool buzz(JsonObject json_message);
bool led_on(JsonObject json_message);
bool led_off(JsonObject json_message);
JsonTalkie::Run runCommands_1[] = {
    {"buzz", "Triggers buzzing", buzz}
};
JsonTalkie::Run runCommands_2[] = {
    {"on", "Turns led On", led_on},
    {"off", "Turns led Off", led_off}
};

bool set_duration(JsonObject json_message, long duration);
JsonTalkie::Set setCommands[] = {
    {"duration", "Sets duration", set_duration}
};

long get_total_runs(JsonObject json_message);
long get_duration(JsonObject json_message);
JsonTalkie::Get getCommands_1[] = {
    {"total_runs", "Gets the total number of runs", get_total_runs},
    {"duration", "Gets duration", get_duration}
};
JsonTalkie::Get getCommands_2[] = {
    {"total_runs", "Gets the total number of runs", get_total_runs}
};

bool process_response(JsonObject json_message);


// MANIFESTO DECLARATION

JsonTalkie::Manifesto manifesto_1(
    &device_1,
    runCommands_1, sizeof(runCommands_1)/sizeof(JsonTalkie::Run),
    setCommands, sizeof(setCommands)/sizeof(JsonTalkie::Set),
    getCommands_1, sizeof(getCommands_1)/sizeof(JsonTalkie::Get),
    process_response, nullptr
);

JsonTalkie::Manifesto manifesto_2(
    &device_2,
    runCommands_2, sizeof(runCommands_2)/sizeof(JsonTalkie::Run),
    nullptr, 0,
    getCommands_2, sizeof(getCommands_2)/sizeof(JsonTalkie::Get),
    process_response, nullptr
);

// END OF MANIFESTO



// Buzzer pin
#define buzzer_pin 2    // D2 (GPIO 4) is a fully available port on ESP8266

void setup() {
    // Serial is a singleton class (can be began multiple times)
    Serial.begin(9600);
    while (!Serial);
    
    delay(2000);    // Just to give some time to Serial

    // Saving string in PROGMEM (flash) to save RAM memory
    Serial.println("\n\nOpening the Socket...");
    
    WiFi.begin(ssid, password);
    
    Serial.print("\n\nConnecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.print("\nIP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Broadcast: ");
    Serial.println(WiFi.localIP());

    if (!udp.begin(PORT)) {
        Serial.println("Failed to start UDP");
        while(1);
    }

    Serial.println("\n\nOpening the Socket...");
    broadcast_socket.set_port(5005);    // By default is already 5005
    broadcast_socket.set_udp(&udp);

    Serial.println("Setting JsonTalkie devices...");
    json_talkie_1.set_manifesto(&manifesto_1);
    json_talkie_1.plug_socket(&broadcast_socket);
    json_talkie_2.set_manifesto(&manifesto_2);
    json_talkie_2.plug_socket(&broadcast_socket);


    Serial.println("Talker ready");

    pinMode(buzzer_pin, OUTPUT);
    digitalWrite(buzzer_pin, LOW);
    delay(10);
    digitalWrite(buzzer_pin, HIGH);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    delay(20);
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.println("Sending JSON...");
}

void loop() {
    json_talkie_1.listen();
    json_talkie_2.listen(false);    // Doesn't call socket receive, processes previous receive instead!

    static unsigned long lastSend = 0;
    if (millis() - lastSend > 39000) {

        // AVOID GLOBAL JSONDOCUMENT VARIABLES, HIGH RISK OF MEMORY LEAKS
        #if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument message_doc;
        if (message_doc.overflowed()) {
            Serial.println("CRITICAL: Insufficient RAM");
        } else {
            JsonObject message = message_doc.to<JsonObject>();
            message["m"] = 0;   // talk
            json_talkie_1.talk(message);
        }
        #else
        StaticJsonDocument<BROADCAST_SOCKET_BUFFER_SIZE> message_doc;
        if (message_doc.capacity() < BROADCAST_SOCKET_BUFFER_SIZE) {  // Absolute minimum
            Serial.println("CRITICAL: Insufficient RAM");
        } else {
            JsonObject message = message_doc.to<JsonObject>();
            message["m"] = 0;   // talk
            json_talkie_1.talk(message);
        }
        #endif
        
        lastSend = millis();
    }
}


long total_runs = 0;
long _duration = 5;  // Example variable

// Command implementations
bool buzz(JsonObject json_message) {
    #ifndef BROADCASTSOCKET_SERIAL
    digitalWrite(buzzer_pin, LOW);
    delay(_duration); 
    digitalWrite(buzzer_pin, HIGH);
    #endif
    total_runs++;
    json_message["r"] = "Buzzed!";
    json_talkie_1.talk(json_message);
    return true;
}


bool is_led_on = false;  // keep track of state yourself, by default it's off

bool led_on(JsonObject json_message) {
    if (!is_led_on) {
        digitalWrite(LED_BUILTIN, LOW);
        is_led_on = true;
        total_runs++;
    } else {
        json_message["r"] = "Already On!";
        json_talkie_2.talk(json_message);
        return false;
    }
    return true;
}

bool led_off(JsonObject json_message) {
    if (is_led_on) {
        digitalWrite(LED_BUILTIN, HIGH);
        is_led_on = false;
        total_runs++;
    } else {
        json_message["r"] = "Already Off!";
        json_talkie_2.talk(json_message);
        return false;
    }
    return true;
}


bool set_duration(JsonObject json_message, long duration) {
    (void)json_message; // Silence unused parameter warning
    _duration = duration;
    return true;
}

long get_duration(JsonObject json_message) {
    (void)json_message; // Silence unused parameter warning
    return _duration;
}

long get_total_runs(JsonObject json_message) {
    (void)json_message; // Silence unused parameter warning
    return total_runs;
}


bool process_response(JsonObject json_message) {
    Serial.print(json_message["f"].as<String>());
    Serial.print(" - ");
    if (json_message["r"].is<String>()) {
        Serial.println(json_message["r"].as<String>());
    } else if (json_message["d"].is<String>()) {
        Serial.println(json_message["d"].as<String>());
    } else {
        Serial.println("Empty echo received!");
    }
    return false;
}

