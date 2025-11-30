// SPI Slave Code - Pure String Commands
#include <SPI.h>
#include <ArduinoJson.h>    // Include ArduinoJson Library


enum MessageCode : uint8_t {
    START   = 0xF0, // Start of transmission
    END     = 0xF1, // End of transmission
    ACK     = 0xF2, // Acknowledge
    NACK    = 0xF3, // Not acknowledged
    READY   = 0xF4, // Slave has response ready
    ERROR   = 0xF5, // Error frame
    RECEIVE = 0xF6, // Asks the receiver to start receiving
    SEND    = 0xF7, // Asks the receiver to start sending
    NONE    = 0xF8  // Means nothing to send
};


// Pin definitions
const int GREEN_LED_PIN = 2;
const int YELLOW_LED_PIN = 21;  // A7 = digital pin 21
const int SS_PIN = 10;

#define BUFFER_SIZE 128
char _receiving_buffer[BUFFER_SIZE] = {'\0'};
char _sending_buffer[BUFFER_SIZE] = {'\0'};

volatile uint8_t _receiving_index = 0;
volatile uint8_t _sending_index = 0;

volatile bool _receiving_state = false;
volatile bool _sending_state = false;
volatile bool _process_message = false;


void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\nSPI Slave Ready - Half-Duplex Mode");

    Serial.print("LED_BUILTIN: ");  // Led 13 is already used by SCK
    Serial.println(LED_BUILTIN);

    pinMode(GREEN_LED_PIN, OUTPUT);
    digitalWrite(GREEN_LED_PIN, LOW);
    pinMode(YELLOW_LED_PIN, OUTPUT);
    digitalWrite(YELLOW_LED_PIN, LOW);

    pinMode(MISO, OUTPUT);  // MISO must be OUTPUT for Slave to send data!

    // Initialize SPI as slave - EXPLICIT MSB FIRST
    SPCR = 0;  // Clear register
    SPCR |= _BV(SPE);    // SPI Enable
    SPCR |= _BV(SPIE);   // SPI Interrupt Enable  
    SPCR &= ~_BV(DORD);  // MSB First (DORD=0 for MSB first)
    SPCR &= ~_BV(CPOL);  // Clock polarity 0
    SPCR &= ~_BV(CPHA);  // Clock phase 0 (MODE0)

    // // Initialize SPI as slave
    // SPCR |= _BV(SPE);
    // SPCR |= _BV(SPIE);

    SPI.attachInterrupt();  
}



// ------------------------------
// SPI Interrupt
// ------------------------------
ISR(SPI_STC_vect) {

    // AVOID PLACING HEAVY CODE OR CALL HERE. THIS INTERRUPTS THE LOOP!

    // WARNING:
    //     AVOID PLACING Serial.print CALLS HERE BECAUSE IT WILL DELAY 
    //     THE POSSIBILITY OF SPI CAPTURE AND RESPONSE IN TIME !!!

    uint8_t c = SPDR;    // Avoid using 'char' while using values above 127

    if (c < 128) {  // If it's a typical ASCII char

        if (_receiving_state) {
            if (_receiving_index < BUFFER_SIZE) {
                _receiving_buffer[_receiving_index++] = c;
                // Returns same received char as receiving confirmation (no need to set SPDR)
            } else {
                _receiving_state = false;
                SPDR = ERROR;
            }
        } else if (_sending_state) {
            if (_sending_index > 1 && c != _sending_buffer[_sending_index - 2]) {  // Two messages delay
                _sending_state = false;
                SPDR = ERROR;
            } else if (_sending_buffer[_sending_index - 1] == '\0') {	// Has to send '\0' in order to its previous char be checked
                _sending_state = false;
                _sending_buffer[0] = '\0';   // Makes sure the sending buffer is marked as empty
                SPDR = END;     // Nothing more to send (spares extra send, '\0' implicit)
            } else if (_sending_index < BUFFER_SIZE) {
                SPDR = _sending_buffer[_sending_index++];
            } else {
                _sending_state = false;
                SPDR = ERROR;
            }
        } else {
            SPDR = NACK;
        }

    } else {    // It's a control message 0xFX
        
        switch (c) {
            case RECEIVE:
                _receiving_state = true;
                _receiving_index = 0;
                SPDR = ACK;
                break;
            case SEND:
                _sending_index = 0;
                if (_sending_buffer[_sending_index] == '\0') {
                    SPDR = NONE;    // Nothing to send
                } else {    // Starts sending right away, so, no ACK
                    SPDR = _sending_buffer[_sending_index++];
                    _sending_state = true;
                }
                break;
            case END:
                _receiving_state = false;
                _process_message = true;
                SPDR = ACK;
                break;
            case ERROR:
                _receiving_state = false;
                _process_message = false;
                _sending_state = false;
                SPDR = ACK;
                break;
            default:
                SPDR = NACK;
        }
    }
}


void processMessage() {

    Serial.print("Processed command: ");
    Serial.println(_receiving_buffer);

    // JsonDocument in the stack makes sure its memory is released (NOT GLOBAL)
    #if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument message_doc;
    #else
    StaticJsonDocument<BROADCAST_SOCKET_BUFFER_SIZE> message_doc;
    #endif

    DeserializationError error = deserializeJson(message_doc, _receiving_buffer, BUFFER_SIZE);
    if (error) {
        return false;
    }
    JsonObject json_message = message_doc.as<JsonObject>();

    const char* command_name = json_message["n"].as<const char*>();


    if (strcmp(command_name, "ON") == 0) {
        digitalWrite(GREEN_LED_PIN, HIGH);
        json_message["n"] = "OK_ON";
        size_t length = serializeJson(json_message, _sending_buffer, BUFFER_SIZE);
        Serial.print("LED is ON");
        Serial.print(" | Sending: ");
        Serial.println(_sending_buffer);

    } else if (strcmp(command_name, "OFF") == 0) {
        digitalWrite(GREEN_LED_PIN, LOW);
        json_message["n"] = "OK_OFF";
        size_t length = serializeJson(json_message, _sending_buffer, BUFFER_SIZE);
        Serial.print("LED is OFF");
        Serial.print(" | Sending: ");
        Serial.println(_sending_buffer);
    } else {
        json_message["n"] = "BUZZ";
        size_t length = serializeJson(json_message, _sending_buffer, BUFFER_SIZE);
        Serial.print("Unknown command");
        Serial.print(" | Sending: ");
        Serial.println(_sending_buffer);
    }
}

void loop() {
    // HEAVY PROCESSING SHALL BE IN THE LOOP
    if (_process_message) {
        processMessage();   // Called only once!
        _process_message = false;    // Critical to avoid repeated calls over the ISR function
    }
}

