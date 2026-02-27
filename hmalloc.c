#include "hmalloc.h"



// Keeps track of the heap starting address.
static void *heap_start = NULL;



// Performs right-block-coalescing once, without checking whether the block on
// the right exists (that is, if `hdr->hdr_next` is not `NULL`) or if i's free. 
// Those checks must be performed by the caller function.
//
// This function shall never be called directly by the user and it is in fact a
// static function in the implementation file.
static mbheader *coalesce_right(mbheader *hdr)
{
    hdr->payload_sz += (AL_HDR_SZ + hdr->hdr_next->payload_sz);
    hdr->hdr_next = hdr->hdr_next->hdr_next;
    
    // We check `p_hdr->hdr_next` because we already assigned
    // `p_hdr->hdr_next->hdr_next` to it.
    if(hdr->hdr_next != NULL)
    {
        hdr->hdr_next->hdr_prev = hdr;
    }
    
    return hdr;
}



// Performs left and right coalescing as long as free neightbours to the block
// pointed to by `hdr` are find.
//
// The block pointed to by `hdr` is not marked as free by the funcion, so this
// action must be performed by the caller funcion.
//
// This function shall never be called directly by the user and it is in fact a
// static function in the implementation file. 
static mbheader *coalesce(mbheader *hdr)
{
    while(hdr->hdr_next != NULL && hdr->hdr_next->free)
    {
        hdr = coalesce_right(hdr);
    }

    while(hdr->hdr_prev != NULL && hdr->hdr_prev->free)
    {
        hdr = coalesce_right(hdr->hdr_prev);
    }

    return hdr;
}



// Performs block splitting withouth checking if the remaining space woult be
// enough to fit a whole new block. Thus, this check must be performed by the
// caller.
//
// This function shall never be called directly by the user and it is in fact a
// static function in the implementation file. 
static void split(mbheader *hdr, size_t al_payload_sz)
{
    size_t old_payload_sz = hdr->payload_sz;
    mbheader *old_hdr_next = hdr->hdr_next;

    mbheader *hdr_new = (mbheader *)((char *)hdr + AL_HDR_SZ + al_payload_sz);

    // Updating the current block.
    hdr->payload_sz = al_payload_sz;
    hdr->hdr_next = hdr_new;

    // Initializing the new block.
    hdr_new->payload_sz = old_payload_sz - al_payload_sz - AL_HDR_SZ;
    hdr_new->free = 1;
    hdr_new->hdr_prev = hdr;
    hdr_new->hdr_next = old_hdr_next;

    // Updating the next-in-the-heap block.
    if(old_hdr_next != NULL)
    {
        old_hdr_next->hdr_prev = hdr_new;
    }
}



void *hmalloc(size_t size)
{
    // Memory alignment must be guaranteed for a well defined behaviour
    // according to the C standard. Thus, we round up both `size` and the
    // header size to the closest multiple of the value that satisfies
    // alignment requirement of each standard type.

    // Aligned `size`.
    size_t al_payload_sz = ALIGN(size);

    // We retrieve the heap start address upon first call. Once `heap_start` is
    // set to a non-`NULL` value, this branch is never supposed to be extecuted
    // again.
    if(heap_start == NULL)
    {
        heap_start = sbrk(0);
        //           ^^^^^^^ We suppose this call not to fail.
    }


    // Since we align `size` (possibly making it bigger) and also have to take
    // into account the header exists, it's safer to perform an overflow check:
    if(al_payload_sz < size || al_payload_sz > SIZE_MAX - AL_HDR_SZ)
    {
        return NULL;
    }


    // The first strategy used to allocate the requested memory is scanning the
    // heap for a block whose payload is big enough to contain `al_payload_sz`
    // bytes of data.

    // Program break address (end of the heap).
    void *prog_brk = sbrk(0);
    //               ^^^^^^^ We suppose this call not to fail.
 
    // Used to traverse the blocks list.
    mbheader *hdr_curr = (mbheader *)heap_start;
    // Used to link memory block headers.
    mbheader *hdr_last = NULL;

    while(hdr_curr != NULL && (prog_brk != heap_start))
    {
        // If we find a big enough free block, we check if block splitting is
        // possible and then return its payload address.
        if(hdr_curr->free == 1 && (hdr_curr->payload_sz >= al_payload_sz))
        {
            hdr_curr->free = 0;
            // We can perform block splitting only if what would remeain of the
            // block we found is big enough for the minimum allocatable block.
            if((hdr_curr->payload_sz - al_payload_sz) >= MIN_BLOCK_SZ)
            {
                split(hdr_curr, al_payload_sz);
            }

            return ((char *)hdr_curr + AL_HDR_SZ);
        }
     
        // We save the current header pointer in case we exit the loop before
        // the next iteration, meaning the header was associated with the last 
        // memory block. In this way, we can reaccess it and link it to the 
        // soon newly added block via `sbrk()`.    
        hdr_last = hdr_curr;

        hdr_curr = hdr_curr->hdr_next;
    }

    // The second strategy used to allocate the requested memory is calling 
    // `sbrk()` for raisung the program break.

    // Pointer to the header of the newly allocated block.
    void *p = sbrk((intptr_t)(al_payload_sz + AL_HDR_SZ));
    
    // There's a possibility `sbrk()` fails.
    if(p == (void *)-1)
    {
        return NULL;
    }

    // If `sbrk()` didn't fail, we must initialize the header fields.
    // Note that `((mbheader *)p)->hdr_prev = hdr_last;` also covers the case
    // where the block is first in the heap because in that case `hdr_last` is 
    // never changed from `NULL`.
    ((mbheader *)p)->free = 0;
    ((mbheader *)p)->payload_sz = al_payload_sz;
    ((mbheader *)p)->hdr_prev = hdr_last;
    ((mbheader *)p)->hdr_next = NULL;

    // Also, if the newly block allocated is not the first in the heap, we need
    // to link the previous one to it.
    if(prog_brk != heap_start)
    {
        ((mbheader *)p)->hdr_prev->hdr_next = p;
    }

    // We must return a pointer to the payload, not the header.
    return ((char *)p + AL_HDR_SZ);
}



void hfree(void *p)
{
    // If argument is `NULL`, the expected behaviour is to do nothing.
    if(p == NULL)
    {
        return;
    }

    // We must check if `p` is a valid pointer by comparing its header to every
    //  possible valid header pointer. Finding no match means the pointer is
    // invalid, it that case we do nothing.

    // Pointer to the header of `p`.
    mbheader *p_hdr = (mbheader *)((char *)p - AL_HDR_SZ);

    // Used to traverse the header list.
    mbheader *hdr_curr = (mbheader *)heap_start;

    while(hdr_curr != p_hdr)
    {
        hdr_curr = hdr_curr->hdr_next;
        if(hdr_curr == NULL)
        {
            // We reached the end of the heap with no valid match.
            return;
        }
    }

    // We must also check if we are trying to double-free a block.
    if(p_hdr->free)
    {
        return;
    }

    // We can now assume `p` is valid.

    //Let's preemtively free the block.
    p_hdr->free = 1;

    // We merge free blocks together.
    p_hdr = coalesce(p_hdr);

    // Note that if left coalescing happened, now we are pointint to the header
    // of the block that was on the left of the block originally pointed by
    // `p_hdr`.

    // If `p_hdr` points to the last block in the heap, we can lower the
    // program break.
    if(((mbheader *)p_hdr)->hdr_next == NULL)
    {
        
               
        // If we reached the start of the heap, we reset `heap_start` back to
        // `NULL`. Otherwise, remove a reference to the block.
        if(p_hdr->hdr_prev == NULL)
        {
            heap_start = NULL;
        }
        else
        {
            p_hdr->hdr_prev->hdr_next = NULL;
        }

        // Call assumed not to fail.
        sbrk((intptr_t)(- AL_HDR_SZ - p_hdr->payload_sz));
    }     
}



void *hcalloc(size_t n_el, size_t size_el)
{
    // If an overflow would happen, we return a `NULL` pointer.
    if(size_el != 0 && n_el > SIZE_MAX / size_el)
    {
        return NULL;
    }

    void *p = hmalloc(n_el * size_el);
    if(p != NULL)
    {
        // We initialize the memory to `0`.
        memset(p, 0, n_el * size_el);
    }

    return p;
}



void *hrealloc(void *p, size_t size_new)
{
    // If `*p` is `NULL`, the function behaves like `hmalloc(size_new)`.
    if(p == NULL)
    {
        return hmalloc(size_new);
    }

    // If `size_new` is `0`, the function behaves like `hfree(p)`.
    if(size_new == 0)
    {
        hfree(p);
        return NULL;
    }

    // `hrealloc()` will work differently depending on whether we are trying to
    // shrink or enlarge a block.

    size_t al_size_new = ALIGN(size_new);
    mbheader *p_hdr = (mbheader *)((char *)p -AL_HDR_SZ);

    // No change
    if(al_size_new == p_hdr->payload_sz)
    {
        return p;
    }

    // Shrinking case
    if(al_size_new < p_hdr->payload_sz)
    {
        // If we can't really split the block, we apply no changes.
        if((p_hdr->payload_sz - al_size_new) >= MIN_BLOCK_SZ)
        {
            split(p_hdr, al_size_new);

            // We also want to try coalescing if free blocks follow.
            coalesce(p_hdr->hdr_next);

            // We want to check if we created a free block at the end of the 
            // heap.
            if(p_hdr->hdr_next->hdr_next == NULL)
            {
                // This will make the heap shrink.
                hfree(p_hdr->hdr_next);
            }
        }

        return p;
    }

    // Grow in the middle of the heap case.
    // Surely `al_size_new > p_hdr->payload_sz` now.
    if(p_hdr->hdr_next != NULL && p_hdr->hdr_next->free == 1 
        && p_hdr->payload_sz + AL_HDR_SZ + p_hdr->hdr_next->payload_sz
            >= al_size_new)
    {
        p_hdr = coalesce_right(p_hdr);
        
        // We want to check if we need to take the whole block or not.
        if(p_hdr->payload_sz >= al_size_new + MIN_BLOCK_SZ)
        {
            split(p_hdr, al_size_new);
        }

        return p;
    }
    
    // Grow at the end of the heap case.
    if(p_hdr->hdr_next == NULL)
    {
        if(sbrk((intptr_t)(al_size_new - p_hdr->payload_sz)) != (void *)-1)
        {
            p_hdr->payload_sz = al_size_new;
            return p;
        }

        return NULL;
    }

    // Fallback allocation case.

    void *p_new = hmalloc(al_size_new);

    if(p_new == NULL)
    {
        return NULL;
    }

    mbheader *p_hdr = (mbheader *)((char *)p - AL_HDR_SZ);

    memcpy(p_new, p, p_hdr->payload_sz);

    hfree(p);

    return p_new;
}



void *hreallocarray(void *p, size_t n_el_new, size_t size_el_new)
{
    // If an overflow would happen, we return a `NULL` pointer.
    if(size_el_new != 0 && n_el_new > SIZE_MAX / size_el_new)
    {
        return NULL;
    }

    return hrealloc(p, n_el_new * size_el_new);
}

