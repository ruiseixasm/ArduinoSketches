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
    uint8_t _spi_miso;
    uint8_t _spi_mosi;
    uint8_t _spi_sck;
    
    // Initialize SPI with custom pins
    void initSPI(uint8_t sck, uint8_t miso, uint8_t mosi) {
        SPI.begin(sck, miso, mosi);
        SPI.setDataMode(SPI_MODE0);
        SPI.setBitOrder(MSBFIRST);
        SPI.setFrequency(1000000); // 1 MHz
    }

public:
    // Constructor for Master
    SPI_Comm(uint8_t ss_pin, uint8_t sck = 18, uint8_t miso = 19, uint8_t mosi = 23) 
        : _ss_pin(ss_pin), _spi_sck(sck), _spi_miso(miso), _spi_mosi(mosi) {
        
        pinMode(_ss_pin, OUTPUT);
        digitalWrite(_ss_pin, HIGH); // Deselect slave
        
        initSPI(_spi_sck, _spi_miso, _spi_mosi);
    }

    // Constructor for Slave (no SS pin needed)
    SPI_Comm(uint8_t sck = 18, uint8_t miso = 19, uint8_t mosi = 23) 
        : _ss_pin(0), _spi_sck(sck), _spi_miso(miso), _spi_mosi(mosi) {
        
        initSPI(_spi_sck, _spi_miso, _spi_mosi);
        pinMode(miso, INPUT);
    }

    // Master: Send command and get response
    uint8_t sendCommand(uint8_t cmd, uint8_t data = 0) {
        digitalWrite(_ss_pin, LOW);
        delayMicroseconds(10);
        
        // Send command
        uint8_t response1 = SPI.transfer(cmd);
        delayMicroseconds(10);
        
        // Send data
        uint8_t response2 = SPI.transfer(data);
        delayMicroseconds(10);
        
        digitalWrite(_ss_pin, HIGH);
        delayMicroseconds(10);
        
        return response2; // Return the actual response
    }

    // Slave: Check for incoming command (non-blocking)
    bool checkForCommand(uint8_t &cmd, uint8_t &data) {
        if (SPI.hasTransaction()) {
            return false;
        }
        
        // Check if master is trying to communicate
        if (digitalRead(_spi_miso) == LOW) {
            // Master will pull SS low, we handle in ISR or loop
            return false;
        }
        
        // In real implementation, you'd use SPI.available() or interrupts
        // This is simplified for example
        return false;
    }

    // Slave: Process incoming command (call from ISR or loop)
    uint8_t processCommand(uint8_t cmd, uint8_t data) {
        switch(cmd) {
            case CMD_PING:
                return RESP_OK;
                
            case CMD_LED_ON:
                digitalWrite(LED_BUILTIN, HIGH);
                return RESP_LED_ON;
                
            case CMD_LED_OFF:
                digitalWrite(LED_BUILTIN, LOW);
                return RESP_LED_OFF;
                
            case CMD_LED_TOGGLE:
                digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
                return digitalRead(LED_BUILTIN) ? RESP_LED_ON : RESP_LED_OFF;
                
            case CMD_LED_STATUS:
                return digitalRead(LED_BUILTIN) ? RESP_LED_ON : RESP_LED_OFF;
                
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
};

#endif // SPI_COMM_H
