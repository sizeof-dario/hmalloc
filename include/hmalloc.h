/*  hmalloc - heap memory allocator project.

    See https://github.com/sizeof-dario/hmalloc.git for the project repo and
    check its README file for more informations about the project.

 *************************************************************************** */

/*  "hmalloc.h" - Master include file for hmalloc.

    Contains all the API definitions for the allocator.  */

#ifndef HMALLOC_H
#define HMALLOC_H 1

#include <unistd.h>

/*  Allocates `size` bytes of heap memory.

    On success, returns a pointer to the allocated memory. 
    On failure, returns `NULL`. */

void *hmalloc(size_t size);



/*  Frees memory pointed to by `p`.

    The memory must have been allocated via `hmalloc()`, `hcalloc()`, 
    `hrealloc()` or `hreallocarray()`. If not, it's undefined behaviour.    */
void hfree(void *p);



/*  Allocates heap memory for `n` elements of `size` bytes and initializes the 
    memory to `0`.

    On success, returns a pointer to the allocated memory. 
    On failure, returns `NULL`. */
void *hcalloc(size_t n, size_t size);



/*  Reallocates memory pointed to by `p` resizing it to `size`.  

    On success, returns a pointer to the reallocated memory. 
    On failure, returns `NULL`.

    The memory must have been allocated via `hmalloc()`, `hcalloc()`, 
    `hrealloc()` or `hreallocarray()`. If not, it's undefined behaviour.    */
void *hrealloc(void *p, size_t size);



/*  Reallocates an array pointed to by `p` resizing it for `n` elements of
    `size` bytes.

    On success, returns a pointer to the reallocated memory. 
    On failure, returns `NULL`.

    The memory must have been allocated via `hmalloc()`, `hcalloc()`, 
    `hrealloc()` or `hreallocarray()`. If not, it's undefined behaviour.    */
void *hreallocarray(void *p, size_t n, size_t size);

#endif /* HMALLOC_H */
