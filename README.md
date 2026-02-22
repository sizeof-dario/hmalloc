# hmalloc
A didactic heap memory allocator project to learn about memory management in user-space on Linux.

> [!Tip]
> This README is intended as a progress journal. Each paragraph has an associated commit linked in the title.

Current state of the project, as a minimal version:

<div align="center">

|`hmalloc()`          | `hfree()`                 | `hcalloc()`         | `hrealloc()`         | `hreallocarray()`         | 
|:--------------------|:--------------------------|:--------------------|:---------------------|:--------------------------|
| ✅ Memory alignment | ✅ Program break lowering | ❌ (absent)         | ❌ (absent)          | ❌ (absent)               |
| ✅ Block splitting  | ✅ Argument checking      |                     |                      |                           |
|                     | ❌ Block coalescing       |                     |                      |                           |

</div>

Unit tests: ❌ absent.

## [Minimal implementation](https://github.com/sizeof-dario/hmalloc/commit/85772ac) of `hmalloc()` and `hfree()`


The smallest possible working-ish version of these functions works with non-aligned memory blocks provided with a simple header.
Upon call, `hmalloc()` first tries to find a free block which is big enough, then, if that fails, it calls `sbrk()` to shift the program break upwards.
All `hfree()` does – for now – is marking a block as free, so, in particular, the program break is never lowered.

> [!Note]
> **Up-to-this-version limitations**: **Memory alignment is not guaranteed** and `hfree()` does not lower the program break when the block adjacent to the program break is freed or perform any verification upon its argument.
> Moreover, there's neither block splitting nor coalescing.

> [!Warning]
> Depending on the architecture, misaligned memory access can cause the kernel to send a SIGBUS signal whose default action is termination with core dumping.
> This would prevent this version from even being marked as "working-ish". However, misaligned memory is at least compatible with x86_64 architecture, with a performance penalty.

> [!Warning]
> Since `free()` doesn't check if its argument is a valid pointer, it could cause undefined behaviour or heap corruption if used incorrectly.

## [Memory alignment](https://github.com/sizeof-dario/hmalloc/commit/f2b3800) in `hmalloc()`

A _working_ allocator must ensure memory alignment in conformity with the C standard, to avoid undefined behaviour and a potential SIGBUS or any performance issue.
> [!Note]
> **Up-to-this-version limitations**: `free()` still wasn't updated and block splitting and coalescing are yet to be implemented.

## [Minimal working](https://github.com/sizeof-dario/hmalloc/commit/6c7b10b) `free()`

`free()` needs to know when the memory block pointed by its argument is the last in the heap, in order to lower the program break upon freeing.
To achieve this, the block header is provided with a pointer to the next block, that is set to `NULL` for the last block.

A header with such pointer also allows `malloc()` to traverse the memory blocks as a linked list, for an easier manipulation. 

`hfree()` also checks if its argument is an invalid pointer, preventing heap corruption or any undefined behaviour, and if it points to a block that was already freed.
> [!Note]
> **Up-to-this-version limitations**: No block splitting and no coalescing.

> [!Warning]
> **Bug**: `free()` misbehaves when the last block in the heap is freed when one or more blocks before it are already free. The function is supposed to never leave a free block at the end of the heap.
> However, upon lowering the program break when the last block gets freed, it doesn't check if the second-to-last block was marked as free (which should make the program break get lowered more), so a free block is left on top of the heap.

## Block splitting in `hmalloc()`
When implementing an allocator, it's important to prevent memory fragmentation. That is, involuntarily subdividing the heap in so many small blocks that they become virtually unusable, potentially making a lot of free space get wasted.
Block splitting takes care of internal fragmentation (unusable space inside a single whole memory block), still leaving external fragmentation.

To introduce block splitting, the header was provided with a pointer to the previous block. This is not always the best solution for an allocator, because it increases its header size. However, it is the easiest didactically speaking.
>[!Note]
> **Up-to-this-version limitations**: Still no coalescing.

> [!Note]
> **Bug fix:** `free()` now works correctly, with the implementation of heap trimming.
