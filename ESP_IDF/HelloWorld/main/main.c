#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    printf("Hello from ESP-IDF!\n");
    
    while (1) {
        printf("Running...\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}