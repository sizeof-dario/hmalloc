# hmalloc
A didactic heap memory allocator project to learn about memory management in user-space on Linux.

> [!Tip]
> This README is intended as a progress journal. Each paragraph has an associated commit linked in the title.

Current state of the project, as a minimal version:

<div align="center">

| ✅ `hmalloc()`   | ✅ `hfree()`           | ✅ `hcalloc()`        | ✅ `hrealloc()`     | ✅ `hreallocarray()` | 
|:-----------------|:-----------------------|:----------------------|:--------------------|:---------------------|
| Memory alignment | Program break lowering | Overflow safe         | Edge cases checking | Overflow safe        |
| Block splitting  | Arguments checking     | Memory initialization | In place shrinking  |                      |
| Overflow safe    | Block coalescing       |                       | In place growing    |                      |
|                  |                        |                       | Fallback allocation |                      |

</div>

Unit tests: ❌ (absent).

## [Minimal implementation](https://github.com/sizeof-dario/hmalloc/commit/85772ac) of `hmalloc()` and `hfree()`


The smallest possible working-ish version of these functions works with non-aligned memory blocks provided with a simple header.
Upon call, `hmalloc()` first tries to find a free block which is big enough, then, if that fails, it calls `sbrk()` to shift the program break upwards. When receiving an argument of `0`, `hmalloc()` still returns a non-`NULL` pointer that can be passed to `hfree()` and that should never be used.
All `hfree()` does – for now – is marking a block as free, so, in particular, the program break is never lowered.

> [!Note]
> **Up-to-this-version limitations**: **Memory alignment is not guaranteed** and `hfree()` does not lower the program break when the block adjacent to the program break is freed or perform any verification upon its argument.
> Moreover, there's neither block splitting nor coalescing.

> [!Warning]
> Depending on the architecture, misaligned memory access can cause the kernel to send a SIGBUS signal whose default action is termination with core dumping.
> This would prevent this version from even being marked as "working-ish". However, misaligned memory is at least compatible with x86_64 architecture, with a performance penalty.

> [!Warning]
> Since `hfree()` doesn't check if its argument is a valid pointer, it could cause undefined behaviour or heap corruption if used incorrectly.

## [Memory alignment](https://github.com/sizeof-dario/hmalloc/commit/f2b3800) in `hmalloc()`

A _working_ allocator must ensure memory alignment in conformity with the C standard, to avoid undefined behaviour and a potential SIGBUS or any performance issue.
> [!Note]
> **Up-to-this-version limitations**: `hfree()` still wasn't updated and block splitting and coalescing are yet to be implemented.

## [Minimal working](https://github.com/sizeof-dario/hmalloc/commit/6c7b10b) `hfree()`

`hfree()` needs to know when the memory block pointed by its argument is the last in the heap, in order to lower the program break upon freeing.
To achieve this, the block header is provided with a pointer to the next block, that is set to `NULL` for the last block.

A header with such pointer also allows `hmalloc()` to traverse the memory blocks as a linked list, for an easier manipulation. 

`hfree()` also checks if its argument is an invalid pointer, preventing heap corruption or any undefined behaviour, and if it points to a block that was already freed.
> [!Note]
> **Up-to-this-version limitations**: No block splitting and no coalescing.

> [!Warning]
> **Bug**: `hfree()` misbehaves when the last block in the heap is freed when one or more blocks before it are already free. The function is supposed to never leave a free block at the end of the heap.
> However, upon lowering the program break when the last block gets freed, it doesn't check if the second-to-last block was marked as free (which should make the program break get lowered more), so a free block is left on top of the heap.

## [Block splitting](https://github.com/sizeof-dario/hmalloc/commit/e0fbcfb) in `hmalloc()`
When implementing an allocator, it's important to prevent memory fragmentation. That is, involuntarily subdividing the heap in so many small blocks that they become virtually unusable, potentially making a lot of free space get wasted.
Block splitting takes care of internal fragmentation (unusable space inside a single whole memory block), still leaving external fragmentation.

To introduce block splitting, the header was provided with a pointer to the previous block. This is not always the best solution for an allocator, because it increases its header size. However, it is the easiest didactically speaking.
>[!Note]
> **Up-to-this-version limitations**: Still no coalescing.

> [!Note]
> **Bug fix:** `hfree()` now works correctly, with the implementation of cascading heap trimming.

## [Block coalescing](https://github.com/sizeof-dario/hmalloc/commit/2ed6f95) in `hfree()`

Block coalescing was implemented to reduce external fragmentation (that is, the unwanted creation of small free blocks all adjacent to each other). Cascading heap trimming is not necessary anymore since coalescing makes sure there can only be one free block at the top of the heap.

> [!Tip]
> At this point, `hmalloc()` and `hfree()` are supposed to be a minimal working allocator[^1].

## [Working](https://github.com/sizeof-dario/hmalloc/commit/7821367) `hcalloc()`

`hcalloc()` has been included in the allocator. It was chosen for it to still return a non-`NULL` pointer if one or both of its argument are `0`. Such pointer can still be passed to `hfree()` and should never be used.

## [Minimal working](https://github.com/sizeof-dario/hmalloc/commit/2c81caf) `hrealloc()`

A minimal version of `hrealloc()` was implemented. It checks for the edge cases (if its arguments are `NULL` or `0`) and always reallocates the block. This version is intentionally simplistic and far from finished, since it should distinguish between shrinking and enlarging a block of memory, and decide what to do in the second case depending on whether there is free space around the block or if it's at the end of the heap.

## [Working](https://github.com/sizeof-dario/hmalloc/commit/950fbd7) `hrealloc()`
Now, `hrealloc()` performs different operations depending on whether the requested size is greater or lesser than the original one. Moreover, if the new size is smaller, the functions shrinks the block in place; if it is bigger, it first tries local changes to the heap structure and only reallocates as a fallback strategy if anything else fails.

## [Working](https://github.com/sizeof-dario/hmalloc/commit/8a0ba42) `hreallocarray()`
The function was included for a complete minimal allocator compatible with the functions available in a POSIX environment. Remember that the stdlib function `reallocarray()` is not ISO C and has in fact Feature Test Macro Requirements.
> [!Tip]
> At this point, the minimal version of each function is complete. However, we can do some optimization (like using `mmap()` for big blocks in `hmalloc()`) or implement unit tests. Such features are expected.

[^1]: `hmalloc()` should perform an overflow checking. However, to this version, it does not. This gets fixed when `hrealloc()` is introduced for the first time. 
