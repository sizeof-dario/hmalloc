/*  hmalloc - heap memory allocator project.

    See https://github.com/sizeof-dario/hmalloc.git for the project repo and
    check its README file for more informations about the project.

 *************************************************************************** */

#include "include/hmalloc.h"
#include "hmalloc_internal.h"

static void *heap_start = NULL; /* Keeps track of the heap starting address. */



static inline header *do_coalesce_right(header *hdr_to_clsc)
{
    /*  We update the payload size making sure to also include the extra header
        space.  */
    hdr_to_clsc->payload_size 
        += (AL_HDR_SIZE + hdr_to_clsc->hdr_next->payload_size);

    /*  We update the hdr_next pointer in the block we are performing right
        coalescing on.  */
    hdr_to_clsc->hdr_next = hdr_to_clsc->hdr_next->hdr_next;
    
    /*  We update the hdr_prev pointer in the block on the right. Note that we
        must work on hdr_to_clsc->hdr_next and not 
        hdr_to_clsc->hdr_next->hdr_next now because of the previous 
        reassigment.    */
    if(hdr_to_clsc->hdr_next != NULL)
    {
        hdr_to_clsc->hdr_next->hdr_prev = hdr_to_clsc;
    }
    
    /*  We return the header pointer passed to the function. It's not really
        necessary, but I thinks it renders these functions more uniform.    */
    return hdr_to_clsc;
}



static inline header *try_coalesce(header *hdr_to_clsc)
{
    /*  As long as right free neighbours are found, we perform right 
        coalescing. */
    while(hdr_to_clsc->hdr_next != NULL && hdr_to_clsc->hdr_next->is_free)
    {
        hdr_to_clsc = do_coalesce_right(hdr_to_clsc);
    }

    /*  As long as left free neighbours are found, we perform left 
        coalescing. */
    while(hdr_to_clsc->hdr_prev != NULL && hdr_to_clsc->hdr_prev->is_free)
    {
        hdr_to_clsc = do_coalesce_right(hdr_to_clsc->hdr_prev);
    }

    /*  We return the header pointer passed to the function because left
        coalescing changes it and so it must be updated in the caller.  */
    return hdr_to_clsc;
}



static inline void do_split(header *hdr_to_split, size_t hdr_payload_size)
{
    /*  We need to save the current data to later update the headers of the 
        blocks adjacent to the one we're about to split and initialize the 
        header of the extra block that will be created by the splitting.    */

    size_t payload_size_old = hdr_to_split->payload_size;
    header *hdr_next_old = hdr_to_split->hdr_next;

    /*  We create a new header.                 */

    header *hdr_new 
        = (header *)((char *)hdr_to_split + AL_HDR_SIZE + hdr_payload_size);

    /*  We update the splitted block header.    */
    hdr_to_split->payload_size = hdr_payload_size;
    hdr_to_split->hdr_next = hdr_new;

    /*  We initialize the new block header.     */
    hdr_new->payload_size = payload_size_old - hdr_payload_size - AL_HDR_SIZE;
    hdr_new->is_free = 1;
    hdr_new->hdr_prev = hdr_to_split;
    hdr_new->hdr_next = hdr_next_old;

    /*  We update the header of the block that next to the new one. */
    if(hdr_next_old != NULL)
    {
        hdr_next_old->hdr_prev = hdr_new;
    }
}



static inline void try_split(header *hdr_to_split, size_t hdr_payload_size)
{
    /*  If the space we need is small enough for a non-degenerate block to fit
        in what's left of the free block after the allocation, we can perform 
        block splitting. */
    if((hdr_to_split->payload_size - hdr_payload_size) >= MIN_BLOCK_SIZE)
    {
        do_split(hdr_to_split, hdr_payload_size);
    }
}



void *hmalloc(size_t payload_size)
{
    /*  Upon first call, we shall retrieve the heap starting address value. */
    if(heap_start == NULL)
    {
        /*  sbrk() can fail. However, looking at glibc implementation 
            https://github.com/lattera/glibc/blob/master/misc/sbrk.c we can see
            how the following check is perfomed at lines 44-45: 
            
            if (increment == 0)
                return __curbrk;

            Thus, we can assume sbrk(0) not to fail and avoid checking if its 
            return value is (void *)(-1) from now on when the provided argument 
            is 0.   */

        heap_start = sbrk(0);
    }

    /*  We must guarantee memory alignment to ensure defined behaviour
        according to the C standard. Thus, even if the user passes a certain
        payload_size to hmalloc(), the function will really align it first and
        then work with the aligned value.   */

    size_t al_payload_size = ALIGN(payload_size);   /* Aligned payload size. */

    /*  Aligning the payload size may increase its value, so we must prevent a
        possible overflow. We shall check if the aligned value turns out to be
        smaller than the original value (meaning that it itself overflowed),
        but we must also consider the case where the aligned payload size still
        is in the range of a size_t but it's adding the header that makes the
        whole block too big for its size of type size_t. */

    if(al_payload_size < payload_size 
    || al_payload_size > SIZE_MAX - AL_HDR_SIZE)
    {
        return NULL;
    }

    /*  We'll never need to use the unaligned value of payload_size. So I think
        it's cleaner to just overwrite it and continue using payload_size
        instead of having to remember to use its al_ version.   */
    payload_size = al_payload_size;





    /*  To allocate the memory, it's easier to first assume some previously 
        allocated blocks already exist; so, we attempt traversing their list to
        possibly find a block with a big enough payload size so we can reuse it 
        intstead of calling sbrk() to increase the program break.   */

    void *pbrk = sbrk(0);                       /* Program break.            */
    header *hdr_curr = (header *)heap_start;    /* Traverses the list.       */
    header *hdr_last = NULL;                    /* Used for linking headers. */

    /*  The first condition checks if there are no blocks in the heap;
        The second ones tells if we reached the end of the list.        */
    while(pbrk != heap_start && hdr_curr != NULL)
    {
        /*  We want to find a big enough block that is free.    */
        if(hdr_curr->is_free == 1 && (hdr_curr->payload_size >= payload_size))
        {
            /*  We mark the block as occupied.  */
            hdr_curr->is_free = 0;

            /*  We try block splitting.         */
            try_split(hdr_curr, payload_size);

            /*  We return a pointer to the payload area, not the header.    */
            return ((char *)hdr_curr + AL_HDR_SIZE);
        }

        /*  We keep track of the last accessed header so that if we exit the
            loop without returning, meaning we reached the end of the list,
            we can create a new block and link its header to it.            */    
        hdr_last = hdr_curr;

        /*  We move forward in the list.    */
        hdr_curr = hdr_curr->hdr_next;
    }





    /*  If we weren't able to find a suitable block, or if there were no blocks
        to begin with, we use sbrk() to raise the program break and we create a
        new block.  */
    
    /*  Pointer to the header of the newly allocated block. */
    void *p = sbrk((intptr_t)(payload_size + AL_HDR_SIZE));
    
    /*  sbrk() can fail.    */
    if(p == (void *)(-1))
    {
        return NULL;
    }

    /*  We have to initialize the header fields. Note that

        ((mbheader *)p)->hdr_prev = hdr_last; 

        also covers the case where the block is first in the heap because in 
        that case hdr_last is never changed from its initialization value of
        NULL.   */
    ((header *)p)->is_free = 0;
    ((header *)p)->payload_size = payload_size;
    ((header *)p)->hdr_prev = hdr_last;
    ((header *)p)->hdr_next = NULL;

    /*  Moreover, if the block is not first in the heap, we need to link to it 
    the previous one.       */
    if(pbrk != heap_start)
    {
        ((header *)p)->hdr_prev->hdr_next = p;
    }

    /*  We return a pointer to the payload area, not the header.    */
    return ((char *)p + AL_HDR_SIZE);
}



void hfree(void *payload_ptr)
{
    /*  As stated in N1570 §7.22.3.3 for the free() function: "If [payload_ptr]
        is a null pointer, no action occurs". We'll follow the standard. */
    if(payload_ptr == NULL)
    {
        return;
    }

    /*  payload_ptr could not be a pointer to memory allocated by hmalloc() or 
        the related functions. The standard for free() says that "if the
        argument does not match a pointer earlier returned by a memory 
        management function [...] the behavior is undefined". However, for the 
        sake of trying to prevent heap corruption, we'll check if payload_ptr
        is a valid pointer by comparing its header to every possible valid 
        header pointer. Finding no match means the pointer is probabily 
        invalid, and in that case we do nothing.    */

    /* Pointer to the header of payload_ptr.        */
    header *hdr = (header *)((char *)payload_ptr - AL_HDR_SIZE);

    /* Pointer used to traverse the headers list.   */
    header *hdr_curr = (header *)heap_start;

    while(hdr_curr != hdr)
    {
        hdr_curr = hdr_curr->hdr_next;
        if(hdr_curr == NULL)
        {
            /*  If we get into this branch, it means we found no falid match
                for hdr, thus payload_ptr is invalid and we just silently 
                return. */
            return;
        }
    }

    /*  The standard also calls undefined behaviour when a double free is
        attempted. We can make easly turn this scenario deterministic by
        silently returnign in such cases.   */
    if(hdr->is_free)
    {
        return;
    }





    /*  After the previous checks, we can assume payload_ptr is valid, in the
        sense that hdr corresponds to a valid header of our list that we can
        deallocate.                         */

    /*  Let's preemtively free the block.   */
    hdr->is_free = 1;

    /*  We can try block coalescing.        */
    hdr = try_coalesce(hdr);


    /*  If hdr happens to point to the last block in the heap, we can lower the
        program break.                      */
    if(hdr->hdr_next == NULL)
    {
        /*  Unless we also happen to be freeing the first block in the heap, we
            want to update the new-last block of our list.  */
        if(hdr->hdr_prev != NULL)
        {
            hdr->hdr_prev->hdr_next = NULL;
        }

        /*  We lower the program break. Note that sbrk() can fail, however it's
            not a big deal here because it just means we are deallocating the
            memory "logically" (that means, it can be reaused) but not 
            "phisically" (because it was not given back to the kernel). */
        sbrk((intptr_t)(- AL_HDR_SIZE - hdr->payload_size));
    }     
}



void *hcalloc(size_t n_el, size_t size_el)
{
    /*  If an overflow would happen, we return a NULL pointer.  */
    if(size_el != 0 && n_el > SIZE_MAX / size_el)
    {
        return NULL;
    }

    void *p = hmalloc(n_el * size_el);

    if(p != NULL)
    {
        /*  We initialize the memory to 0.  */
        memset(p, 0, n_el * size_el);
    }

    return p;
}



void *hrealloc(void *payload_ptr, size_t payload_size_new)
{
    /*  First, we need to handle the two trivial argument cases.    */

    /*  1. According to N1570 §7.22.3.5, realloc(NULL, payload_size_new) must 
        work as malloc(payload_size_new). We will follow this standard with 
        hrealloc() and hmalloc().   */
    if(payload_ptr == NULL)
    {
        return hmalloc(payload_size_new);
    }

    /*  2. The ISO C standard doesn't explicitly define what realloc() (and so
        hrealloc()) should do when payload_size_new is 0. The UNIX manual 
        states at https://man7.org/linux/man-pages/man3/realloc.3p.html that 
        "If [payload_size_new] is 0 [you can return] a pointer to the allocated
        space [and free] the memory object pointed to by [payload_ptr]. We 
        choose this option. */
    if(payload_size_new == 0)
    {
        hfree(payload_ptr);
        return payload_ptr;
    }





    /*  Now, hrealloc() can perform different operations when reallocating the
        block, based on the resizing request and its position in the heap.
        We'll see them after setting the stage. */

    size_t al_payload_size_new = ALIGN(payload_size_new);

    /*  Aligning the payload new size may increase its value, so we must 
    prevent a possible overflow. We shall check if the aligned value turns out
    to be smaller than the original value (meaning that it itself overflowed),
    but we must also consider the case where the aligned payload size still
    is in the range of a size_t but it's adding the header that makes the
    whole block too big for its size of type size_t. */
    if(al_payload_size_new < payload_size_new 
    || al_payload_size_new > SIZE_MAX - AL_HDR_SIZE)
    {
        /*  For this situation, the ISO C standard imposes that "if memory for
            the new object cannot be allocated, the old object is not 
            deallocated and its value is unchanged" and asks to return a
            pointer (in this case, unchanged) to the old object.            */
        return payload_ptr;
    }

    /*  We'll never need to use the unaligned value of payload_size_new. So 
        I think it's cleaner to just overwrite it and continue using 
        payload_size_new instead of its al_ version.    */
    payload_size_new = al_payload_size_new;

    /*  Header associated to `payload_ptr`. */
    header *hdr = (header *)((char *)payload_ptr - AL_HDR_SIZE);





    /*  We can now divide the hrealloc() action into 5 cases.   */

    /*  1. No change.   */
    if(payload_size_new == hdr->payload_size)
    {
        return payload_ptr;
    }





    /*  2. The block must be shrunk. */
    if(payload_size_new < hdr->payload_size)
    {
        /*  We can try to use block splitting if enough space becomes
            available. We perform the check that should be handled by
            try_split() and then call do_split() because we want to implement
            additional logic. In particular, we are considering the cases where
            we are shrinking a block that has a free block on the right, or no 
            right blocks at all.    */
        if((hdr->payload_size - payload_size_new) >= MIN_BLOCK_SIZE)
        {
            do_split(hdr, payload_size_new);

            /*  We might have a free block on the right to coalesce with.   */
            try_coalesce(hdr->hdr_next);

            /*  Also, there's a possibility we created a free block at the end 
                of the heap.    */
            if(hdr->hdr_next->hdr_next == NULL)
            {
                /*  This will make the heap shrink. */
                hfree(hdr->hdr_next);
            }
        }

        return payload_ptr;
    }





    /*  3. The block must grow and it's in the middle of the heap. Note that for
        the previous two if statements, we can safely assume that 
        payload_size_new > hdr->payload_size.   */
    if(hdr->hdr_next != NULL && hdr->hdr_next->is_free == 1 
    && hdr->payload_size + AL_HDR_SIZE + hdr->hdr_next->payload_size 
        >= payload_size_new)
    {
        /*  We use right coalescing to merge the next block, found to be free,
            to the current one, before "taking what we need" and trying 
            splitting.  */
        hdr = do_coalesce_right(hdr);
        
        try_split(hdr, payload_size_new);

        return payload_ptr;
    }
    




    /*  4. The block must grow and it's at the end of the heap. */
    if(hdr->hdr_next == NULL)
    {
        /*  We must make sure to only update the payload size if sbrk() didn't
            fail.   */
        if(sbrk((intptr_t)(payload_size_new - hdr->payload_size)) 
            != (void *)(-1))
        {
            hdr->payload_size = payload_size_new;
        }

        return payload_ptr;
    }





    /*  5. Fallback allocation case. The following code is executed when we are
        forced to copy all the data to a new (bigger) location. */

    void *payload_ptr_new = hmalloc(payload_size_new);

    if(payload_ptr_new == NULL)
    {
        return payload_ptr;
    }

    memcpy(payload_ptr_new, payload_ptr, hdr->payload_size);

    hfree(payload_ptr);

    return payload_ptr_new;
}



void *hreallocarray(void *p, size_t n_el_new, size_t size_el_new)
{
    /*  If an overflow would happen, we return a NULL pointer.  */
    if(size_el_new != 0 && n_el_new > SIZE_MAX / size_el_new)
    {
        return NULL;
    }

    return hrealloc(p, n_el_new * size_el_new);
}
