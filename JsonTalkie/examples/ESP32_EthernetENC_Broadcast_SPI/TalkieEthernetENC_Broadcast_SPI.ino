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

// To upload a sketch to an ESP32, when the "......." appears press the button BOOT for a while

// LED_BUILTIN is already defined by ESP32 platform
// Typically GPIO2 for most ESP32 boards
#ifndef LED_BUILTIN
  #define LED_BUILTIN 2  // Fallback definition if not already defined
#endif

#include <JsonTalkie.hpp>
// ONLY THE CHANGED LIBRARY ALLOWS THE RECEPTION OF BROADCASTED UDP PACKAGES TO 255.255.255.255
#include "S_EthernetENC_Broadcast.hpp"
#include "S_Broadcast_SPI_ESP_Arduino_Master.hpp"
#include "M_Spy.hpp"
#include "M_LedManifesto.hpp"
#include "M_MessageTester.hpp"


// TALKERS 
// M_Spy Talker
const char t_spy_name[] = "spy";
const char t_spy_desc[] = "I'm a M_Spy and I spy the talkers' pings";
M_Spy spy_manifesto;
JsonTalker t_spy = JsonTalker(t_spy_name, t_spy_desc, &spy_manifesto);

// Talker (led)
const char l_led_name[] = "blue";
const char l_led_desc[] = "I turn led Blue on and off";
M_LedManifesto led_manifesto(LED_BUILTIN);
JsonTalker l_led = JsonTalker(l_led_name, l_led_desc, &led_manifesto);

// Talker (JsonMessage tester)
const char t_tester_name[] = "test";
const char t_tester_desc[] = "I test the JsonMessage class";
M_MessageTester message_tester;
JsonTalker t_tester = JsonTalker(t_tester_name, t_tester_desc, &message_tester);


// SOCKETS
// Singleton requires the & (to get a reference variable)
auto& ethernet_socket = S_EthernetENC_Broadcast::instance();
int spi_pins[] = {4, 16};
auto& spi_socket = S_Broadcast_SPI_ESP_Arduino_Master::instance(spi_pins, sizeof(spi_pins)/sizeof(int));


// SETTING THE REPEATER
BroadcastSocket* uplinked_sockets[] = { &ethernet_socket };
JsonTalker* downlinked_talkers[] = { &t_spy, &t_tester, &l_led };
BroadcastSocket* downlinked_sockets[] = { &spi_socket };
const MessageRepeater message_repeater(
		uplinked_sockets, sizeof(uplinked_sockets)/sizeof(BroadcastSocket*),
		downlinked_talkers, sizeof(downlinked_talkers)/sizeof(JsonTalker*),
		downlinked_sockets, sizeof(downlinked_sockets)/sizeof(BroadcastSocket*)
	);


EthernetENC_BroadcastUDP udp;

// uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};   // DEFAULT

// uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01};
// uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x02};
// uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x03};
// uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x04};

// uint8_t mac[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01};
// uint8_t mac[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x02};
uint8_t mac[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x03};
// uint8_t mac[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x04};
// uint8_t mac[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x05};

// uint8_t mac[] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x01};
// uint8_t mac[] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x02};
// uint8_t mac[] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x03};
// uint8_t mac[] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x04};
// uint8_t mac[] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x05};

// uint8_t mac[] = {0x02, 0xAA, 0xFA, 0xCE, 0x10, 0x01};
// uint8_t mac[] = {0x02, 0xAA, 0xFA, 0xCE, 0x10, 0x02};
// uint8_t mac[] = {0x02, 0xAA, 0xFA, 0xCE, 0x10, 0x03};
// uint8_t mac[] = {0x02, 0xAA, 0xFA, 0xCE, 0x10, 0x04};
// uint8_t mac[] = {0x02, 0xAA, 0xFA, 0xCE, 0x10, 0x05};


// Network settings
#define PORT 5005   // UDP port


void setup() {
    // Initialize pins FIRST before anything else
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); // Start with LED off
    
    // Then start Serial
    Serial.begin(115200);
    delay(2000); // Important: Give time for serial to initialize
    Serial.println("\n\n=== ESP32 with EthernetENC STARTING ===");

    // Add a small LED blink to confirm code is running
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    
    // STEP 1: Initialize Ethernet with CS pin
    const int CS_PIN = 5;  // Defines CS pin here (Enc28j60)
    Serial.println("Step 1: Initializing EthernetENC...");
	// This forces the Ethernet to use the default SPI
    Ethernet.init(CS_PIN);	// Uses global SPI (VSPI)
	// // As alternative it is possible to give a specific SPI
	// SPIClass* ethSPI = new SPIClass(HSPI);
	// ethSPI->begin(14, 12, 13, 15);  // SCK, MISO, MOSI, SS (dummy)
	// EthernetENC(uint8_t csPin, SPIClass* ethSPI)
    Serial.println("Ethernet initialized successfully");
    delay(500);

    // STEP 2: Begin Ethernet connection with DHCP
    Serial.println("Step 2: Starting Ethernet connection with DHCP...");
    if (Ethernet.begin(mac) == 0) {
        Serial.println("Failed to configure Ethernet using DHCP");
        // Optional: Fallback to static IP
        // Ethernet.begin(mac, IPAddress(192, 168, 1, 100));
        // while (Ethernet.localIP() == INADDR_NONE) {
        //     delay(1000);
        // }
    } else {
        Serial.println("DHCP successful!");
    }

    // Give Ethernet time to stabilize
    delay(1500);

    // STEP 3: Check connection status
    Serial.println("Step 3: Checking Ethernet status...");
    Serial.print("Local IP: ");
    Serial.println(Ethernet.localIP());
    Serial.print("Subnet Mask: ");
    Serial.println(Ethernet.subnetMask());
    Serial.print("Gateway IP: ");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("DNS Server: ");
    Serial.println(Ethernet.dnsServerIP());

    // STEP 4: Initialize UDP and broadcast socket
    Serial.println("Step 4: Initializing UDP...");
    if (udp.begin(PORT)) {
        Serial.println("UDP started successfully on port " + String(PORT));
    } else {
        Serial.println("Failed to start UDP!");
    }

    // STEP 5: Setting up broadcast sockets
    Serial.println("Step 5: Setting up broadcast sockets...");
	SPIClass* hspi = new SPIClass(HSPI);  // heap variable!
	// ================== INITIALIZE HSPI ==================
	// Initialize SPI with HSPI pins: SCK=14, MISO=12, MOSI=13, SS=15
	hspi->begin(14, 12, 13, 15);  // SCK, MISO, MOSI, SS
    spi_socket.begin(hspi);
    ethernet_socket.set_port(PORT);
    ethernet_socket.set_udp(&udp);

    // Final startup indication
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("Setup completed - Ready for JSON communication!");
}


void loop() {
    Ethernet.maintain();		// Maintain DHCP lease (important for long-running applications)
	message_repeater.loop();	// Keep calling the Repeater
}


// ESP32 wiring with the ENC28J60 (SPI)

//     MOSI (D23)  =   SI
//     MISO (D19)  =   SO
//     SCK  (D18)  =   SCK
//     SS   (D5)   =   CS
//     GND         =   GND
//     MUTUAL EXCLUSIVE:
//         VIN     =   5V
//         3V3     =   Q3


// ESP32 (3.3V)        →   Nano (5V)          Risk Level
// ─────────────────────────────────────────────────────
// ESP32 MOSI (D13)    →   Nano MOSI (D11)    SAFE (Nano input)
// ESP32 MISO (D12)    ←   Nano MISO (D12)    DANGEROUS! (Nano output=5V)
// ESP32 SCK  (D14)    →   Nano SCK  (D13)    SAFE (Nano input) (Also DANGEROUS due to LED_BUILTIN on pin 13)
// ESP32 SS   (D15)    →   Nano SS   (D10)    SAFE (Nano input)

// Use level shifter or resistors
//     ESP32 → Nano: Direct (3.3V → 5V input is fine)
//     Nano → ESP32: 1K series resistor (limits current to safe level)

// Direct connection often works:
//     ESP32 MOSI (3.3V) → Nano D11 (5V input)  // SAFE
//     ESP32 SCK  (3.3V) → Nano D13 (5V input)  // RISKY due to LED_BUILTIN on pin 13 
//     ESP32 SS   (3.3V) → Nano D10 (5V input)  // SAFE
//     Nano MISO  (5V)   → ESP32 MISO (3.3V)    // RISKY but usually survives

// Key Parameters:
//     Nano output: 5V, can source ~20mA max
//     ESP32 input: Has ESD protection diodes to 3.3V rail
//     Diode forward voltage: ~0.6V each

// Calculations:
//     V_resistor = V_nano - V_esp32 = 5V - 3.9V = 1.1V
//     I = V_resistor / R = 1.1V / 1000Ω = 1.1mA

