# hmalloc
A didactic heap memory allocator project to learn about memory management in user-space on Linux.

> [!Tip]
> This README is intended as a progress journal. Each paragraph has an associated commit linked in the title.

## [Minimal implementation](https://github.com/sizeof-dario/hmalloc/commit/85772acec980ebc011b0253488e7b8918c3aa94d) of `hmalloc()` and `hfree()`


The smallest possible working-ish version of these functions works with non-aligned memory blocks provided with a simple header.
Upon call, `hmalloc()` first tries to find a free block which is big enough, then calls `sbrk()` to shift the program break upwards.
All `hfree()` does – for now – is marking a block as free via its header; so, in particular, the program break is never lowered.

> [!Note]
> **Up-to-this-version limitations**: **Memory alignment is not guaranteed** and `hfree()` does not lower the program break when the block adjacent to the program break is freed or perform any verification upon its argument.
> Moreover, there's neither block splitting nor coalescing.

> [!Warning]
> Depending on the architecture, misaligned memory access can cause the kernel to send a SIGBUS signal whose default action is termination with core dumping.
> This would prevent this version from even being marked as "working-ish". However, misaligned memory is at least compatible with x86_64 architecture, with a performance penalty.

> [!Warning]
> Since `free()` doesn't check if its argument is a valid pointer, it could cause undefined behaviour or heap corruption if used incorrectly.

## [Memory alignment](https://github.com/sizeof-dario/hmalloc/commit/f2b3800) in `hmalloc()`

A working ~-ish~ allocator must ensure memory alignment in conformity with the C standard, to avoid undefined behaviour and a potential SIGBUS or any performance issue.
> [!Note]
> **Up-to-this-version limitations**: `free()` still wasn't updated and block splitting and coalescing are yet to be implemented.

## [Minimal working](https://github.com/sizeof-dario/hmalloc/commit/bc545a745b8675a0f378322dcf07dc294e700f9f) `free()`

`free()` needs to know when the memory block pointed by its argument is the last in the heap, in order to lower the program break upon freeing.
To achieve this, the block header is provided with a pointer to the next block, that is set to NULL for the last block.

A header with such pointer also allows `malloc()` to traverse the memory blocks as a linked list, for an easier manipulation. 

`hfree()` also checks if its argument is an invalid pointer, preventing heap corruption or any undefined behaviour, and if it points to a block that was already freed.
> [!Note]
> **Up-to-this-version limitations**: No block splitting and no coalescing.

