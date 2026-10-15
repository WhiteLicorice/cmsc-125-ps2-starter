/*
 * main.c: the given program entry point. Do not edit.
 *
 * It reads the command line, loads the script, maps the arena, walks the
 * request list, prints every line of the report, and maps the result onto the
 * exit codes the manual states.
 *
 *   0    every request ran
 *   65   the command line or the script file was refused
 *   70   an ownership error, or the arena ran out
 *
 * The request loop stays here so that the four modules you write hold the
 * mechanism and the output, and nothing else.
 */
#include "ps2.h"

#include <stdlib.h>
#include <string.h>

static long dump_offsets[PS2_MAX_BLOCKS];
static long dump_sizes[PS2_MAX_BLOCKS];

static void usage(void)
{
    fprintf(stderr,
            "usage: cmsc-125-ps2 --policy first|best|next <script>\n");
}

static int set_policy(const char *word, ps2_policy *out)
{
    if (strcmp(word, "first") == 0) {
        *out = PS2_POLICY_FIRST;
    } else if (strcmp(word, "best") == 0) {
        *out = PS2_POLICY_BEST;
    } else if (strcmp(word, "next") == 0) {
        *out = PS2_POLICY_NEXT;
    } else {
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    ps2_script script;
    ps2_run run;
    ps2_diag diag;
    ps2_status status = PS2_OK;
    ps2_policy policy = PS2_POLICY_FIRST;
    const char *path = NULL;
    int policy_given = 0;
    int faulted = 0;
    size_t i;
    int arg;

    memset(&script, 0, sizeof script);
    memset(&diag, 0, sizeof diag);

    for (arg = 1; arg < argc; arg++) {
        const char *word = argv[arg];

        if (strcmp(word, "--policy") == 0) {
            if (arg + 1 >= argc) {
                fprintf(stderr, "ps2: --policy needs a value\n");
                usage();
                return PS2_EXIT_REJECTED;
            }
            if (set_policy(argv[++arg], &policy) != 0) {
                fprintf(stderr, "ps2: --policy needs first, best, or next\n");
                return PS2_EXIT_REJECTED;
            }
            policy_given = 1;
        } else if (word[0] == '-' && word[1] != '\0') {
            fprintf(stderr, "ps2: unknown option '%s'\n", word);
            usage();
            return PS2_EXIT_REJECTED;
        } else if (path == NULL) {
            path = word;
        } else {
            fprintf(stderr, "ps2: unexpected argument '%s'\n", word);
            usage();
            return PS2_EXIT_REJECTED;
        }
    }

    if (!policy_given) {
        fprintf(stderr, "ps2: --policy is required\n");
        usage();
        return PS2_EXIT_REJECTED;
    }
    if (path == NULL) {
        fprintf(stderr, "ps2: no script file\n");
        usage();
        return PS2_EXIT_REJECTED;
    }

    status = ps2_script_load(path, policy, &script, &diag);
    if (status != PS2_OK) {
        fprintf(stderr, "%s\n", diag.text);
        return PS2_EXIT_REJECTED;
    }

    memset(&run, 0, sizeof run);
    run.script = &script;

    ps2_arena_open(&script);
    if (ps2_arena_base() == NULL) {
        fprintf(stderr, "ps2: cannot map the arena\n");
        return PS2_EXIT_FAULT;
    }

    ps2_report_arena(stdout, &script);
    ps2_report_segments(stdout, &script);

    ps2_alloc_init(&run);

    for (i = 0; i < script.count; i++) {
        const ps2_request *request = &script.requests[i];
        ps2_allocation record;
        long address = 0;
        long translated = 0;
        ps2_translation result;
        size_t count;

        memset(&record, 0, sizeof record);

        switch (request->kind) {
        case PS2_REQ_ALLOC:
            status = ps2_alloc_allocate(&run, request->handle, request->size,
                                        &record, &diag);
            if (status != PS2_OK) {
                faulted = 1;
                break;
            }
            ps2_report_alloc(stdout, &record);
            break;

        case PS2_REQ_FREE:
            status = ps2_alloc_release(&run, request->handle, &record, &diag);
            if (status != PS2_OK) {
                faulted = 1;
                break;
            }
            ps2_report_release(stdout, &record);
            break;

        case PS2_REQ_FREEAT:
            status = ps2_alloc_release_at(&run, request->address, &record, &diag);
            if (status != PS2_OK) {
                faulted = 1;
                break;
            }
            ps2_report_release_at(stdout, &record);
            break;

        case PS2_REQ_TOUCH:
            status = ps2_alloc_touch(&run, request->handle, &address, &diag);
            if (status != PS2_OK) {
                faulted = 1;
                break;
            }
            ps2_report_touch(stdout, request->handle, address);
            break;

        case PS2_REQ_TRANSLATE:
            result = ps2_translate(&script, request->segment, request->offset,
                                   request->op, &translated);
            ps2_report_translate(stdout, request->segment, request->offset,
                                 request->op, translated, result);
            break;

        case PS2_REQ_DUMP:
            count = ps2_alloc_snapshot(&run, dump_offsets, dump_sizes,
                                       PS2_MAX_BLOCKS);
            ps2_report_dump(stdout, dump_offsets, dump_sizes, count);
            break;
        }

        if (faulted) {
            break;
        }
    }

    ps2_alloc_summary(&run, &run.summary);
    ps2_report_summary(stdout, &run.summary);
    ps2_report_result(stdout, faulted);
    fflush(stdout);

    if (faulted) {
        fprintf(stderr, "%s\n", diag.text);
        return PS2_EXIT_FAULT;
    }
    return PS2_EXIT_OK;
}
