#include "req2vec.h"



req_t *
create_req (const char *loc, off_t offset, size_t size, const char *fop, char *info)
{
    req_t *req = NULL;
    req = malloc(sizeof(req_t));
    if(!req)
        goto out;
    if (loc)
    {
        req->loc = malloc(sizeof(loc));
        if (!req->loc)
        {
            destroy_req(req);
            goto out;
        }
        strcpy(req->loc, loc);
    }
    
    req->offset = offset;
    req->size = size;

    if (!fop)
    {
        destroy_req(req);
        goto out;
    }
    strcpy(req->fop, fop);
    if (info)
    {
        req->info = malloc(sizeof(info));
        if (!req->info)
        {
            destroy_req(req);
            goto out;
        }
        strcpy(req->info, info);
    }

out:
    return req;
}

void
destroy_req (req_t *req)
{
    if (!req)
        return;
    if(req->loc)
    {
        free(req->loc);
        req->loc = NULL;
    }
    if(req->info)
    {
        free(req->info);
        req->info = NULL;
    }

    free(req);
    return;
}

