#ifndef HMALLOC_H
#define HMALLOC_H 1

// Required for sbrk() since glibc 2.19.
#define _DEFAULT_SOURCE

#include <stdalign.h> // For alignof().
#include <stddef.h> // For max_align_t.
#include <unistd.h> // For sbrk().

// Memory block header used by hmalloc.
typedef struct mbheader
{
    size_t size;
    int free;
} mbheader;

// Aligns x to the alignement of at least the greatest standard type with no 
// alignment specifiers.
#define ALIGN(x) \
    (((x) + alignof(max_align_t) - 1) & ~(alignof(max_align_t) - 1))

// Allocates size bytes of heap memory.
void *hmalloc(size_t size);

// Frees heap memory allocated via hmalloc.
void hfree(void *p);

#endif // HMALLOC_H
