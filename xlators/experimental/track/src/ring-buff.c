/*
  Copyright (c) 2008-2012 Red Hat, Inc. <http://www.redhat.com>
  This file is part of GlusterFS.

  This file is licensed to you under your choice of the GNU Lesser
  General Public License, version 3 or any later version (LGPLv3 or
  later), or the GNU General Public License, version 2 (GPLv2), in all
  cases as published by the Free Software Foundation.
*/

#include "ring-buff.h"
#include "libglusterfs-messages.h"

/* hold lock while calling this function */
int _rb_write_data(ring_buffer_t *buffer, void *item)
{
        void *ptr = NULL;
        if (buffer->size_used == buffer->size_buffer)
                return -1;
        //DO we really need the assert here?
        //GF_ASSERT (buffer->used_len <= buffer->size_buffer);
        /*Should we free the buffer at writing point here?*/

        buffer->rb[buffer->w_index] = item;
        buffer->w_index++;
        buffer->w_index %= buffer->size_buffer;
        buffer->size_used++;
        return buffer->w_index;
}

int rb_write_data(ring_buffer_t *buffer, void *item)
{
        int write_index = -1;
        pthread_mutex_lock(&buffer->lock);
        {
                write_index = _rb_write_data(buffer, item);
        }
        pthread_mutex_unlock(&buffer->lock);
        pthread_cond_signal(&buffer->read_signal);
        return write_index;
}

void *rb_read_data(ring_buffer_t *buffer)
{
        void *data = buffer->rb[buffer->r_index];
        if (data)
        {
                buffer->r_index = (buffer->r_index + 1) % buffer->size_buffer;
                buffer->size_used--;
        }
        return data;
}

ring_buffer_t *
rb_buffer_new(size_t buffer_size, void (*destroy_buffer_data)(void *data))
{
        ring_buffer_t *buffer = NULL;
        buffer = malloc(sizeof(ring_buffer_t));
        if (!buffer)
        {
                goto out;
        }

        buffer->rb = calloc(buffer_size, sizeof(void *));
        if (!buffer->rb)
        {
                free(buffer);
                buffer = NULL;
                goto out;
        }
        buffer->w_index = 0;
        buffer->r_index = 0;
        buffer->size_used = 0;
        buffer->size_buffer = buffer_size;
        buffer->destroy_buffer_data = destroy_buffer_data;
        pthread_mutex_init(&buffer->lock, NULL);
        pthread_cond_init(&buffer->read_signal, NULL);

out:
        return buffer;
}

void rb_buffer_destroy(ring_buffer_t *buffer)
{
        int i = 0;
        void *ptr = NULL;
        if (buffer)
        {
                if (buffer->rb)
                {
                        for (i = 0; i < buffer->size_buffer; i++)
                        {
                                ptr = buffer->rb[i];
                                if (!ptr)
                                {
                                        buffer->destroy_buffer_data(ptr);
                                }
                        }
                        GF_FREE(buffer->rb);
                }
                pthread_mutex_destroy(&buffer->lock);
                pthread_cond_destroy(&buffer->read_signal);
                GF_FREE(buffer);
        }
}
