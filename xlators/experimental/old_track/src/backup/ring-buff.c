#include "ring-buff.h"
#include "stdio.h"

#define INDEX_INC(index, size) index = (index + 1) & (size - 1);
ring_buff_t *
rb_create(size_t table_size, size_t buffer_size, const char *dumpfile_name)
{
    ring_buff_t *buffer = NULL;

    buffer = (ring_buff_t *)malloc(sizeof(ring_buff_t));
    if (!buffer)
        goto out;
    buffer->table = calloc(table_size, sizeof(void *));
    if (!buffer->table)
        goto out;
    buffer->start = malloc(buffer_size);
    if (!buffer->start)
        goto out;
    memset(buffer->start, 0, buffer_size);
    pthread_mutex_init(&buffer->lock);
    pthread_cond_init(&buffer->cond);

    buffer->end = buffer->start + buffer->size;
    buffer->head = 0;
    buffer->tail = 0;
    buffer->empty = true;

    buffer->dumpfile = fopen(dumpfile, "a+");
    if (!buffer->dumpfile)
    {
        printf("Dumpfile open failed\n");
        goto out;
    }
    return buffer;
out:
    printf("buffer init failed\n");
    rb_destroy(buffer);
    return NULL;
}

void rb_destroy(ring_buff_t *buffer)
{
    if (!buffer)
        reutrn;
    if (buffer->start)
    {
        free(buffer->start);
        buffer->start = NULL;
        buffer->end = NULL;
    }
    if (buffer->table)
    {
        free(table);
        buffer->table = NULL;
    }
    if (buffer->lock)
    {
        pthread_mutex_destory(&buffer->lock);
        buffer->lock = NULL;
    }
    if (buffer->cond)
    {
        pthread_cond_destroy(&buffer->cond);
        buffer->cond = NULL;
    }
    free(buffer);
    buffer = NULL;
    if (buffer->dumpfile)
        fclose(buffer->dumpfile);
    return;
}

void rb_write_cover(ring_buff_t *buffer, void *data, size_t size)
{
    void **table = buffer->table;
    unsigned int head = buffer->head;
    unsigned int tail = buffer->tail;
    size_t split_size = 0;
    void *new_head_offset;

    if (tail == head && !buffer->empty)
    {
        INDEX_INC(tail, buffer->table_size);
    }
    if (table[head] + size > buffer->end)
    {
        split_size = buffer->end - table[head];
        memcpy(table[head], data, split_size);
        memcpy(buffer->start, data + split_size, size - split_size);
        new_head_offset = buffer->start + size - split_size;
        while(table[tail] > table[head] || table[tail] < new_head_offset)
        {
            INDEX_INC(tail,buffer->table_size);
        }
    }
    else
    {
        memcpy(table[head], data, size);
        new_head_offset = table[head] + size;
        while(table[tail] > table[head] && table[tail] < new_head_offset)
        {
            INDEX_INC(tail, buffer->table_size);
        }
    }

    INDEX_INC(head, buffer->table_size);
    table[head] = new_head_offset;

    buffer->head = head;
    buffer->tail = tail;
    buffer->empty = false;
    return;
}

void rb_write(ring_buff_t *buffer, void *data, size_t size)
{
    void **table = buffer->table;
    unsigned int head = buffer->head;
    unsigned int tail = buffer->tail;
    size_t split_size = 0;
    void *new_head_offset = NULL;
    if (head == tail && !buffer->empty)
    {
        printf("Buffer table is full\n");
        return;
    }

    if (table[head] + size > buffer->end)
    {
        if ((!buffer->empty && table[tail] >= table[head]) ||
            table[tail] < buffer->start + table[head] + size - buffer->end)
        {
            printf("Buffer is full\n");
            return;
        }
        split_size = buffer->end - table[head];
        memcpy(table[head], data, split_size);
        memcpy(buffer->start, data + split_size, size - split_size);
        new_head_offset = buffer->start + size - split_size;
        INDEX_INC(buffer->head, buffer->table_size);
        table[buffer->head] = new_head_offset;
    }
    else
    {
        if (!buffer->empty && table[tail] >= table[head] && table[tail] < table[head] + size)
        {
            printf("Buffer is full\n");
            return;
        }
        memcpy(table[head], data, size);
        new_head_offset = table[head] + size;
        INDEX_INC(buffer->head, buffer->table_size);
        table[buffer->head] = new_head_offset;
    }
    return;
}

void rb_dump(ring_buff_t *buffer)
{
    FILE *stream = buffer->dumpfile;
    if (!stream)
    {
        printf("Dumpfile not opened\n");
        return;
    }
    void **table = buffer->table;
    unsigned int head = buffer->head;
    unsigned int tail = buffer->tail;
    if (buffer->empty)
        return;
    if (table[tail] < table[head])
    {
        fwrite(table[tail], table[head] - table[tail],
               1, stream);
    }
    else
    {
        fwrite(table[tail], buffer->end - table[tail],
               1, stream);
        fwrite(buffer->start, table[head] - buffer->start,
               1, stream);
    }
    buffer->tail = buffer->head;
    buffer->empty = true;
    return;
}