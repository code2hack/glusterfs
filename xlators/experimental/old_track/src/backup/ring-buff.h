#ifndef _RING_BUFF_H
#define _RING_BUFF_H

#include "common-utils.h"

#define DEFAULT_TABLE_SIZE 4096
#define DEFAULT_BUFFER_SIZE 1<<20 //1MB total buffersize

typedef struct _ring_buff
{
    void **table;
    /*Boundary of the buffer*/
    void *start;
    void *end;
    unsigned int head;//w_index in the table
    unsigned int tail;//r_index
    size_t table_size;//Number of indexes
    size_t buffer_size;//Total capacity(bytes) of the buffer
    bool empty;

    pthread_t lock;
    pthread_cond_t cond;
    /*Log file */
    FILE *dumpfile;
}ring_buff_t;



ring_buff_t *
rb_create (size_t table_size, size_t buffer_size, const char* dumpfile_name);

void
rb_write_cover(ring_buff_t *buffer, void *data,size_t size);

void
rb_write(ring_buff_t *buffer, void *data, size_t size);

void *
rb_read (ring_buff_t *buffer);

void
rb_destroy(ring_buff_t *buffer);

void
rb_dump(ring_buff_t *buffer);

#endif
