/*
  Copyright (c) 2008-2012 Red Hat, Inc. <http://www.redhat.com>
  This file is part of GlusterFS.

  This file is licensed to you under your choice of the GNU Lesser
  General Public License, version 3 or any later version (LGPLv3 or
  later), or the GNU General Public License, version 2 (GPLv2), in all
  cases as published by the Free Software Foundation.
*/

#ifndef _RB_H
#define _RB_H

#include "common-utils.h"
#include "logging.h"


struct _ring_buffer
{
        //unsigned int w_index;
        pthread_mutex_t lock;
        pthread_cond_t read_signal;
        unsigned int r_index;
        unsigned int w_index;
        size_t size_buffer;
        size_t size_used;
//        void *stats;
        //gf_boolean_t use_once;
        /* This variable is assigned the proper value at the time of initing */
        /* the buffer. It indicates, whether the buffer should be used once */
        /*  it becomes full. */

        /* indicates the amount of circular buffer used. */

        void **rb;
        void (*destroy_buffer_data)(void *data);
};

typedef struct _ring_buffer ring_buffer_t;

int rb_write_data(ring_buffer_t *buffer, void *item);
void* rb_read_data(ring_buffer_t *buffer);

ring_buffer_t *
rb_buffer_new(size_t buffer_size,void (*destroy_data)(void *data));

void rb_buffer_destroy(ring_buffer_t *buffer);

#endif /* _CB_H */
