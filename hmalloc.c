#include "hmalloc.h"



// Keeps track of the heap starting address.
static void *heap_start = NULL;



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
                size_t old_payload_sz = hdr_curr->payload_sz;
                mbheader *old_hdr_next = hdr_curr->hdr_next;

                mbheader *hdr_new = (mbheader *)
                    ((char *)hdr_curr + AL_HDR_SZ + al_payload_sz);

                // Updating the current block.
                hdr_curr->payload_sz = al_payload_sz;
                hdr_curr->hdr_next = hdr_new;

                // Initializing the new block.
                hdr_new->payload_sz 
                    = old_payload_sz - al_payload_sz - AL_HDR_SZ;
                hdr_new->free = 1;
                hdr_new->hdr_prev = hdr_curr;
                hdr_new->hdr_next = old_hdr_next;

                // Updating the next-in-the-heap block.
                if(old_hdr_next != NULL)
                {
                    old_hdr_next->hdr_prev = hdr_new;
                }
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

    // If `p` points to the last block in the heap, we can lower the program
    // break. Otherwise, we just mark the block as free for future reusage.
    if(((mbheader *)p_hdr)->hdr_next == NULL)
    {
        // We reuse `hdr_curr` to possibly traverse back the heap if multiple
        // free blocks are present, to lower the program break.
        hdr_curr = ((mbheader *)p_hdr);
        // Keeps track of how much the program break will decrease.
        size_t shift = -(hdr_curr->payload_sz +  AL_HDR_SZ);

        while(hdr_curr->hdr_prev != NULL && hdr_curr->hdr_prev->free == 1)
        {
            // As long as we don't reach the start of the heap, we increase (as
            // a negative value) `shift` and set to `NULL` the `hdr_next` 
            // pointer of the previous header. 
            shift -= (hdr_curr->hdr_prev->payload_sz +  AL_HDR_SZ);
            hdr_curr->hdr_prev->hdr_next = NULL;

            hdr_curr = hdr_curr->hdr_prev;
        }

        // Call assumed not to fail.
        sbrk((intptr_t)(shift));
               
        // If we reached the start of the heap, we reset `heap_start` back to
        // `NULL`.
        if(hdr_curr->hdr_prev == NULL)
        {
            heap_start = NULL;
        }
    }
    else
    {
        p_hdr->free = 1;
    }      
}
