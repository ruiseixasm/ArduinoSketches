/*
 WiFi Web Server LED Blink

 A simple web server that lets you blink an LED via the web.
 This sketch will print the IP address of your WiFi Shield (once connected)
 to the Serial monitor. From there, you can open that address in a web browser
 to turn on and off the LED on pin 5.

 If the IP address of your shield is yourAddress:
 http://yourAddress/H turns the LED on
 http://yourAddress/L turns it off

 This example is written for a network using WPA encryption. For
 WEP or WPA, change the Wifi.begin() call accordingly.

 Circuit:
 * WiFi shield attached
 * LED attached to pin 5

 created for arduino 25 Nov 2012
 by Tom Igoe

ported for sparkfun esp32 
31.01.2017 by Jan Hendrik Berlin
 
 */

//  Pins Configuration
//  D27 SERVO
//  D26 BUTTON
//  D25 RED
//  D33 GREEN
//  D32 BUZZER

#include <WiFi.h>
// Include the ESP32 Arduino Servo Library instead of the original Arduino Servo Library
#include <ESP32_Servo.h> 

const char* ssid     = "WiFi";
const char* password = "password";

Servo myservo;  // create servo object to control a servo

// Possible PWM GPIO pins on the ESP32: 0(used by on-board button),2,4,5(used by on-board LED),12-19,21-23,25-27,32-33 
int servoPin = 27;      // GPIO pin used to connect the servo control (digital OUT) 
int buttonPin = 26;     // GPIO pin used to connect the button (digital IN) 
int redPin = 25;        // GPIO pin used to connect the red led (digital OUT) 
int greenPin = 33;      // GPIO pin used to connect the green led (digital OUT) 
int buzzerPin = 32;     // GPIO pin used to connect the buzzer (digital OUT)

// Set min and max positions of the Servo
int positionMin = 0;
int positionMax = 120;
int stepDelay = 15;

// Controls button time press to avoid false triggers
unsigned long lastTime;

//// Possible ADC pins on the ESP32: 0,2,4,12-15,32-39; 34-39 are recommended for analog input
//int potPin = 34;        // GPIO pin used to connect the potentiometer (analog in)
//int ADC_Max = 4096;     // This is the default ADC max value on the ESP32 (12 bit ADC width);
//                        // this width can be set (in low-level oode) from 9-12 bits, for a
//                        // a range of max values of 512-4096

WiFiServer server(80);
// Set your Static IP address
IPAddress local_IP(192, 168, 31, 132);
// Set your Gateway IP address
IPAddress gateway(192, 168, 31, 77);
IPAddress subnet(255, 255, 255, 0);

//IPAddress primaryDNS(8, 8, 8, 8);   //optional
//IPAddress secondaryDNS(8, 8, 4, 4); //optional

void setup()
{
    Serial.begin(115200);
    pinMode(buttonPin, INPUT);    // set the BUTTON pin mode
    pinMode(redPin, OUTPUT);      // set the RED LED pin mode
    pinMode(greenPin, OUTPUT);    // set the GREEN LED pin mode
    pinMode(buzzerPin, OUTPUT);   // set the BUZZER pin mode

    // Configures static IP address
    if (!WiFi.config(local_IP, gateway, subnet)) {
      Serial.println("STA Failed to configure");
    }
    
    delay(10);

    // We start by connecting to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    server.begin();

    digitalWrite(redPin, LOW);                // GET /L turns the RED LED off
    digitalWrite(greenPin, HIGH);             // GET /L turns the GREEN LED on
    digitalWrite(buzzerPin, LOW);             // GET /L turns the BUZZER off

          myservo.attach(servoPin);              // attaches the servo on pin for RDS3128 to the servo object
                                                 // which are the defaults, so this line could be
                                                 // "myservo.attach(servoPin);" OR "myservo.attach(servoPin, 500, 2500);"

          myservo.write(positionMin);    // set the servo position according to the scaled value (value between 0 and 180)

    lastTime = millis();
}

void loop(){
 WiFiClient client = server.available();   // listen for incoming clients

  if (digitalRead(buttonPin) == HIGH) {
    if (abs(millis() - lastTime) > 500) {
          openDoor();
          lastTime = millis();
    }
  } else {
    lastTime = millis();
  }

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/H\">here</a> to turn the LED on pin 5 on.<br>");
            client.print("Click <a href=\"/L\">here</a> to turn the LED on pin 5 off.<br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {

          openDoor();
          
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(redPin, LOW);                // GET /L turns the RED LED off
          digitalWrite(greenPin, HIGH);             // GET /L turns the GREEN LED on
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
    
    lastTime = millis();
  }
}

void openDoor(){
                                                 
          digitalWrite(redPin, HIGH);                  // GET /H turns the RED LED on
          digitalWrite(greenPin, LOW);                 // GET /L turns the GREEN LED off

          digitalWrite(buzzerPin, HIGH);               // GET /H turns the BUZZER on
          delay(1000);
          digitalWrite(buzzerPin, LOW);                // GET /H turns the BUZZER off

          for(int posDegrees = positionMin; posDegrees <= positionMax; posDegrees++) {
              myservo.write(posDegrees);    // set the servo position according to the scaled value (value between 0 and 180)
              Serial.println(myservo.read());
         
              delay(stepDelay);
          }
      
          for(int posDegrees = positionMax; posDegrees >= positionMin; posDegrees--) {
            
              myservo.write(posDegrees);    // set the servo position according to the scaled value (value between 0 and 180)
              Serial.println(posDegrees);
              delay(stepDelay);
          }

//          myservo.detach();

          // To avoid overheating
          myservo.write(positionMin);    // set the servo position according to the scaled value (value between 0 and 180)
          delay(2500);
          
//          //Releases signal
//          pinMode(servoPin, OUTPUT);                 // set the SERVO pin mode
//          digitalWrite(servoPin, LOW);              // GET /L turns the SERVO off
          
          digitalWrite(redPin, LOW);                // GET /L turns the RED LED off
          digitalWrite(greenPin, HIGH);             // GET /L turns the GREEN LED on
}
