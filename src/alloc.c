/*
 * alloc.c: yours. The free list over the arena.
 *
 * The free list links blocks inside the arena. The list stays in address
 * order. See "The Free List", "Splitting", "Coalescing", and "The Policies"
 * in the manual.
 */
#include "ps2.h"

#include <stdio.h>

void ps2_alloc_init(ps2_run *run)
{
    /* TODO: Write the arena's whole size into the first block.
       Set the first block's link to -1.
       The free list starts with that one block at offset 0.
       See "The Free List" in the manual. */

    /* TODO: Record no allocation yet.
       Set run->handles to 0.
       See "Ownership" in the manual. */

    (void)run;
}

ps2_status ps2_alloc_allocate(ps2_run *run, const char *handle, long size,
                              ps2_allocation *out, ps2_diag *diag)
{
    /* TODO: Measure the request.
       Add the header to the size rounded up to the alignment.
       See "The Alignment Rule" in the manual. */

    /* TODO: Choose a free block with the policy the script carries.
       First fit takes the lowest address that fits.
       cases/policy/first_fit.script
       See "First Fit" in the manual. */

    /* TODO: Best fit takes the smallest block that fits.
       A tie goes to the lower address.
       cases/policy/best_fit.script
       See "Best Fit" in the manual. */

    /* TODO: Next fit starts at the cursor.
       Wrap to the lowest address when that search fails.
       cases/policy/next_fit.script
       See "Next Fit" in the manual. */

    /* TODO: Refuse the request when no free block fits.
       Write "ps2: alloc <handle>: no free block holds <needed> bytes".
       cases/full/runs_out.script
       See "When the Arena Runs Out" in the manual. */

    /* TODO: Split the chosen block when the remainder holds 32 bytes or more.
       Leave the remainder on the list as a new block.
       cases/arena/split_at_min.script
       See "Splitting" in the manual. */

    /* TODO: Hand the whole block over when the remainder is smaller than 32 bytes.
       cases/arena/no_split_below_min.script
       See "Splitting" in the manual. */

    /* TODO: Unlink the chosen block from the free list.
       cases/arena/first_block.script
       See "The Free List" in the manual. */

    /* TODO: Record the offset just handed out as the next fit cursor.
       cases/policy/tie_cursor_next.script
       See "Next Fit" in the manual. */

    /* TODO: Record the handle, the request, the block, and the address.
       Set the address to the block's offset plus the header.
       Set the handle live.
       cases/arena/first_block.script
       See "What the Allocator Hands Out" in the manual. */

    (void)run;
    (void)handle;
    (void)size;
    (void)out;

    snprintf(diag->text, sizeof diag->text, "ps2: alloc.c is not implemented");
    return PS2_ERR_FAULT;
}

ps2_status ps2_alloc_release(ps2_run *run, const char *handle,
                             ps2_allocation *out, ps2_diag *diag)
{
    /* TODO: Find the handle's record.
       The script already checked that an earlier line allocates that handle.
       See "Ownership" in the manual. */

    /* TODO: Refuse a handle that is already released.
       Write "ps2: free <handle>: handle '<handle>' was already released".
       cases/own/double_free.script
       See "Ownership" in the manual. */

    /* TODO: Mark the handle released.
       Put its block back on the free list.
       cases/coalesce/at_the_start.script
       See "Coalescing" in the manual. */

    (void)run;
    (void)handle;
    (void)out;

    snprintf(diag->text, sizeof diag->text, "ps2: alloc.c is not implemented");
    return PS2_ERR_FAULT;
}

ps2_status ps2_alloc_release_at(ps2_run *run, long address,
                                ps2_allocation *out, ps2_diag *diag)
{
    /* TODO: Find a live block at the address the allocator handed out.
       Write "ps2: freeat <address>: no block was handed out at <address>".
       cases/own/foreign_pointer.script
       See "Freeing by Address" in the manual. */

    /* TODO: Refuse an address whose block is already free.
       Write "ps2: freeat <address>: the block at <address> was already released".
       cases/own/double_free_by_address.script
       See "Freeing by Address" in the manual. */

    /* TODO: Mark the handle released.
       Put its block back on the free list.
       cases/own/release_by_address.script
       See "Coalescing" in the manual. */

    (void)run;
    (void)address;
    (void)out;

    snprintf(diag->text, sizeof diag->text, "ps2: alloc.c is not implemented");
    return PS2_ERR_FAULT;
}

ps2_status ps2_alloc_touch(ps2_run *run, const char *handle, long *address,
                           ps2_diag *diag)
{
    /* TODO: Refuse a read through a handle that is not live.
       Write "ps2: touch <handle>: handle '<handle>' was released".
       cases/own/use_after_free.script
       See "Ownership" in the manual. */

    /* TODO: Write the live block's address to *address.
       cases/own/touch_a_live_handle.script
       See "Ownership" in the manual. */

    (void)run;
    (void)handle;
    (void)address;

    snprintf(diag->text, sizeof diag->text, "ps2: alloc.c is not implemented");
    return PS2_ERR_FAULT;
}

size_t ps2_alloc_snapshot(const ps2_run *run, long *offsets, long *sizes,
                          size_t cap)
{
    /* TODO: Walk the free list from its first block.
       Copy each block's offset and size into the caller's arrays.
       Stop at the last block or at cap entries.
       Return how many blocks were copied.
       cases/coalesce/merge_next.script
       See "The Free List" in the manual. */

    (void)run;
    (void)offsets;
    (void)sizes;
    (void)cap;

    return 0;
}

void ps2_alloc_summary(const ps2_run *run, ps2_summary *out)
{
    /* TODO: Report the bytes the live blocks hold as allocated.
       Report the bytes the free list holds as free.
       See "The Summary Line" in the manual. */

    /* TODO: Report the largest free block and the number of free blocks.
       cases/frag/external_holes.script
       See "The Summary Line" in the manual. */

    /* TODO: Report the bytes wasted inside the live blocks.
       Subtract the header and the rounded-up request from each live block.
       cases/frag/internal_waste.script
       See "The Summary Line" in the manual. */

    /* TODO: Walk the free list once.
       cases/frag/ratio_goes_to_zero.script
       See "The Free List" in the manual. */

    (void)run;

    out->allocated = 0;
    out->free = 0;
    out->largest = 0;
    out->blocks = 0;
    out->internal = 0;
}
