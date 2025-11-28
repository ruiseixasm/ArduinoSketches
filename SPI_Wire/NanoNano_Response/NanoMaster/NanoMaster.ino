// SPI Master Code - Pure String Commands
#include <SPI.h>

// Pin definitions
const int BUZZ_PIN = 2; // External BUZZER pin
const int SS_PIN = 10;  // Slave Select pin

#define BUFFER_SIZE 128
char receiving_buffer[BUFFER_SIZE];

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
  
  digitalWrite(SS_PIN, LOW);
  delayMicroseconds(20);
  
  // Send command
  int i = 0;
  while (command[i] != '\0') {
    SPI.transfer(command[i]);
    i++;
    delayMicroseconds(10);
  }
  SPI.transfer('\0');
  
  delayMicroseconds(50);

  // Receive response
  char c;
  for(size_t i = 0; i < BUFFER_SIZE; i++) {

    c = SPI.transfer(0xFF);  // Reads char by char
    receiving_buffer[i] = c;
    if (c == '\0') break;
  }
  
  digitalWrite(SS_PIN, HIGH);
  
  Serial.print("Sent: ");
  Serial.print(command);
  Serial.print(" | Received: ");
  Serial.println(receiving_buffer);
  
  // NEW CODE: Check if response is "BUZZ" and activate buzzer
  if (strcmp(receiving_buffer, "BUZZ") == 0) {
    digitalWrite(BUZZ_PIN, HIGH);
    delay(200);  // Buzzer on for 200ms
    digitalWrite(BUZZ_PIN, LOW);
    Serial.println("BUZZER activated for 200ms!");
  }
  
}

