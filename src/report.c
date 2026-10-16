/*
 * report.c: yours. Every line the program writes to standard output.
 *
 * The manual's "The Report" section states each line and each field. Nothing
 * here may depend on the host: every address is an offset into the arena, or a
 * physical address the segment table describes.
 */
#include "ps2.h"

void ps2_report_arena(FILE *out, const ps2_script *script)
{
    /* TODO: Write the arena line.
       Give the arena size, the alignment, the header, and the split threshold.
       Give the policy word the command line chose.
       cases/arena/first_block.script
       See "The Machine Lines" in the manual. */

    (void)out;
    (void)script;
}

void ps2_report_segments(FILE *out, const ps2_script *script)
{
    /* TODO: Write one line for each of the three segment rows.
       Keep the table order: code, then heap, then stack.
       Give the name, the base, the bound, the growth word, and the protection field.
       cases/trans/read_only.script
       See "The Machine Lines" in the manual. */

    (void)out;
    (void)script;
}

void ps2_report_alloc(FILE *out, const ps2_allocation *record)
{
    /* TODO: Write the allocation line.
       Give the handle, the request, the address, and the block.
       cases/arena/first_block.script
       See "The Event Lines" in the manual. */

    (void)out;
    (void)record;
}

void ps2_report_release(FILE *out, const ps2_allocation *record)
{
    /* TODO: Write the release line.
       Give the handle, the address, and the block's size before any merge.
       cases/worked/first_fit.script
       See "The Event Lines" in the manual. */

    (void)out;
    (void)record;
}

void ps2_report_release_at(FILE *out, const ps2_allocation *record)
{
    /* TODO: Write the freeat line.
       Give the address and the block's size before any merge.
       cases/own/release_by_address.script
       See "The Event Lines" in the manual. */

    (void)out;
    (void)record;
}

void ps2_report_touch(FILE *out, const char *handle, long address)
{
    /* TODO: Write the touch line.
       Give the handle and the address it reads.
       cases/own/touch_a_live_handle.script
       See "The Event Lines" in the manual. */

    (void)out;
    (void)handle;
    (void)address;
}

void ps2_report_dump(FILE *out, const long *offsets, const long *sizes,
                     size_t count)
{
    /* TODO: Write one dump line that lists every free block.
       Give each block's offset and its size as a pair, in address order.
       Separate the pairs with one space.
       cases/coalesce/merge_next.script
       See "The Dump Line" in the manual. */

    /* TODO: Write "dump none" when the free list is empty.
       cases/arena/exact_arena.script
       See "The Dump Line" in the manual. */

    (void)out;
    (void)offsets;
    (void)sizes;
    (void)count;
}

void ps2_report_translate(FILE *out, ps2_segment_id segment, long offset,
                          ps2_operation op, long address,
                          ps2_translation result)
{
    /* TODO: Write the translation line.
       Give the segment name, the offset, the operation, and the address.
       cases/trans/code_bounds.script
       See "The Event Lines" in the manual. */

    /* TODO: Write "fault bounds" for an offset outside the segment.
       Write "fault protect" for an operation the segment rejects.
       cases/trans/protect_bits.script
       See "The Event Lines" in the manual. */

    (void)out;
    (void)segment;
    (void)offset;
    (void)op;
    (void)address;
    (void)result;
}

void ps2_report_summary(FILE *out, const ps2_summary *summary)
{
    /* TODO: Write the summary line.
       Give the allocated, free, largest, blocks, and internal totals.
       cases/frag/internal_waste.script
       See "The Summary Line" in the manual. */

    /* TODO: Write the external ratio with six decimals.
       Divide the free bytes the largest block cannot reach by the free total.
       Write 0.000000 when nothing is free.
       cases/frag/external_holes.script
       See "The Summary Line" in the manual. */

    (void)out;
    (void)summary;
}

void ps2_report_result(FILE *out, bool faulted)
{
    /* TODO: Write the result line.
       Write "result complete" after a whole run.
       Write "result faulted" after a fault.
       cases/full/runs_out.script
       See "The Result Line" in the manual. */

    (void)out;
    (void)faulted;
}
