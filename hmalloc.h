#ifndef HMALLOC_H
#define HMALLOC_H 1

// Feature test macro required for `sbrk()` since glibc 2.19.
#define _DEFAULT_SOURCE

#include <stdalign.h>   // For `alignof()`.
#include <stddef.h>     // For `max_align_t`.
#include <unistd.h>     // For `sbrk()`.

// Memory block header used by `hmalloc()` and `hfree()`.
typedef struct mbheader
{
    size_t blocksize;       // Size of the block associated to this.
    int free;               // Flag for whether a block is free or occupied.
    struct mbheader *next;  // Pointer to the next memory block header.
} mbheader;

// Aligns `x` to the alignment of at least the greatest standard type with no
// alignment specifiers.
#define ALIGN(x) \
    (((x) + alignof(max_align_t) - 1) & ~(alignof(max_align_t) - 1))

// Allocates `size` bytes of heap memory.
void *hmalloc(size_t size);

// Frees heap memory pointed by `p` and allocated by `hmalloc()`.
void hfree(void *p);

#endif // HMALLOC_H
