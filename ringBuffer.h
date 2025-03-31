#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdint.h> 
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <systemd/sd-journal.h>

/// @brief структура для описания кольцевого буффера. buffer - указатель на область памяти. 
/// buffer_size - размер буффера. head - номер ячейки где начинаются данные в буффере
/// tail - номер ячейки, где заканчиваются даные в буффере.
/// bytes_avail количество байт доступных для записи
typedef struct 
{
    uint8_t *buffer;
    size_t buffer_size;
    size_t head;
    size_t tail;
    size_t bytes_avail;
}ringBuffer ;

bool put(ringBuffer *buffer, uint8_t *data, size_t size);
bool get(ringBuffer *buffer, uint8_t *data, size_t size);
void print_ring_buffer(ringBuffer *ringBuffer);
void print_ring_buffer_hex(ringBuffer *ringBuffer);

#endif // RINGBUFFER_H