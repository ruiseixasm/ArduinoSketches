// SPI Master Code
#include <SPI.h>

// Define commands
#define LED_ON  0x01
#define LED_OFF 0x00

// Pin definitions
const int BUZZ_PIN = 2; // External BUZZER pin
const int SS_PIN = 10;  // Slave Select pin

void setup() {
  // Initialize SPI
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);  // Set clock speed
  SPI.setDataMode(SPI_MODE0);            // SPI mode 0
  
  // Initialize Slave Select pin
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);  // Start with slave deselected
  
  // Initialize serial for debugging
  Serial.begin(9600);
  Serial.println("SPI Master Initialized");
}

void loop() {
  // Send LED ON command
  sendCommand(LED_ON);
  Serial.println("Sent: LED ON");
  delay(2000);  // Wait 2 seconds
  
  // Send LED OFF command
  sendCommand(LED_OFF);
  Serial.println("Sent: LED OFF");
  delay(2000);  // Wait 2 seconds
}

void sendCommand(byte command) {
  // Select the slave
  digitalWrite(SS_PIN, LOW);
  
  // Small delay for slave to ready itself
  delayMicroseconds(10);
  
  // Send command
  byte response = SPI.transfer(command);
  
  // Deselect slave
  digitalWrite(SS_PIN, HIGH);
  
  // Print response for debugging
  Serial.print("Slave response: ");
  Serial.println(response, HEX);
}

