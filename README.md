# hmalloc
A didactic heap memory allocator project to learn about memory management in user-space on Linux.

> [!Tip]
> This README is intended as a progress journal. Each paragraph has an associated commit linked in the title.

## [Minimal implementation](https://github.com/sizeof-dario/hmalloc/commit/85772acec980ebc011b0253488e7b8918c3aa94d) of `hmalloc()` and `hfree()`


The smallest possible working-ish version of these functions works with non-aligned memory blocks provided with a simple header.
Upon call, `hmalloc()` first tries to find a free block which is big enough, then calls `sbrk()` to shift the program break upwards.
All `hfree()` does – for now – is marking a block as free via its header; so, in particular, the program break is never lowered.

> [!Note]
> **Up-to-this-version limitations**: **Memory alignment is not guaranteed** and `hfree()` does not lower the program break when the block adjacent to the program break is freed.
> Moreover, there's neither block splitting nor coalescing.

> [!Warning]
> Depending on the achitecture, misalligned memory access can cause the kernel to send a SIGBUS signal whose default action is termination with core dumping.
> This would prevent this version from even being marked as "working-ish". However, misalligned memory is at least compatible with x86_64 architecture, with a performance penalty.

## [Memory alignment](https://github.com/sizeof-dario/hmalloc/commit/f2b3800) in `hmalloc()`

A working ~-ish~ allocator must ensure memory alignment in conformity with the C standard, to avoid undefined behaviour and a potential SIGBUS or any performance issue.
> [!Note]
> **Up-to-this-version limitations**: The program break is still never lowered and block splitting and coalescing are yet to be implemented.
