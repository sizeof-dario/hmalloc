#include "hmalloc.h"

static void *hstart = NULL;

void *hmalloc(size_t size)
{
    // hmalloc() does different things depending on weather it's called for the
    // first time or not, so we use a local variable to track that. By default,
    // we assume hmalloc() has been alreay called before. If not, firstcall is
    // set to 1 when checking if hstart is set.
    int firstcall = 0;

    // We must guarantee alignment for a well defined behaviour according to
    // the C standard.
    size_t al_size = ALIGN(size);
    size_t al_mbhsize = ALIGN(sizeof(mbheader));

    // hamlloc() retrieves the virtual address of the heap start when called
    // for the first time, and stores it in hstart. firstcall is also updated.
    if(hstart == NULL)
    {
        hstart = sbrk(0);
        firstcall = 1;
    }

    // hmalloc() first scans the heap for a block of memory that is big enough
    // to contain size bytes of data. It will eventually move the program break
    // up if it finds none.
    void *curr = hstart;
    void *last = NULL; // Used later to link memory blocks.
    while(curr != NULL && !firstcall)
    {
        if(((mbheader *)curr)->free && ((mbheader *)curr)->size >= al_size)
        {
            ((mbheader *)curr)->free = 0;
            return (char *)curr + al_mbhsize;
        }
        last = curr;
        curr = ((mbheader *)curr)->next;
    }

    // Code above: searching for a big enough block.
    // COde below: raising the proram break.

    void *p = sbrk(al_size + al_mbhsize);
    
    // There's a possibility sbrk() fails.
    if(p == (void *)-1)
    {
        return NULL;
    }

    // If this is not the first time hmalloc() got called, we need to link the
    // previous memory block to the current new one.
    if(!firstcall)
    {
        ((mbheader *)last)->next = p;
    }

    // We must initialize the block header.
    ((mbheader *)p)->free = 0;
    ((mbheader *)p)->size = al_size;
    ((mbheader *)p)->next = NULL;

    return (char *)p + al_mbhsize;
}

void hfree(void *p)
{
    size_t al_mbhsize = ALIGN(sizeof(mbheader));

    // If hfree() receives a NULL pointer, the expected behaviour is to do
    // nothing.
    if(p != NULL)
    {
        // hfree() first checks if p is a valid pointer by comparing it, with
        // the right offset, to every possible valid mbheader pointer. If it
        // fails to do so, it does nothing.
        
        mbheader *hbptr = (mbheader *)((char *)p - al_mbhsize);
        mbheader *curr = (mbheader *)hstart;
        mbheader *prev = NULL; // Used later.
        while(curr != hbptr)
        {
            prev = curr;
            curr = curr->next;
            if(curr == NULL)
            {
                // It means we reached the end of the heap without finding a
                // valid match.
                return;
            }
        }

        // hfree() must also check if p points to an aleady freed block for p
        // to be considered valid.
        if(hbptr->free)
        {
            return;
        }

        // Next section is executed if p is valid:

        // If hfree() receives a pointer to the last block in the heap, it
        // decreases the program break and sets to NULL the next pointer of the
        // new last block header. Otherwise, it just marks the block as free
        // for future reusage.
        if(((mbheader *)hbptr)->next == NULL)
        {
            sbrk(-(hbptr->size + al_mbhsize));
            if(prev != NULL)
            {
                prev->next = NULL;
            }
            else
            {
                hstart = NULL;
            }
        }
        else
        {
            hbptr->free = 1;
        }      
    }
}
