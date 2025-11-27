// SPI Master Code - Pure String Commands
#include <SPI.h>

// Pin definitions
const int BUZZ_PIN = 2; // External BUZZER pin
const int SS_PIN = 10;  // Slave Select pin

void setup() {
  // Initialize SPI
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.setDataMode(SPI_MODE0);
  
  // Initialize pins
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);
  pinMode(BUZZ_PIN, OUTPUT);
  digitalWrite(BUZZ_PIN, LOW);
  
  // Initialize serial
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\nSPI Master Initialized - String Mode");
}

void loop() {
  // Send string commands directly
  sendString("LED_ON");
  delay(2000);
  
  sendString("LED_OFF");
  delay(2000);
}

void sendString(const char* command) {
  // Select slave
  digitalWrite(SS_PIN, LOW);
  delayMicroseconds(100);
  
  // Send each character of the string
  int i = 0;
  while (command[i] != '\0') {
    SPI.transfer(command[i]);
    i++;
    delayMicroseconds(10);
  }
  
  // Send null terminator to mark end of string
  SPI.transfer('\0');
  
  // Deselect slave
  digitalWrite(SS_PIN, HIGH);
  
  Serial.print("Sent: ");
  Serial.println(command);
  
  // Buzz feedback
  digitalWrite(BUZZ_PIN, HIGH);
  delay(5);
  digitalWrite(BUZZ_PIN, LOW);
}

