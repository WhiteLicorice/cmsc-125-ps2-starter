/*
 * pagesim.h
 *
 * Shared declarations for the paging translator.
 *
 * The program that links pagesim.c supplies the case list. The student-facing
 * build supplies the demo and worked-example cases in demo_cases.c. Another
 * build can supply a different list.
 *
 * The model is fixed. The README beside this file states it.
 */

#ifndef PAGESIM_H
#define PAGESIM_H

/* The fixed model uses 256-byte pages and 256-byte frames. */
#define PAGE_BITS 8
#define PAGE_BYTES (1 << PAGE_BITS)      /* 256 */
#define VA_BITS 12
#define VPN_BITS (VA_BITS - PAGE_BITS)   /* 4 */
#define NUM_VPAGES (1 << VPN_BITS)       /* 16 */
#define PA_BITS 11
#define PFN_BITS (PA_BITS - PAGE_BITS)   /* 3 */
#define NUM_FRAMES (1 << PFN_BITS)       /* 8 */

/* Protection bits. The order matches the printed string, R then W then X. */
#define PROT_R 1
#define PROT_W 2
#define PROT_X 4

#define MAX_ACCESS 8
#define MAX_SIZE_ROWS 4

enum case_kind { KIND_TRANSLATE, KIND_SIZE };

struct pte {
    int valid;
    int pfn;
    int prot;
    int present;
    int dirty;
    int accessed;
};

struct access {
    unsigned int vaddr;
    char op;
};

struct size_row {
    int addr_bits;
    int page_bits;
};

struct case_def {
    const char *id;
    enum case_kind kind;
    struct pte table[NUM_VPAGES];
    int naccess;
    struct access access[MAX_ACCESS];
    int nrows;
    struct size_row rows[MAX_SIZE_ROWS];
};

extern const struct case_def sim_cases[];
extern const int sim_case_count;

#endif
