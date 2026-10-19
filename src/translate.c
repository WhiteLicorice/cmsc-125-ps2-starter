/*
 * translate.c: yours. The segment table.
 *
 * Each row holds a physical base, a bound, a growth direction, and three
 * protection characters in the order read, write, execute. See "The Segment
 * Table", "Growth Direction", and "The Faults" in the manual.
 */
#include "ps2.h"

ps2_translation ps2_translate(const ps2_script *script, ps2_segment_id segment,
                              long offset, ps2_operation op, long *address)
{
    /* TODO: Read the row selected by the request.
       The script's three requests carry code, heap, and stack.
       See "The Table" in the manual. */

    /* TODO: Compare the offset with the row's bound.
       Return PS2_TR_BOUNDS when the offset is outside it.
       cases/trans/code_bounds.script
       See "The Two Faults" in the manual. */

    /* TODO: Check the row's protection field for the operation.
       Return PS2_TR_PROTECT when the field rejects the operation.
       cases/trans/protect_bits.script
       See "The Protection Field" in the manual. */

    /* TODO: Check the bound before the protection field.
       A fault outside the segment is a bounds fault.
       cases/trans/bounds_before_protect.script
       See "The Two Faults" in the manual. */

    /* TODO: Add the offset to the base for an upward row.
       cases/trans/every_growth.script
       See "Two Growth Directions" in the manual. */

    /* TODO: Subtract the offset from the row's highest byte for a downward row.
       cases/trans/stack_shrinks.script
       See "Two Growth Directions" in the manual. */

    /* TODO: Write the physical address to *address.
       Return PS2_TR_OK.
       cases/worked/translation.script
       See "The Table" in the manual. */

    (void)script;
    (void)segment;
    (void)offset;
    (void)op;
    (void)address;

    return PS2_TR_BOUNDS;
}
