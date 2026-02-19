# hmalloc
A didactic heap memory allocator project to learn about memory management in user-space on Linux.

## Minimal implementation of hmalloc() and hfree()
The smallest possible working-ish version of these functions works with non-aligned memory blocks provided with a simple header.
Upon call, `hmalloc()` first tries to find a free block which is big enough, then calls `sbrk()` to shift the program break upwards.
All `hfree()` does – for now – is marking a block as free via its header; so, in particular, the program break is never lowered.

> [!Note]
> **Current limitations**: Memory alignment is not guaranteed and `hfree()` does not lower the program break when the block adjacent to the program break is freed.
> Moreover, there's neither block splitting nor coalescing.
