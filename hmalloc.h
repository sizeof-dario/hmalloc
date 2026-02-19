#ifndef HMALLOC_H
#define HMALLOC_H 1

#define _DEFAULT_SOURCE

#include <unistd.h>

// Memory block header used by hmalloc.
typedef struct mbheader
{
    size_t size;
    int free;
} mbheader;

// Allocates size bytes of heap memory.
void *hmalloc(size_t size);

// Frees heap memory allocated via hmalloc.
void hfree(void *p);

#endif // HMALLOC_H
