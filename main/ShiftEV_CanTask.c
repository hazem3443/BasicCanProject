#include <inttypes.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "can_drv/can_drv.h"
#include "tim_drv/tim_drv.h"

#define CAN_IDENTIFIER 0x5
#define DATA_LENGTH 8
#define DELAY_500_MS (500 / portTICK_PERIOD_MS)

static const char *TAG = "CAN_APP";

#define USE_RTOS_EXAMPLE
#define num_of_cycle_per_ms 1000

typedef enum{
    TransmitState = 0,
    WaitState,
}Transmit_StateMachine;

void app_main(void) {
    
    // Set up the message to send
    CANMessage rxMessage, txMessage = {
        .canId = CAN_IDENTIFIER,
        .canData = {0xBE, 0xEF, 0xAB, 0xAB, 0x00, 0x00, 0x00, 0x00},
        .dataSize = DATA_LENGTH
    };
    
    esp_err_t err = can_drv_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize CAN driver: %d", err);
        return;
    }

    #ifndef USE_RTOS_EXAMPLE
    // Initialize the TWAI driver
    // Register a semaphore for RX notifications
    SemaphoreHandle_t rx_notification;
    can_drv_register_rx_notification(&rx_notification);

    while (true) {
        // Check for received messages
        if (xSemaphoreTake(rx_notification, pdMS_TO_TICKS(100)) == pdTRUE) {

            bool success = CAN_rx(&(rxMessage.canId), rxMessage.canData, &(rxMessage.dataSize));
            if (success) {
                // Process the received message
                ESP_LOGI(TAG, "Received message with ID: 0x%" PRIx32, rxMessage.canId);
            } else {
                ESP_LOGE(TAG, "Failed to read received message");
            }
        }

        // Send messages periodically or upon certain events
        CAN_tx(txMessage.canId, txMessage.canData, txMessage.dataSize);
        ESP_LOGI(TAG, "Message sent successfully");

        //we can use this new module from any where in our project
        uint32_t timestamp = TIM_getTimestamp();
        ESP_LOGI(TAG, "Current timestamp (10us units): %" SCNu32, timestamp);

        // Wait for 500 ms
        vTaskDelay(DELAY_500_MS);
    }

    // Deinitialize the TWAI driver
    can_drv_deinit();
    #else
    //register transmit function to send messages periodically from the queue
    init_can_transmit_timer();

    //check for received CAN message
    Transmit_StateMachine currentState; //initial state is TransmitState
    uint32_t msDelay;

    currentState = TransmitState;
    while (1) {
        bool success = CAN_rx(&(rxMessage.canId), rxMessage.canData, &(rxMessage.dataSize));
        if (success) {
            //isr hook called successfully removing elements from the queue
            ESP_LOGI(TAG, "Message received successfully");
        }

        switch (currentState)
        {
        case TransmitState:
            CAN_tx(txMessage.canId, txMessage.canData, txMessage.dataSize);
            ESP_LOGI(TAG, "Message sent successfully");
            msDelay = 1000 * num_of_cycle_per_ms;
            currentState = WaitState;
            break;
        case WaitState:
            if(msDelay-- == 0){
                currentState = TransmitState;
            }
            else{
                /*do nothing*/
            }
            break;
        default:
            ESP_LOGI(TAG, "error Occured");
            break;
        }

    }

    #endif
}
