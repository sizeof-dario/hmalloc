#include "hmalloc.h"

// Static variable keeping track of the heap starting address.
// A value of `NULL` indicates that the heap starting address is at its lowest.
static void *hstart = NULL;

void *hmalloc(size_t size)
{
    // `hmalloc()` does slightly different things depending on whether the
    // program break currently coincides with the heap start or not, so we use
    // `firstcall` to keep track of that. By default, we assume the heap start
    // and the program break do not coincide, assigning to `firstcall` a value
    // of `0`.
    //
    // Note that the name `firstcall` doesn't mean the variable gets set to `1`
    // only if `hmalloc()` is getting called for the first time. It only hints
    // to the fact that's a trivial case in which the value does become `1`. 
    int firstcall = 0;

    // We must guarantee alignment for a well defined behaviour according to
    // the C standard. Thus, we round up both the `size` argument value and the
    // memory block header size to the closest multiple of the alignment
    // required by the greatest standard type with no alignment specifiers.

    // Aligned `size` argument.
    size_t al_blocksize = ALIGN(size);
    // Aligned memory block header size.
    size_t al_headersize = ALIGN(sizeof(mbheader));

    // `hamlloc()` retrieves the virtual address of the heap start when called
    // while `hstart` is `NULL`, and updates the variable value, along with
    // `firstcall`.
    //
    // Note that calling `sbrk(0)` to update `hstart` is really only needed
    // upon the first `hmalloc()` call. However, code simplicity is here
    // preferred over perfect optimization.
    if(hstart == NULL)
    {
        hstart = sbrk(0);
        firstcall = 1;
    }

    // `hmalloc()` first scans the heap for a block of memory that is big
    // enough to contain `al_blocksize` bytes of data. It will eventually move
    // the program break up if it finds none.

    // Used to traverse the headers list.
    void *hcurr = hstart;
    // Used to link memory block headers.
    void *hlast = NULL;

    // Searching for a big enough block.
    while(hcurr != NULL && !firstcall)
    {
        // If we find a big enough free block, we mark it as occupied and
        // return the block address.
        if(((mbheader *)hcurr)->free 
            && ((mbheader *)hcurr)->blocksize >= al_blocksize)
        {
            ((mbheader *)hcurr)->free = 0;
            return ((char *)hcurr + al_headersize);
        }

        // We save the current header pointer in case we exit the loop, meaning
        // it was associated with the last memory block. In this way, we can
        // access it and link it to the soon newly added block via `sbrk()`.
        hlast = hcurr;
        
        hcurr = ((mbheader *)hcurr)->next;
    }

    // Following code is executed if we failed to find a suitable block to our
    // needs. In particular, `sbrk()` raises the program break and the returned
    // pointer is used to calculate the block address to return.

    // Pointer to the header of the newly allocated block.
    void *p = sbrk(al_blocksize + al_headersize);
    
    // There's a possibility `sbrk()` fails.
    if(p == (void *)-1)
    {
        return NULL;
    }

    // If `sbrk()` didn't fail, we must initialize the block header fields.
    ((mbheader *)p)->free = 0;
    ((mbheader *)p)->blocksize = al_blocksize;
    ((mbheader *)p)->next = NULL;

    // Also, if the block allocated is not the first in the heap, we need to
    // link the previous memory block to it.
    if(!firstcall)
    {
        ((mbheader *)hlast)->next = p;
    }

    return (char *)p + al_headersize;
}

void hfree(void *p)
{
    // Aligned memory block header size.
    size_t al_headersize = ALIGN(sizeof(mbheader));

    // If `hfree()` receives a `NULL` pointer, the expected behaviour is to do
    // nothing.
    if(p != NULL)
    {
        // hfree() first checks if `p` is a valid pointer by comparing it, with
        // the right offset, to every possible valid header pointer. If it
        // fails to do so, it does nothing.
        
        // Pointer to the header of the block pointed by `p`.
        mbheader *p_header_ptr = (mbheader *)((char *)p - al_headersize);

        // Pointer used to traverse the header list.
        mbheader *curr_header_ptr = (mbheader *)hstart;

        // Pointer used to possibly update `next` in the second-to-last block
        // in the heap if the last block gets freed.
        mbheader *prev_header_ptr = NULL;

        // We traverse the headers list looking for the header associated to
        // `p`. Not finding it would imply that the pointer is invalid.
        while(curr_header_ptr != p_header_ptr)
        {
            prev_header_ptr = curr_header_ptr;
            curr_header_ptr = curr_header_ptr->next;
            if(curr_header_ptr == NULL)
            {
                // It means we reached the end of the heap without finding a
                // valid match.
                return;
            }
        }

        // `hfree()` must also make sure that `p` doesn't point to an already
        // freed block for it to be considered valid.
        if(p_header_ptr->free)
        {
            return;
        }

        // We can now assume `p` is valid.

        // If `hfree()` receives a pointer to the last block in the heap, it
        // decreases the program break and sets to `NULL` the `next` pointer of
        // the new last header. Otherwise, it just marks the block as free for 
        // future reusage.
        if(((mbheader *)p_header_ptr)->next == NULL)
        {
            // This call to `sbrk()` will assumed not to fail.
            sbrk(-(p_header_ptr->blocksize + al_headersize));

            // We have to make sure we are not freeing the first block of the
            // heap before trying to access `prev_header_ptr`. Also, if we are
            // freeing such block, we reset `hstart` back to `NULL`.
            if(prev_header_ptr != NULL)
            {
                prev_header_ptr->next = NULL;
            }
            else
            {
                hstart = NULL;
            }
        }
        else
        {
            p_header_ptr->free = 1;
        }      
    }
}
