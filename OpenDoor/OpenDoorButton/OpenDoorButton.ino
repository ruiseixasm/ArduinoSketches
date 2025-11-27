/*
Open Door Button 
 */

#include <WiFi.h>
#include <HTTPClient.h>
#define USE_SERIAL Serial

const char* ssid     = "WiFi";
const char* password = "password";

int buttonPin = 26;     // GPIO pin used to connect the button (digital IN) 
boolean switchStatus = false;

// Controls button time press to avoid false triggers
unsigned long lastTime;

void setup()
{
    Serial.begin(115200);
    pinMode(buttonPin, INPUT);    // set the BUTTON pin mode
    switchStatus = digitalRead(buttonPin);
    
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
    
    lastTime = millis();
}

void loop(){

  Serial.println(digitalRead(buttonPin));

  if (switchStatus != digitalRead(buttonPin)) {
    
    if (abs(millis() - lastTime) > 500) {
      
        HTTPClient http;

        USE_SERIAL.print("[HTTP] begin...\n");
        http.begin("http://192.168.31.132/H"); //HTTP

        USE_SERIAL.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCode will be negative on error
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                USE_SERIAL.println(payload);
            }
        } else {
            USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();

        // do a delay in order to not overload the openDoor board
        delay(4000);
        
        switchStatus = digitalRead(buttonPin); 

        lastTime = millis();
               
    }
    
  } else {
    
    lastTime = millis();
    
  }

}
