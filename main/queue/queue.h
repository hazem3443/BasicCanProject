#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct Queue_t* QueueHandle_t;

QueueHandle_t queue_create(size_t queue_size, size_t item_size);
void queue_destroy(QueueHandle_t queue);

bool queue_send_to_front(QueueHandle_t queue, const void *item);
bool queue_send_to_back(QueueHandle_t queue, const void *item);
bool queue_receive(QueueHandle_t queue, void *item);

bool queue_is_empty(QueueHandle_t queue);
bool queue_is_full(QueueHandle_t queue);

#endif // QUEUE_H
