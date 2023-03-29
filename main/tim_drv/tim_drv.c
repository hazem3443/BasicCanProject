#include "tim_drv.h"
#include "esp_timer.h"

uint32_t TIM_getTimestamp(void) {
    int64_t timestamp_us = esp_timer_get_time();
    uint32_t timestamp_10us = (uint32_t)(timestamp_us / 10);
    return timestamp_10us;
}
