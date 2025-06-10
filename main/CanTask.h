#ifndef SHIFTEV_CANTASK_H
#define SHIFTEV_CANTASK_H

#include <inttypes.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_private/esp_clk.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "can_drv/can_drv.h"
#include "tim_drv/tim_drv.h"


#define CAN_IDENTIFIER 0x5
#define DATA_LENGTH 8
#define DELAY_500_MS (500 / portTICK_PERIOD_MS)

static const char *TAG = "CAN_APP";

#define num_of_cycle_per_ms 1000

typedef enum{
    TransmitState = 0,
    WaitState,
}Transmit_StateMachine;

void RXCAN_Hook(void);

#endif // TIM_DRV_H
