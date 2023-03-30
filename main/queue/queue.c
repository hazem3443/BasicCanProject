#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct Queue_t {
    size_t item_size;
    size_t capacity;
    size_t count;
    size_t front;
    size_t rear;
    uint8_t *buffer;
} Queue_t;

QueueHandle_t queue_create(size_t capacity, size_t item_size) {
    Queue_t *queue = malloc(sizeof(Queue_t));
    if (queue == NULL) {
        return NULL;
    }

    queue->item_size = item_size;
    queue->capacity = capacity;
    queue->count = 0;
    queue->front = 0;
    queue->rear = 0;
    queue->buffer = malloc(capacity * item_size);

    if (queue->buffer == NULL) {
        free(queue);
        return NULL;
    }

    return queue;
}

void queue_destroy(QueueHandle_t queue_handle) {
    Queue_t *queue = (Queue_t *)queue_handle;
    free(queue->buffer);
    free(queue);
}

bool queue_send_to_front(QueueHandle_t queue_handle, const void *item) {
    Queue_t *queue = (Queue_t *)queue_handle;
    if (queue_is_full(queue)) {
        return false;
    }

    queue->front = (queue->front + queue->capacity - 1) % queue->capacity;
    memcpy(queue->buffer + queue->front * queue->item_size, item, queue->item_size);
    queue->count++;

    return true;
}

bool queue_send_to_back(QueueHandle_t queue_handle, const void *item) {
    Queue_t *queue = (Queue_t *)queue_handle;
    if (queue_is_full(queue)) {
        return false;
    }

    memcpy(queue->buffer + queue->rear * queue->item_size, item, queue->item_size);
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->count++;

    return true;
}

bool queue_receive(QueueHandle_t queue_handle, void *item) {
    Queue_t *queue = (Queue_t *)queue_handle;
    if (queue_is_empty(queue)) {
        return false;
    }

    memcpy(item, queue->buffer + queue->front * queue->item_size, queue->item_size);
    queue->front = (queue->front + 1) % queue->capacity;
    queue->count--;

    return true;
}

bool queue_is_empty(QueueHandle_t queue_handle) {
    Queue_t *queue = (Queue_t *)queue_handle;
    return queue->count == 0;
}

bool queue_is_full(QueueHandle_t queue_handle) {
    Queue_t *queue = (Queue_t *)queue_handle;
    return queue->count == queue->capacity;
}
