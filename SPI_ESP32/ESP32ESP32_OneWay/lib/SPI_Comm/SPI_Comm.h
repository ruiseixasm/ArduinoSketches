#ifndef SPI_COMM_H
#define SPI_COMM_H

#include <SPI.h>
#include <Arduino.h>

class SPI_Comm {
public:
    // Command codes
    enum Command : uint8_t {
        CMD_PING = 0x01,      // Check if slave is alive
        CMD_LED_ON = 0x02,    // Turn LED ON
        CMD_LED_OFF = 0x03,   // Turn LED OFF
        CMD_LED_TOGGLE = 0x04, // Toggle LED
        CMD_LED_STATUS = 0x05, // Get LED status
        CMD_ACK = 0x06,       // Acknowledge
        CMD_NACK = 0x07,      // Not acknowledge
        CMD_ERROR = 0x08      // Error
    };

    // Response codes
    enum Response : uint8_t {
        RESP_OK = 0x10,
        RESP_LED_ON = 0x11,
        RESP_LED_OFF = 0x12,
        RESP_ERROR = 0x13
    };

private:
    uint8_t _ss_pin;
    SPIClass* _spi;
    
public:
    // Constructor for Master
    SPI_Comm(uint8_t ss_pin, SPIClass* spi = &SPI) 
        : _ss_pin(ss_pin), _spi(spi) {
        
        pinMode(_ss_pin, OUTPUT);
        digitalWrite(_ss_pin, HIGH); // Deselect slave
    }

    // Constructor for Slave (no SS pin needed)
    SPI_Comm(SPIClass* spi = &SPI) 
        : _ss_pin(0), _spi(spi) {
        // Nothing to initialize for slave in this constructor
    }

    // Master: Initialize SPI
    void beginMaster() {
        _spi->begin();
        _spi->setDataMode(SPI_MODE0);
        _spi->setBitOrder(MSBFIRST);
        _spi->setFrequency(1000000); // 1 MHz
    }

    // Slave: Initialize SPI
    void beginSlave() {
        _spi->begin();
        // Set MISO as OUTPUT for slave
        pinMode(_spi->pinSS(), INPUT_PULLUP);
    }

    // Master: Send command and get response
    uint8_t sendCommand(uint8_t cmd, uint8_t data = 0) {
        digitalWrite(_ss_pin, LOW);
        delayMicroseconds(10);
        
        // Start SPI transaction
        _spi->beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
        
        // Send command
        uint8_t response1 = _spi->transfer(cmd);
        delayMicroseconds(10);
        
        // Send data
        uint8_t response2 = _spi->transfer(data);
        delayMicroseconds(10);
        
        _spi->endTransaction();
        
        digitalWrite(_ss_pin, HIGH);
        delayMicroseconds(10);
        
        return response2; // Return the actual response
    }

    // Process command (for Slave)
    uint8_t processCommand(uint8_t cmd, uint8_t data, uint8_t led_pin = 2) {  // Default to GPIO2
        switch(cmd) {
            case CMD_PING:
                return RESP_OK;
                
            case CMD_LED_ON:
                digitalWrite(led_pin, HIGH);
                return RESP_LED_ON;
                
            case CMD_LED_OFF:
                digitalWrite(led_pin, LOW);
                return RESP_LED_OFF;
                
            case CMD_LED_TOGGLE:
                digitalWrite(led_pin, !digitalRead(led_pin));
                return digitalRead(led_pin) ? RESP_LED_ON : RESP_LED_OFF;
                
            case CMD_LED_STATUS:
                return digitalRead(led_pin) ? RESP_LED_ON : RESP_LED_OFF;
                
            default:
                return RESP_ERROR;
        }
    }

    // Simple check if slave is responding
    bool ping() {
        uint8_t response = sendCommand(CMD_PING);
        return (response == RESP_OK);
    }

    // Control LED on slave
    bool ledOn() {
        uint8_t response = sendCommand(CMD_LED_ON);
        return (response == RESP_LED_ON);
    }

    bool ledOff() {
        uint8_t response = sendCommand(CMD_LED_OFF);
        return (response == RESP_LED_OFF);
    }

    bool ledToggle() {
        uint8_t response = sendCommand(CMD_LED_TOGGLE);
        return (response == RESP_LED_ON || response == RESP_LED_OFF);
    }

    bool getLedStatus() {
        uint8_t response = sendCommand(CMD_LED_STATUS);
        return (response == RESP_LED_ON);
    }

    // Getter for SS pin
    uint8_t getSSPin() const { return _ss_pin; }
};

#endif // SPI_COMM_H

