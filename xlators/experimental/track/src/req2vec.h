#ifndef _REQ2VEC_H
#define _REQ2VEC_H



typedef struct _req
{
    /*Origin data*/
    char *loc;
    off_t offset;
    size_t size;
    char fop[24];
    char *info;

    //Feature vector for this req
}req_t;

typedef struct _window
{
    req_t *reqs;
}window_t;

req_t *
create_req (char *loc, off_t offset, size_t size, char *fop, char *info);

void
destroy_req (req_t *req);


#endif