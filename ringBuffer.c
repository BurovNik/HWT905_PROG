#include "ringBuffer.h"



static inline int  min(int a, int b)
{
    return a < b ? a : b;
} 


/// @brief Внести даные в buffer из data
/// @param buffer указатель на кольцевой буффер 
/// @param data указатель на данные, которые надо внести
/// @param size количество данных
/// @return возвращает true, если данные успешно добавлены,\n 
/// false - если данные превышают размер допустимый для добавления
bool put(ringBuffer *buffer, uint8_t *data, size_t size)
{
    if (buffer->buffer_size - buffer->bytes_avail < size )
    {
        return false;
    }

    const size_t part_1 = min(size, buffer->buffer_size - buffer->tail);
    const size_t part_2 = size - part_1;

    memcpy(buffer->buffer + buffer->tail, data, part_1);
    memcpy(buffer->buffer, data+part_1, part_2);

    buffer->tail = (buffer->tail + size) % buffer->buffer_size;
    buffer->bytes_avail += size;
    return true;

}

/// @brief Взять данные из buffer в data
/// @param buffer указатель на кольцевой буффер 
/// @param data указатель куда необходимо поместить данные
/// @param size количество данных
/// @return возвращает true, если данные успешно извлечены,\n 
/// false - если данные превышают размер допустимый для извлечения
bool get(ringBuffer *buffer, uint8_t *data, size_t size)
{
    if(buffer->bytes_avail < size)
    {
        return false;
    }

    const size_t part_1 = min(size, buffer->buffer_size - buffer->head);
    const size_t part_2 = size - part_1;

    memcpy(data, buffer->buffer + buffer->head, part_1);
    memcpy(data + part_1, buffer->buffer, part_2);

    buffer->head = (buffer->head + size) % buffer->buffer_size;
    buffer->bytes_avail -= size;
}

/// @brief Функиция, которая выводит кольцевой буфер в консоль
/// @param ringBuffer - указатель на кольцевой буфер
/// @return 
void print_ring_buffer(ringBuffer *ringBuffer)
{
    char text[ringBuffer->bytes_avail + 1];

    for (size_t i = 0; i < ringBuffer->bytes_avail; i++)
    {
        text[i] = ringBuffer->buffer[(ringBuffer->head + i) % ringBuffer->buffer_size];
        
    }
    text[ringBuffer->bytes_avail] = '\n';
    sd_journal_print(LOG_INFO,"%s", text);
    
}

/// @brief функция, которая вывод в консоль содержимое ringBuffer в шестнадцатиричной форме
/// @param ringBuffer - указатель на кольцевой буфер
void print_ring_buffer_hex(ringBuffer *ringBuffer)
{
    for(size_t i =0; i < ringBuffer->bytes_avail; i++)
    {
        printf("%02X ", (unsigned char) ringBuffer->buffer[(ringBuffer->head + i) % ringBuffer->buffer_size]);
    }
    printf("\n");
}