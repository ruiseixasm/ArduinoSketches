#include <Arduino.h>
#include "Slave_class.hpp"
#include "driver/spi_slave.h"

// -----------------------------------------------------------------------------
// Static member definitions (must be in a single translation unit)
// -----------------------------------------------------------------------------
char SlaveSPI::_receiving_buffer[BUFFER_SIZE];
char SlaveSPI::_sending_buffer[BUFFER_SIZE];

volatile uint8_t  SlaveSPI::_transmission_mode = MessageCode::NONE;
volatile uint16_t SlaveSPI::_receiving_index  = 0;
volatile uint16_t SlaveSPI::_sending_index    = 0;
volatile bool     SlaveSPI::_process_message  = false;

// -----------------------------------------------------------------------------
// Small transaction scratch bytes used by SPI callbacks (IRAM safe)
// -----------------------------------------------------------------------------
static uint8_t spi_rx_byte = 0;
static uint8_t spi_tx_byte = MessageCode::READY;

// We will allocate a single transaction struct for reuse
static spi_slave_transaction_t spi_trans;

// Forward declarations for callbacks used by the ESP32 SPI slave driver
extern "C" {
    void IRAM_ATTR spi_post_setup_cb(spi_slave_transaction_t *trans);
    void IRAM_ATTR spi_post_trans_cb(spi_slave_transaction_t *trans);
}

// -----------------------------------------------------------------------------
void SlaveSPI::begin()
{
    // Clear buffers & state
    memset(_receiving_buffer, 0, BUFFER_SIZE);
    memset(_sending_buffer, 0, BUFFER_SIZE);
    _transmission_mode = MessageCode::NONE;
    _receiving_index = 0;
    _sending_index = 0;
    _process_message = false;

    // SPI pins (change to match wiring if needed)
    const int PIN_MISO = 19;
    const int PIN_MOSI = 23;
    const int PIN_SCLK = 18;
    const int PIN_SS   = 5;

    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 8
    };

    spi_slave_interface_config_t slvcfg = {
        .spics_io_num = PIN_SS,
        .flags = 0,
        .queue_size = 3,
        .mode = 0,
        .post_setup_cb = spi_post_setup_cb,
        .post_trans_cb = spi_post_trans_cb
    };

    esp_err_t r = spi_slave_initialize(HSPI_HOST, &buscfg, &slvcfg, SPI_DMA_DISABLED);
    if (r != ESP_OK) {
        Serial.printf("spi_slave_initialize failed: %d\n", r);
    }

    // Prepare a queued transaction (we set lengths per byte inside the callback)
    memset(&spi_trans, 0, sizeof(spi_trans));
    spi_trans.length = 8;            // bits (1 byte)
    spi_trans.rx_buffer = &spi_rx_byte;
    spi_trans.tx_buffer = &spi_tx_byte;

    // Queue the first transaction
    spi_slave_queue_trans(HSPI_HOST, &spi_trans, portMAX_DELAY);
}

bool SlaveSPI::hasMessage()
{
    return _process_message;
}

const char * SlaveSPI::getMessage()
{
    return _receiving_buffer;
}

void SlaveSPI::setSendBuffer(const char *s)
{
    // copy into sending buffer; avoid using heavy functions in IRAM path
    strncpy(_sending_buffer, s, BUFFER_SIZE - 1);
    _sending_buffer[BUFFER_SIZE - 1] = '\0';
}

// -----------------------------------------------------------------------------
// This is the minimal post-setup callback (IRAM), called *before* a transfer.
// We must set the tx buffer pointer (it will be used for the clocked out byte).
// -----------------------------------------------------------------------------
void IRAM_ATTR spi_post_setup_cb(spi_slave_transaction_t *trans)
{
    // The driver may pass us the transaction; ensure it will use spi_tx_byte
    trans->tx_buffer = &spi_tx_byte;
}

// -----------------------------------------------------------------------------
// This is called AFTER a transaction finishes (IRAM). Received byte is available.
// We must process it with your exact handler and prepare the reply for next byte.
// -----------------------------------------------------------------------------
void IRAM_ATTR spi_post_trans_cb(spi_slave_transaction_t *trans)
{
    // read incoming byte (already placed by driver into spi_rx_byte)
    uint8_t incoming = spi_rx_byte;
    uint8_t reply = MessageCode::READY; // default reply

    // Call your exact ISR-like handler (kept unchanged)
    SlaveSPI::spiByteHandler(incoming, reply);

    // Prepare next tx byte (the driver will clock this out on next transfer)
    spi_tx_byte = reply;

    // Re-queue the transaction so we continuously accept bytes
    // Note: spi_trans is reused; ensure driver finished with it
    spi_trans.length = 8;
    spi_trans.rx_buffer = &spi_rx_byte;
    spi_trans.tx_buffer = &spi_tx_byte;
    spi_slave_queue_trans(HSPI_HOST, &spi_trans, portMAX_DELAY);
}

// -----------------------------------------------------------------------------
// Your exact protocol code — PRESERVED. Runs in IRAM (marked IRAM_ATTR).
// NOTE: avoid heavy library use or Serial inside here.
// -----------------------------------------------------------------------------
void IRAM_ATTR SlaveSPI::spiByteHandler(uint8_t c, uint8_t &reply) {

    // WARNING: keep this function small and fast. No Serial inside here.

    if (c < 128) {  // Only ASCII chars shall be transmitted as data

        switch (_transmission_mode) {
            case RECEIVE:
                if (_receiving_index < BUFFER_SIZE) {
                    // Returns same received char as receiving confirmation (no need to set SPDR)
                    _receiving_buffer[_receiving_index++] = c;
                } else {
                    reply = FULL;    // ALWAYS ON TOP
                    _transmission_mode = NONE;
                }
                break;

            case SEND:
                if (_sending_index < BUFFER_SIZE) {
                    reply = _sending_buffer[_sending_index];  // prepare byte to send
                    if (_receiving_index > _sending_index) {  // Less missed sends this way
                        reply = END;  // All chars have been checked
                        break;
                    }
                } else {
                    reply = FULL;
                    _transmission_mode = NONE;
                    break;
                }
                // Starts checking 2 indexes after
                if (_sending_index > 1) {    // Two positions of delay
                    if (c != _sending_buffer[_receiving_index]) {   // Also checks '\0' char
                        reply = ERROR;
                        _transmission_mode = NONE;  // Makes sure no more communication is done, regardless
                        break;
                    }
                    _receiving_index++; // Starts checking after two sent
                }
                // Only increments if NOT at the end of the string being sent
                if (_sending_buffer[_sending_index] != '\0') {
                    _sending_index++;
                }
                break;

            default:
                reply = NACK;
        }

    } else {    // It's a control message 0xF*

        switch (c) {
            case RECEIVE:
                reply = ACK;
                _transmission_mode = RECEIVE;
                _receiving_index = 0;
                break;

            case SEND:
                if (_sending_buffer[0] == '\0') {
                    reply = NONE;    // Nothing to send
                } else {    // Starts sending right away, so, no ACK
                    reply = _sending_buffer[0];
                    _transmission_mode = SEND;
                    _sending_index = 1;  // Skips to the next char
                    _receiving_index = 0;
                }
                break;

            case END:
                reply = ACK;
                if (_transmission_mode == RECEIVE) {
                    _process_message = true;
                } else if (_transmission_mode == SEND) {
                    _sending_buffer[0] = '\0';  // Makes sure the sending buffer is marked as empty (NONE next time)
                }
                _transmission_mode = NONE;
                break;

            case ACK:
                reply = READY;
                break;

            case ERROR:
            case FULL:
                reply = ACK;
                _transmission_mode = NONE;
                break;

            default:
                reply = NACK;
        }
    }
}

// -----------------------------------------------------------------------------
// MAIN : example usage and message handling (runs in loop context, not IRAM)
// -----------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    delay(50);
    Serial.println("\nESP32 SPI Slave (Nano-like behavior) starting...");

    // Example: put a test message in the sending buffer
    const char *test = "HELLO_FROM_ESP32";
    SlaveSPI::setSendBuffer(test);

    SlaveSPI::begin();

    Serial.println("Slave initialized.");
}

void loop()
{
    // When the ISR sets _process_message to true, read the buffer in loop (safe)
    if (SlaveSPI::hasMessage()) {
        // Copy the message to local buffer quickly (avoid long operations in ISR)
        char local[BUFFER_SIZE];
        noInterrupts();
        strncpy(local, SlaveSPI::getMessage(), BUFFER_SIZE - 1);
        local[BUFFER_SIZE - 1] = '\0';
        // Reset the receiving buffer index so new messages can come
        SlaveSPI::_receiving_index = 0;
        SlaveSPI::_process_message = false;
        interrupts();

        Serial.print("Received message: ");
        Serial.println(local);

        // Example: prepare reply
        SlaveSPI::setSendBuffer("OK");
    }

    // small idle delay
    delay(10);
}
