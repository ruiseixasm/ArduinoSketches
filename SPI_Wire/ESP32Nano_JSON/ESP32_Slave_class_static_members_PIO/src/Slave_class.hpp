#pragma once
#include <Arduino.h>
#include <driver/spi_slave.h>
#include <driver/spi_common.h>

// Buffer size
#define BUFFER_SIZE 128

// Message codes (match your protocol)
enum MessageCode : uint8_t {
    START   = 0xF0,
    END     = 0xF1,
    ACK     = 0xF2,
    NACK    = 0xF3,
    READY   = 0xF4,
    ERROR   = 0xF5,
    RECEIVE = 0xF6,
    SEND    = 0xF7,
    NONE    = 0xF8,
    FULL    = 0xF9,
    VOID    = 0xFF
};

class SlaveSPI {
public:
    // public API
    static void begin();
    static bool hasMessage();            // true if a full message received
    static const char * getMessage();    // pointer to received buffer
    static void setSendBuffer(const char *s); // copy into sending buffer

    // Buffers exposed intentionally (original code expects them)
    static char _receiving_buffer[BUFFER_SIZE];
    static char _sending_buffer[BUFFER_SIZE];

    // State (shared)
    static volatile uint8_t _transmission_mode;
    static volatile uint16_t _receiving_index;
    static volatile uint16_t _sending_index;
    static volatile bool _process_message;

private:
    // These are implemented in main.cpp (keeps heavy code out of header)
    static void IRAM_ATTR onSpiPostSetup(spi_slave_transaction_t *trans);
    static void IRAM_ATTR onSpiPostTrans(spi_slave_transaction_t *trans);
	
    // friend functions used by C callbacks
    friend void IRAM_ATTR spi_post_setup_cb(spi_slave_transaction_t *trans);
    friend void IRAM_ATTR spi_post_trans_cb(spi_slave_transaction_t *trans);
	
public:
    static void IRAM_ATTR spiByteHandler(uint8_t c, uint8_t &reply);

};
