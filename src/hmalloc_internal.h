/*  hmalloc - heap memory allocator project.

    See https://github.com/sizeof-dario/hmalloc.git for the project repo and
    check its README file for more informations about the project.

 *************************************************************************** */

/* "hmalloc_internal.h"

    Contains internal definition for hmalloc.   */

#ifndef HMALLOC_INTERNAL_H
#define HMALLOC_INTERNAL_H 1

/*  Feature test macro required for sbrk() since glibc 2.19, as stated in UNIX 
    manual at https://man7.org/linux/man-pages/man2/brk.2.html  */
#ifndef _DEFAULT_SOURCE
 #define _DEFAULT_SOURCE
#endif

#include <stdalign.h>
#include <stddef.h>     /* For max_align_t  */
#include <stdint.h>     /* For SIZE_MAX     */ 
#include <string.h>



/*  Header of a heap memory block allocated by `hmalloc()` and the related 
    functions.  */
typedef struct header
{
    size_t         payload_size;    /* Size of the block payload.       */
    int            is_free;         /* Tracks whether a block is free.  */
    struct header *hdr_prev;        /* Pointer to the previous header.  */
    struct header *hdr_next;        /* Pointer to the next header.      */
} header;



/*  Aligns `size` to be compatible with the alignement requirements of all the
    standard types. */
#define ALIGN(size) \
    (((size) + alignof(max_align_t) - 1) & ~(alignof(max_align_t) - 1))

/*  Aligned header size. */
#define AL_HDR_SIZE ALIGN(sizeof(header))

/*  Minimum size of a block with. */
#define MIN_BLOCK_SIZE (AL_HDR_SIZE + ALIGN(sizeof(char)))

#endif /* HMALLOC_INTERNAL_H */
