#ifndef HMALLOC_H
#define HMALLOC_H 1



// Feature test macro required for `sbrk()` since glibc 2.19.
#define _DEFAULT_SOURCE



#include <stdalign.h>   // For `alignof()`.
#include <stddef.h>     // For `max_align_t`.
#include <stdint.h>     // For `SIZE_MAX`.
#include <string.h>     // For `memset()`.
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



// Gets the minimum of two values.
#define MIN(x, y) ((x) < (y) ? (x) : (y))


// Allocates `size` bytes of heap memory.
void *hmalloc(size_t size);



// Frees heap memory pointed by `p`, allocated by `hmalloc()` or `hcalloc()`.
void hfree(void *p);



// Allocates enough space for `n_el` elements of size `size_el`.
void *hcalloc(size_t n_el, size_t size_el);



// Reallocates memory pointed to by `p` resizing it to `size_new`.
void *hrealloc(void *p, size_t size_new);


#endif // HMALLOC_H
