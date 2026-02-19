#include "hmalloc.h"

static void *hstart = NULL;

void *hmalloc(size_t size)
{
    // hamlloc retrieves the virtual address of the heap start when called for
    // the first time, and stores it in hstart.
    if(hstart == NULL)
    {
        hstart = sbrk(0);
    }

    // hmalloc first scans the heap for a block of memory that is big enough to
    // contain size bytes of data. It will eventually move the program break up
    // if it finds none.
    void *progbrk = sbrk(0);
    void *curr = hstart;
    while(curr < progbrk)
    {
        if(((mbheader *)curr)->free && ((mbheader *)curr)->size >= size)
        {
            ((mbheader *)curr)->free = 0;
            return (char *)curr + sizeof(mbheader);
        }

        curr = (char *)curr + sizeof(mbheader) + ((mbheader *)curr)->size;
    }

    // Above, searching for a big enough block. Below, raising the prog. break

    void *p = sbrk(size + sizeof(mbheader));
    
    // There's a possibility sbrk() fails.
    if(p == (void *)-1)
    {
        return NULL;
    }

    // We must initialize the mbheader.
    ((mbheader *)p)->free = 0;
    ((mbheader *)p)->size = size;

    return (char *)p + sizeof(mbheader);
}

void hfree(void *p)
{
    if(p != NULL)
    {
        ((mbheader *)((char *)p - sizeof(mbheader)))->free = 1;
    }
}
