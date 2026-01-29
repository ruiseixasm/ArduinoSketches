#include <Arduino.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_timer.h>

// HSPI Pins
#define MOSI_PIN GPIO_NUM_13
#define MISO_PIN GPIO_NUM_12
#define SCLK_PIN GPIO_NUM_14
#define CS_PIN   GPIO_NUM_15
#ifndef LED_PIN
#define LED_PIN  GPIO_NUM_2
#endif
#define BUFFER_SIZE 128

const char* string_on = "{'t':'Nano','m':2,'n':'ON','f':'Talker-9f','i':3540751170,'c':24893}";
const char* string_off = "{'t':'Nano','m':2,'n':'OFF','f':'Talker-9f','i':3540751170,'c':24893}";

spi_device_handle_t _spi;
uint8_t tx_buffer[BUFFER_SIZE];
uint32_t counter = 0;
bool send_on_string = true;

void setup_gpio() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_PIN, 0);
    Serial.println("LED configured");
}

void setup_spi_master() {
    Serial.println("Setting up SPI Master (HSPI) for 128-byte transfers...");
    
    // SPI bus configuration
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = MOSI_PIN;
    buscfg.miso_io_num = MISO_PIN;
    buscfg.sclk_io_num = SCLK_PIN;
    buscfg.quadwp_io_num = GPIO_NUM_NC;
    buscfg.quadhd_io_num = GPIO_NUM_NC;
    buscfg.max_transfer_sz = BUFFER_SIZE;
    
    // SPI device configuration
    spi_device_interface_config_t devcfg = {};
    devcfg.clock_speed_hz = 8000000;
    devcfg.mode = 0;
    devcfg.spics_io_num = CS_PIN;
    devcfg.queue_size = 1;
    devcfg.flags = 0;
    
    // Initialize SPI bus WITH DMA
    esp_err_t ret = spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        Serial.printf("SPI bus init failed: %s\n", esp_err_to_name(ret));
        return;
    }
    
    // Attach device
    ret = spi_bus_add_device(HSPI_HOST, &devcfg, &_spi);
    if (ret != ESP_OK) {
        Serial.printf("SPI device add failed: %s\n", esp_err_to_name(ret));
        return;
    }
    
    Serial.println("SPI Master (HSPI) initialized:");
    Serial.printf("  MOSI: GPIO%d\n", MOSI_PIN);
    Serial.printf("  MISO: GPIO%d\n", MISO_PIN);
    Serial.printf("  SCLK: GPIO%d\n", SCLK_PIN);
    Serial.printf("  CS:   GPIO%d\n", CS_PIN);
    Serial.printf("  LED:  GPIO%d\n", LED_PIN);
    Serial.printf("  Clock: 8 MHz\n");
    Serial.printf("  Buffer: %d bytes\n\n", BUFFER_SIZE);
}

void fill_buffer(uint8_t *buffer, uint32_t counter, bool send_on_string) {
    memset(buffer, 0, BUFFER_SIZE);
    
    const char* current_string = send_on_string ? string_on : string_off;
    size_t str_len = strlen(current_string);
    
    if (str_len < BUFFER_SIZE) {
        memcpy(buffer, current_string, str_len);
    } else {
        memcpy(buffer, current_string, BUFFER_SIZE);
    }
    
    char info[32];
    snprintf(info, sizeof(info), "|CNT:%lu|", counter);
    size_t info_len = strlen(info);
    
    if (str_len + info_len < BUFFER_SIZE) {
        memcpy(&buffer[str_len], info, info_len);
    }
    
    if (BUFFER_SIZE > 10) {
        buffer[BUFFER_SIZE-3] = send_on_string ? 'O' : 'F';
        buffer[BUFFER_SIZE-2] = 'N';
        buffer[BUFFER_SIZE-1] = '\0';
    }
}

void send_128byte_buffer_with_led(uint8_t *buffer) {
    gpio_set_level(LED_PIN, 1);
    Serial.println("💡 LED ON - Starting transmission...");
    
    spi_transaction_t trans = {};
    trans.length = BUFFER_SIZE * 8;
    trans.tx_buffer = buffer;
    trans.rx_buffer = NULL;
    
    uint64_t start = esp_timer_get_time();
    esp_err_t ret = spi_device_transmit(_spi, &trans);
    uint64_t duration = esp_timer_get_time() - start;
    
    if (ret == ESP_OK) {
        Serial.printf("✓ 128-byte buffer sent in %lluµs\n", duration);
    } else {
        Serial.printf("✗ SPI transmit failed: %s\n", esp_err_to_name(ret));
    }
    
    uint64_t elapsed_time = esp_timer_get_time() - start;
    int64_t remaining_time = 100000 - elapsed_time;
    
    if (remaining_time > 0) {
        esp_rom_delay_us(remaining_time);
    }
    
    gpio_set_level(LED_PIN, 0);
    Serial.println("💡 LED OFF after 100ms");
}

void print_buffer_preview(uint8_t *buffer, uint32_t counter, bool send_on_string) {
    Serial.printf("[Transmission #%lu - %s]\n", counter, send_on_string ? "ON" : "OFF");
    
    int str_len = 0;
    for (str_len = 0; str_len < BUFFER_SIZE && buffer[str_len] != 0; str_len++);
    
    Serial.printf("String (%d chars): ", str_len);
    for (int i = 0; i < str_len && i < 80; i++) {
        Serial.print((char)buffer[i]);
    }
    if (str_len > 80) Serial.print("...");
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n================================\n");
    Serial.println("ESP32 SPI MASTER (HSPI) - ESP-IDF");
    Serial.println("Sending ON/OFF strings every 2 seconds");
    Serial.println("Blue LED blinks 100ms during transfer");
    Serial.println("================================\n\n");
    
    setup_gpio();
    setup_spi_master();
    
    Serial.println("Starting string transmission loop...\n");
    Serial.printf("String 1 (ON):  %s\n", string_on);
    Serial.printf("String 2 (OFF): %s\n\n", string_off);
    
    // Blink LED 3 times for startup
    for (int i = 0; i < 3; i++) {
        gpio_set_level(LED_PIN, 1);
        delay(100);
        gpio_set_level(LED_PIN, 0);
        delay(100);
    }
}

void loop() {
    fill_buffer(tx_buffer, counter, send_on_string);
    print_buffer_preview(tx_buffer, counter, send_on_string);
    send_128byte_buffer_with_led(tx_buffer);
    
    counter++;
    send_on_string = !send_on_string;
    
    Serial.println("\nWaiting 2 seconds...\n");
    delay(2000);
}
