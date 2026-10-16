/*
 * script.c: yours. Read and validate the script file.
 *
 * The manual's "The Script File" section defines the grammar. Its refusal
 * table lists every message. The parser reads and checks the whole file before
 * it runs a request. A bad line anywhere produces no output.
 *
 *   ps2_script_load fills *out with the machine rows and the requests, in
 *   file order, and reports a refusal through *diag.
 */
#include "ps2.h"

#include <stdio.h>

ps2_status ps2_script_load(const char *path, ps2_policy policy, ps2_script *out,
                           ps2_diag *diag)
{
    /* TODO: Open the script file for reading.
       On failure, write "ps2: cannot open script '<path>'" to *diag.
       Return PS2_ERR_REJECTED.
       cases/parse/missing_script.script
       See "The Script File" in the manual. */

    /* TODO: Read the file one line at a time.
       Refuse a line longer than 511 bytes.
       Write "ps2: script line N is too long".
       cases/parse/long_line.script
       See "The Script File" in the manual. */

    /* TODO: Cut each line at the first '#'.
       Ignore the text after it.
       Skip a line that holds nothing after the cut.
       cases/arena/with_comments.script
       See "The Script File" in the manual. */

    /* TODO: Reject a word outside the grammar.
       Write "ps2: script line N: unknown directive '<word>'".
       cases/parse/unknown_directive.script
       See "The Script File" in the manual. */

    /* TODO: Require the arena row before any other row.
       Write "ps2: script line N: the script opens with 'arena <bytes>'".
       cases/parse/arena_first.script
       See "The Script File" in the manual. */

    /* TODO: Refuse a second arena row.
       Write "ps2: script line N: the arena is already set".
       cases/parse/arena_twice.script
       See "The Script File" in the manual. */

    /* TODO: Read the arena size as a multiple of 8.
       Accept 64 to 65536 bytes.
       Write "ps2: script line N: arena must be a multiple of 8, from 64 to 65536".
       cases/parse/arena_size.script
       See "The Alignment Rule" in the manual. */

    /* TODO: Refuse an arena row after the first request.
       Write "ps2: script line N: the arena comes before the first request".
       cases/parse/arena_late.script
       See "The Script File" in the manual. */

    /* TODO: Read the three segment rows in the order code, heap, stack.
       Write "ps2: script line N: the segment table lists code, heap, and stack in that order".
       cases/parse/seg_order.script
       See "The Segment Rows" in the manual. */

    /* TODO: Read the base, the bound, the growth word, and the protection field.
       Write "ps2: script line N: base must be 0 or greater".
       cases/parse/seg_base.script
       See "The Segment Rows" in the manual. */

    /* TODO: Refuse a base and a bound that run past 1048576 bytes.
       cases/parse/seg_phys.script
       See "The Segment Rows" in the manual. */

    /* TODO: Refuse a growth word that is not up or down.
       Write "ps2: script line N: growth must be up or down".
       cases/parse/seg_growth.script
       See "The Segment Rows" in the manual. */

    /* TODO: Refuse a protection field that is not three characters.
       Accept r, w, x, and '-' only.
       cases/parse/seg_prot.script
       See "The Protection Field" in the manual. */

    /* TODO: Refuse a segment row after the table is complete.
       Write "ps2: script line N: the segment table is already complete".
       cases/parse/seg_four.script
       See "The Script File" in the manual. */

    /* TODO: Refuse a request before the three segment rows exist.
       Write "ps2: script line N: the segment table needs code, heap, and stack".
       cases/parse/table_early.script
       See "The Script File" in the manual. */

    /* TODO: Read a handle name of 1 to 15 characters.
       Start it with a letter, then letters, digits, '_', or '-'.
       Write "ps2: script line N: '<name>' is not a handle name".
       cases/parse/handle_name.script
       See "The Requests" in the manual. */

    /* TODO: Refuse a handle name an earlier line already holds.
       Write "ps2: script line N: duplicate handle '<name>'".
       cases/parse/handle_dup.script
       See "The Script File" in the manual. */

    /* TODO: Refuse a request size below 1 or above the arena.
       Write "ps2: script line N: size must be from 1 to <arena>".
       cases/parse/alloc_size.script
       See "The Script File" in the manual. */

    /* TODO: Require free or touch to refer to a handle that an earlier line allocates.
       Write "ps2: script line N: no such handle '<name>'".
       cases/parse/handle_unknown.script
       See "Ownership" in the manual. */

    /* TODO: Read a freeat address as a count of 0 or more.
       Write "ps2: script line N: address must be 0 or greater".
       cases/parse/freeat_address.script
       See "Freeing by Address" in the manual. */

    /* TODO: Read a translate request.
       Refuse a name that is not a segment.
       Refuse an offset below 0.
       Refuse an operation that is not read, write, or exec.
       cases/parse/operation.script
       See "The Script File" in the manual. */

    /* TODO: Refuse a dump line that carries a field.
       Write "ps2: script line N: dump takes no fields".
       cases/parse/dump_fields.script
       See "The Script File" in the manual. */

    /* TODO: Refuse a script that holds no requests.
       Write "ps2: script holds no requests".
       cases/parse/no_requests.script
       See "The Script File" in the manual. */

    /* TODO: Store each request in the order the file gives it.
       Set out->count to the number of requests.
       Return PS2_OK.
       cases/worked/first_fit.script
       See "The Script File" in the manual. */

    (void)path;
    (void)policy;
    (void)out;

    snprintf(diag->text, sizeof diag->text, "ps2: script.c is not implemented");
    return PS2_ERR_REJECTED;
}
