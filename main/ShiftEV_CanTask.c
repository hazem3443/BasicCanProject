#include "ShiftEV_CanTask.h"

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

    //register transmit function to send messages periodically from the queue
    init_can_transmit_timer();

    //register CAN hook to be called upon reception
    can_drv_register_rx_isr(RXCAN_Hook);
    
    //check for received CAN message
    Transmit_StateMachine currentState; //initial state is TransmitState
    uint32_t msDelay;

    currentState = TransmitState;
    while (1) {

        bool success = CAN_rx(&(rxMessage.canId), rxMessage.canData, &(rxMessage.dataSize));
        if (success) {
            //isr hook called successfully removing elements from the queue
            ESP_LOGI(TAG, "Message received successfully");
            continue;
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
                currentState = TransmitState;
                break;
        }
    }
    //deinitialize the CAN
    can_drv_deinit();
}

void RXCAN_Hook(void)
{
    ESP_LOGW(TAG, "RXCAN_HOOK CALLED");
}