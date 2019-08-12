#ifndef _REQ2VEC_H
#define _REQ2VEC_H

#include "common-utils.h"

#define REQ_DEFAULT_BUFFER_SIZE 512 
typedef struct _req
{
    /*Origin data*/
    char *pathname;
    char fop[24];
    char *info;
    //Feature vector for this req
}req_t;

typedef struct _req_buffer
{
    size_t buffer_size;
    unsigned int w_index;
    req_t **reqs;
    pthread_mutex_t lock;
}req_buffer_t;

typedef struct _model
{
    /*

    */

}model_t;
req_t *
create_req (loc_t *loc, const char *fop, char *info);

void
destroy_req (req_t *req);

req_buffer_t *
create_req_buffer (size_t buffer_size);

void
log_req_buffer (req_buffer_t *buffer);

void
destroy_req_buffer(req_buffer_t *buffer);

#endif
