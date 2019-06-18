/*
   Copyright (c) 2006-2012 Red Hat, Inc. <http://www.redhat.com>
   This file is part of GlusterFS.

   This file is licensed to you under your choice of the GNU Lesser
   General Public License, version 3 or any later version (LGPLv3 or
   later), or the GNU General Public License, version 2 (GPLv2), in all
   cases as published by the Free Software Foundation.
*/

#include <time.h>
#include <errno.h>
#include "glusterfs.h"
#include "xlator.h"
#include "common-utils.h"
#include "event-history.h"
#include "logging.h"
#include "statedump.h"
#include "options.h"
#include "ring-buff.h"
#include "req2vec.h"
#include "options.h"

#define TRACK_DEFAULT_BUFFER_SIZE 4096
#define TRACK_MAX_READ_THREADS 32
#define TRACK_DEFAULT_READ_THREADS 4

typedef struct {
        /* Since the longest fop name is fremovexattr i.e 12 characters, array size
         * is kept 24, i.e double of the maximum.
         */
        char name[24];
        int enabled;
} track_fop_name_t;
typedef struct track_conf {
        ring_buffer_t *buffer;
        pthread_t read_threads[TRACK_MAX_READ_THREADS];
        int threads_num;
        bool read_threads_should_die;
        /*----Location of log/module. The root path of mounted point is recommended----*/
} track_conf_t;

track_fop_name_t track_fop_names[GF_FOP_MAXVALUE];





#define TRACK_STACK_UNWIND(op, frame, params ...)                       \
        do {                                                            \
                frame->local = NULL;                                    \
                STACK_UNWIND_STRICT (op, frame, params);                \
        } while (0);

#define LOG_ELEMENT(_conf, _string)                                     \
        do {                                                            \
                if (_conf) {                                            \
                        if ((_conf->log_history) == _gf_true)           \
                                gf_log_eh ("%s", _string);              \
                        if ((_conf->log_file) == _gf_true)              \
                                gf_log (THIS->name, _conf->track_log_level, \
                                        "%s", _string);                 \
                }                                                       \
        } while (0);

//void _destroy_read_procs(track_conf_t *conf);
