#include "req2vec.h"



req_t *
create_req (loc_t *loc, const char *fop, char *info)
{
    req_t *req = NULL;
    req = malloc(sizeof(req_t));
    if(!req)
        goto out;
    req->pathname = malloc(sizeof(loc->path)+sizeof(loc->name));
    if (!req->pathname)
    {
        destroy_req(req);
        goto out;
    }
    sprintf(req->pathname, strcat(loc->path, loc->name));
    

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
    if(req->pathname)
    {
        free(req->pathname);
        req->pathname = NULL;
    }
    if(req->info)
    {
        free(req->info);
        req->info = NULL;
    }

    free(req);
    return;
}

req_buffer_t *
create_req_buffer(size_t buffer_size)
{
    req_buffer_t *nb = NULL;
    nb = malloc(sizeof(req_buffer_t));
    if (!nb)
        goto out;
    nb->reqs = calloc(buffer_size, sizeof(req_t *));    
    if (!nb->reqs)
    {
        free(nb);
        goto out;
    }
    nb->buffer_size = buffer_size;
    nb->w_index = 0;
    pthread_mutex_init(&nb->lock);
out:
    return nb;
}
void
destroy_req_buffer (req_buffer_t *buffer)
{
    pthread_mutex_lock(&buffer->lock);
    for (unsigned int i=0;i<buffer->w_index;++i)
    {
        destroy_req(buffer->reqs[i]);
        reqs[i] = NULL;
    }
    pthread_mutex_unlock(&buffer->lock);
    pthread_mutex_destroy(&buffer->lock);
    free(buffer);
    return;
}

void
log_req_buffer (req_buffer_t *buffer)
{

}