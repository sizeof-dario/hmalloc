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
    size_t payload_sz;          // Size of the block payload.
    int free;                   // Tells whether a block is free or occupied.
    struct mbheader *hdr_prev;  // Pointer to the previous memory block header.
    struct mbheader *hdr_next;  // Pointer to the next memory block header.
} mbheader;



// Aligns `size` to the alignment of at least the greatest standard type with
// no alignment specifiers.
#define ALIGN(size) \
    (((size) + alignof(max_align_t) - 1) & ~(alignof(max_align_t) - 1))



// Aligned header size
#define AL_HDR_SZ ALIGN(sizeof(mbheader))



// Minimum size for a block.
#define MIN_BLOCK_SZ (AL_HDR_SZ + ALIGN(sizeof(char)))



// Allocates `size` bytes of heap memory.
void *hmalloc(size_t size);



// Frees heap memory pointed by `p` and allocated by `hmalloc()`.
void hfree(void *p);



#endif // HMALLOC_H
