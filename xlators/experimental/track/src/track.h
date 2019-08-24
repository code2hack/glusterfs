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
#include "circ-buff.h"
#include "statedump.h"
#include "options.h"

#define TRACK_DEFAULT_HISTORY_SIZE 1024
#define TRACK_DEFAULT_LOG_DIR "/logs/"
typedef struct {
        /* Since the longest fop name is fremovexattr i.e 12 characters, array size
         * is kept 24, i.e double of the maximum.
         */
        char name[24];
        int enabled;
} track_fop_name_t;

track_fop_name_t track_fop_names[GF_FOP_MAXVALUE];

typedef struct {
        gf_boolean_t log_file;
        gf_boolean_t log_history;
        size_t       history_size;
//        int track_log_level;
        FILE *csv_file;

} track_conf_t;

#define TRACK_STACK_UNWIND(op, frame, params ...)                       \
        do {                                                            \
                frame->local = NULL;                                    \
                STACK_UNWIND_STRICT (op, frame, params);                \
        } while (0);
/* 
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

*/
#define LOG_ELEMENT(_conf, _string)                                     \
        do{                                                             \
                if(_conf)                                               \
                {                                                               \
                        if((_conf->log_file) == _gf_true && _conf->csv_file)    \
                        {                                                       \
                                fputs(_string,_conf->csv_file);              \
                        }                                                       \
                }                                                               \
        } while(0);
