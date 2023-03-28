#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    gpio_set_direction(GPIO_NUM_21, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_21, GPIO_PULLUP_ONLY);

    gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);

    for(;;){
        if(gpio_get_level(GPIO_NUM_21)){
            gpio_set_level(GPIO_NUM_5,0);
            printf("Turn OFF!\n");
        }
        else{
            gpio_set_level(GPIO_NUM_5,1);
            printf("Turn ON!\n");
        }

        vTaskDelay(10);
    }

}
