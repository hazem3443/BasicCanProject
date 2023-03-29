#include "can_drv.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "can_drv";

static QueueHandle_t tx_queue;
static QueueHandle_t rx_queue;
static SemaphoreHandle_t rx_notification;

static void can_receive_task(void *arg);
static void can_transmit_task(void *arg);

esp_err_t can_drv_init(void) {
    twai_general_config_t config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, TWAI_MODE_NORMAL);
    twai_timing_config_t timing = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    esp_err_t err = twai_driver_install(&config, &timing, &filter);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install TWAI driver: %d", err);
        return err;
    }

    tx_queue = xQueueCreate(TX_QUEUE_SIZE, sizeof(twai_message_t));
    rx_queue = xQueueCreate(RX_QUEUE_SIZE, sizeof(twai_message_t));
    rx_notification = xSemaphoreCreateBinary();

    xTaskCreate(can_receive_task, "can_receive_task", 2048, NULL, 5, NULL);
    xTaskCreate(can_transmit_task, "can_transmit_task", 2048, NULL, 5, NULL);

    return ESP_OK;
}

esp_err_t can_drv_deinit(void) {
    esp_err_t err = twai_driver_uninstall();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to uninstall TWAI driver: %d", err);
    }

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

    return err;
}

void CAN_tx(uint32_t canId, uint8_t *canData, uint8_t dataSize) {
    twai_message_t message;
    message.identifier = canId;
    message.flags = TWAI_MSG_FLAG_NONE;
    message.data_length_code = dataSize;
    memcpy(message.data, canData, dataSize);

    BaseType_t result;
    if (message.identifier == PRIORITY_MSG_ID) {
        result = xQueueSendToFront(tx_queue, &message, pdMS_TO_TICKS(100));
    } else {
        result = xQueueSendToBack(tx_queue, &message, pdMS_TO_TICKS(100));
    }

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to enqueue message for transmission");
    }
}

bool CAN_rx(uint32_t *canId, uint8_t *canData, uint8_t *dataSize) {
    twai_message_t message;
    BaseType_t result = xQueueReceive(rx_queue, &message, pdMS_TO_TICKS(100));

    if (result == pdPASS) {
        *canId = message.identifier;
        *dataSize = message.data_length_code;
        memcpy(canData, message.data, message.data_length_code);
        return true;
    } else {
        return false;
    }
}
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

