#include "can_drv.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include <string.h>

static const char *TAG = "can_drv";

static QueueHandle_t tx_queue;
static QueueHandle_t rx_queue;
#ifdef USE_FREERTOS
static SemaphoreHandle_t rx_notification;
static void can_receive_task(void *arg);
static void can_transmit_task(void *arg);
#else
static void can_receive_isr();
static void (*rx_isr_callback)(void) = NULL;
#endif


esp_err_t can_drv_init(void) {
    twai_general_config_t config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, TWAI_MODE_NORMAL);
    
    twai_timing_config_t timing = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    esp_err_t err = twai_driver_install(&config, &timing, &filter);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install TWAI driver: %d", err);
        return err;
    }

    #ifdef USE_FREERTOS
    tx_queue = xQueueCreate(TX_QUEUE_SIZE, sizeof(twai_message_t));
    rx_queue = xQueueCreate(RX_QUEUE_SIZE, sizeof(twai_message_t));
    rx_notification = xSemaphoreCreateBinary();
    #else
    tx_queue = queue_create(TX_QUEUE_SIZE, sizeof(twai_message_t));//we can use static allocation instead 
    rx_queue = queue_create(RX_QUEUE_SIZE, sizeof(twai_message_t));
    #endif

    #ifdef USE_FREERTOS
    xTaskCreate(can_receive_task, "can_receive_task", 2048, NULL, 5, NULL);
    xTaskCreate(can_transmit_task, "can_transmit_task", 2048, NULL, 5, NULL);
    #else
    can_drv_register_rx_isr(NULL);//register default
    #endif

    return ESP_OK;
}

esp_err_t can_drv_deinit(void) {
    esp_err_t err = twai_driver_uninstall();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to uninstall TWAI driver: %d", err);
    }

    #ifdef USE_FREERTOS
    if (tx_queue) {
        vQueueDelete(tx_queue);
        tx_queue = NULL;
    }

    if (rx_queue) {
        vQueueDelete(rx_queue);
        rx_queue = NULL;
    }

    if (rx_notification) {
        vSemaphoreDelete(rx_notification);
        rx_notification = NULL;
    }
    #else
    if (tx_queue) {
        queue_destroy(tx_queue);
        tx_queue = NULL;
    }

    if (rx_queue) {
        queue_destroy(rx_queue);
        rx_queue = NULL;
    }
    #endif
    
    return err;
}

void CAN_tx(uint32_t canId, uint8_t *canData, uint8_t dataSize) {
    twai_message_t message;
    message.identifier = canId;
    message.flags = TWAI_MSG_FLAG_NONE;
    message.data_length_code = dataSize;
    memcpy(message.data, canData, dataSize);

    #ifdef USE_FREERTOS
    BaseType_t result;
    if (message.identifier == PRIORITY_MSG_ID) {
        result = xQueueSendToFront(tx_queue, &message, pdMS_TO_TICKS(100));
    } else {
        result = xQueueSendToBack(tx_queue, &message, pdMS_TO_TICKS(100));
    }

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to enqueue message for transmission");
    }
    #else
    bool success;
    if (message.identifier == PRIORITY_MSG_ID) {
        success = queue_send_to_front(tx_queue, &message);
    } else {
        success = queue_send_to_back(tx_queue, &message);
    }
    if (!success) {
        ESP_LOGE(TAG, "Failed to enqueue message for transmission");
    }
    #endif
}

bool CAN_rx(uint32_t *canId, uint8_t *canData, uint8_t *dataSize) {
    twai_message_t rx_msg;

    #ifdef USE_FREERTOS
    BaseType_t result = xQueueReceive(rx_queue, &rx_msg, pdMS_TO_TICKS(100));

    if (result == pdPASS) {
        *canId = rx_msg.identifier;
        *dataSize = rx_msg.data_length_code;
        memcpy(canData, rx_msg.data, rx_msg.data_length_code);
        return true;
    } else {
        return false;
    }
    #else
    esp_err_t err;
    bool success;

    err = twai_receive(&rx_msg, pdMS_TO_TICKS(0));
    ESP_LOGW(TAG, "rx_msg: ID=0x%x", rx_msg.data_length_code);
    ESP_LOGW(TAG, "rx_msg: DLC=0x%x", rx_msg.data_length_code);
    ESP_LOGW(TAG, "rx_msg: data[0]=0x%x", rx_msg.data[0]);
    ESP_LOGW(TAG, "rx_msg: data[0]=0x%x", rx_msg.data[1]);
    ESP_LOGW(TAG, "rx_msg: data[0]=0x%x", rx_msg.data[2]);
    ESP_LOGW(TAG, "rx_msg: data[0]=0x%x", rx_msg.data[3]);
    ESP_LOGW(TAG, "rx_msg: data[0]=0x%x", rx_msg.data[4]);
    ESP_LOGW(TAG, "rx_msg: data[0]=0x%x", rx_msg.data[5]);
    ESP_LOGW(TAG, "rx_msg: data[0]=0x%x", rx_msg.data[6]);
    ESP_LOGW(TAG, "rx_msg: data[0]=0x%x", rx_msg.data[7]);
    
    if (err == ESP_OK) {
        success = queue_send_to_back(rx_queue, &rx_msg);//enqueue
        //if success
        if (!success) {
            ESP_LOGE(TAG, "Failed to enqueue message for reception");
        }
        else{
            *canId = rx_msg.identifier;
            *dataSize = rx_msg.data_length_code;
            memcpy(canData, rx_msg.data, rx_msg.data_length_code);
            can_receive_isr();//dequeue
        }
        return success;
    } else {
        ESP_LOGE(TAG, "Failed to receive message: %d", err);
        return false;
    }
    #endif
}

#ifdef USE_FREERTOS
esp_err_t can_drv_register_rx_notification(SemaphoreHandle_t *rx_notification_handle) {
    *rx_notification_handle = rx_notification;
    return ESP_OK;
}
static void can_receive_task(void *arg) {
    twai_message_t rx_msg;
    esp_err_t err;

    while (1) {
        err = twai_receive(&rx_msg, pdMS_TO_TICKS(100));
        if (err == ESP_OK) {
            BaseType_t result = xQueueSendToBack(rx_queue, &rx_msg, pdMS_TO_TICKS(100));
            if (result == pdPASS) {
                xSemaphoreGive(rx_notification);
            } else {
                ESP_LOGE(TAG, "Failed to store received message in queue");
            }
        } else if (err != ESP_ERR_TIMEOUT) {
            ESP_LOGE(TAG, "Failed to receive message: %d", err);
        }
    }
}

static void can_transmit_task(void *arg) {
    twai_message_t tx_msg;
    esp_err_t err;

    while (1) {
        BaseType_t result = xQueueReceive(tx_queue, &tx_msg, portMAX_DELAY);
        if (result == pdPASS) {
            err = twai_transmit(&tx_msg, pdMS_TO_TICKS(100));
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to transmit message: %d", err);
            }
        }
    }
}
#else
// Timer callback function
void can_transmit_timer_callback(void *arg) {
    twai_message_t message;
    esp_err_t err;

    // Check if there is a message in the queue and transmit it
    if (!queue_is_empty(tx_queue)) {
        queue_receive(tx_queue, &message);
        err = twai_transmit(&message, pdMS_TO_TICKS(100));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to transmit message: %d", err);
        }
    }
}

// Function to initialize the timer
void init_can_transmit_timer(void) {
    esp_timer_create_args_t timer_args = {
        .callback = can_transmit_timer_callback,
        .name = "can_transmit_timer"
    };

    esp_timer_handle_t timer_handle;
    esp_err_t err = esp_timer_create(&timer_args, &timer_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer: %d", err);
        return;
    }

    // Start the timer with a periodic interval (e.g., 100 ms)
    err = esp_timer_start_periodic(timer_handle, 100 * 1000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start timer: %d", err);
    }
}
static void can_receive_isr(void){
    //send notification message to the application
    rx_isr_callback();
    twai_message_t message;//dummy message to flush
    //remove elemnt from the queue
    queue_receive(tx_queue, &message);
}
esp_err_t can_drv_register_rx_isr(void (*rx_isr)(void)) {
    if (rx_isr) {
        rx_isr_callback = rx_isr;
        return ESP_OK;
    } else {
        return ESP_ERR_INVALID_ARG;
    }
}
#endif


