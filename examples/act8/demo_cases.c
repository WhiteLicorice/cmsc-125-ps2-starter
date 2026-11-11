/*
 * demo_cases.c
 *
 * The student-facing case list for the CMSC 125 activity 8 paging translator.
 * Every case here appears in the activity, its worked examples, or its size
 * table.
 *
 * See the README beside this file for the model and the case table.
 */

#include "pagesim.h"

/*
 * Table A is the page table the worked examples use. Five pages are mapped.
 * The other eleven VPNs are not valid.
 */
#define TABLE_A                                                          \
    {                                                                    \
        [0] = { 1, 3, PROT_R | PROT_W | PROT_X, 1, 1, 1 },               \
        [1] = { 1, 0, PROT_R, 1, 0, 1 },                                 \
        [2] = { 1, 7, PROT_R | PROT_W, 1, 1, 1 },                        \
        [3] = { 1, 1, PROT_R | PROT_X, 1, 0, 1 },                        \
        [4] = { 1, 5, PROT_R | PROT_W, 1, 0, 1 },                        \
    }

const struct case_def sim_cases[] = {
    { "basic", KIND_TRANSLATE, TABLE_A, 4,
      { { 0x0C5, 'R' }, { 0x2A0, 'R' }, { 0x4FF, 'W' }, { 0x3B4, 'X' } },
      0, { { 0, 0 } } },
    { "offset", KIND_TRANSLATE, TABLE_A, 2,
      { { 0x100, 'R' }, { 0x1FF, 'R' } }, 0, { { 0, 0 } } },
    { "faults", KIND_TRANSLATE, TABLE_A, 4,
      { { 0x1A0, 'W' }, { 0x2A0, 'X' }, { 0x9FF, 'R' }, { 0x1A0, 'R' } },
      0, { { 0, 0 } } },
    { "sizes", KIND_SIZE, { { 0 } }, 0, { { 0, 0 } }, 3,
      { { 32, 12 }, { 32, 14 }, { 20, 12 } } },
};

const int sim_case_count =
    (int)(sizeof(sim_cases) / sizeof(sim_cases[0]));
