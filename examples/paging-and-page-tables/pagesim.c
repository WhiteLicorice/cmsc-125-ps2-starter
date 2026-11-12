/*
 * pagesim.c
 *
 * A deterministic paging translator.
 *
 * This file holds the model and the printer. The case list is linked in from
 * the build. A case is either a translation case, which prints a page table
 * and a table of translated addresses, or a size case, which prints the size
 * of a linear page table for several address widths.
 *
 * The student-facing build links demo_cases.c:
 *   cc -std=c11 -Wall -Wextra -Werror -o pagesim pagesim.c demo_cases.c
 *
 * Run one case:
 *   ./pagesim basic
 *
 * Run every case:
 *   ./pagesim
 */

#include <stdio.h>
#include <string.h>

#include "pagesim.h"

static const char *prot_str(int prot)
{
    static char buf[4];

    buf[0] = (prot & PROT_R) ? 'R' : '-';
    buf[1] = (prot & PROT_W) ? 'W' : '-';
    buf[2] = (prot & PROT_X) ? 'X' : '-';
    buf[3] = '\0';

    return buf;
}

static void to_bits(unsigned int value, int bits, char *out)
{
    int i;

    for (i = 0; i < bits; i++)
        out[i] = ((value >> (bits - 1 - i)) & 1u) ? '1' : '0';
    out[bits] = '\0';
}

static void human_bytes(long bytes, char *out, size_t out_size)
{
    if (bytes >= (1L << 20) && bytes % (1L << 20) == 0)
        snprintf(out, out_size, "%ld MiB", bytes >> 20);
    else if (bytes >= (1L << 10) && bytes % (1L << 10) == 0)
        snprintf(out, out_size, "%ld KiB", bytes >> 10);
    else
        snprintf(out, out_size, "%ld B", bytes);
}

static void print_translate(const struct case_def *c)
{
    int i;

    printf("case %s\n", c->id);
    printf("page size %d bytes, virtual address %d bits, physical address %d bits\n",
           PAGE_BYTES, VA_BITS, PA_BITS);
    printf("virtual pages %d, frames %d, vpn bits %d, offset bits %d\n\n",
           NUM_VPAGES, NUM_FRAMES, VPN_BITS, PAGE_BITS);

    printf("page table\n");
    printf("  %-4s %-6s %-5s %-6s %-8s %-6s %s\n", "vpn", "valid", "pfn",
           "prot", "present", "dirty", "accessed");
    for (i = 0; i < NUM_VPAGES; i++) {
        const struct pte *p = &c->table[i];

        if (p->valid)
            printf("  %-4d %-6d %-5d %-6s %-8d %-6d %d\n", i, p->valid,
                   p->pfn, prot_str(p->prot), p->present, p->dirty,
                   p->accessed);
        else
            printf("  %-4d %-6d %-5s %-6s %-8s %-6s %s\n", i, 0, "-", "-", "-",
                   "-", "-");
    }
    printf("\n");

    printf("translation\n");
    printf("  %-7s %-14s %-4s %-7s %-3s %s\n", "vaddr", "bits", "vpn",
           "offset", "op", "result");
    for (i = 0; i < c->naccess; i++) {
        unsigned int va = c->access[i].vaddr;
        unsigned int vpn = (va >> PAGE_BITS) & (unsigned int)(NUM_VPAGES - 1);
        unsigned int offset = va & (unsigned int)(PAGE_BYTES - 1);
        const struct pte *p = &c->table[vpn];
        char op = c->access[i].op;
        int need = (op == 'R') ? PROT_R : (op == 'W') ? PROT_W : PROT_X;
        char vaddr_str[8];
        char offset_str[8];
        char vpn_bits[8];
        char off_bits[9];
        char bits_str[24];
        char result[64];

        snprintf(vaddr_str, sizeof vaddr_str, "0x%03X", va);
        snprintf(offset_str, sizeof offset_str, "0x%02X", offset);
        to_bits(vpn, VPN_BITS, vpn_bits);
        to_bits(offset, PAGE_BITS, off_bits);
        snprintf(bits_str, sizeof bits_str, "%s %s", vpn_bits, off_bits);

        if (!p->valid)
            snprintf(result, sizeof result, "unmapped: page %u is not valid",
                     vpn);
        else if ((p->prot & need) == 0)
            snprintf(result, sizeof result,
                     "protection fault: page %u does not allow %c", vpn, op);
        else
            snprintf(result, sizeof result, "frame %d, physical 0x%03X",
                     p->pfn, (p->pfn << PAGE_BITS) | offset);

        printf("  %-7s %-14s %-4u %-7s %-3c %s\n", vaddr_str, bits_str, vpn,
               offset_str, op, result);
    }
    printf("\n");
}

static void print_size(const struct case_def *c)
{
    char human[16];
    int i;

    printf("case %s\n", c->id);
    printf("linear page table size\n");
    printf("  %-9s %-11s %-8s %-9s %-9s %-11s %s\n", "addr bits",
           "page bytes", "vpn bits", "entries", "pte bytes", "table bytes",
           "table size");
    for (i = 0; i < c->nrows; i++) {
        int addr_bits = c->rows[i].addr_bits;
        int page_bits = c->rows[i].page_bits;
        int page_bytes = 1 << page_bits;
        int vpn_bits = addr_bits - page_bits;
        long entries = 1L << vpn_bits;
        long table_bytes = entries * 4L;

        human_bytes(table_bytes, human, sizeof human);
        printf("  %-9d %-11d %-8d %-9ld %-9d %-11ld %s\n", addr_bits,
               page_bytes, vpn_bits, entries, 4, table_bytes, human);
    }
    printf("\n");

    printf("sv32 two level\n");
    printf("  virtual address 32 bits, page size 4096 bytes, offset bits 12\n");
    printf("  %-8s %-9s %-8s %-10s %-12s %s\n", "level", "vpn bits",
           "entries", "pte bytes", "table bytes", "table size");
    human_bytes(4096L, human, sizeof human);
    printf("  %-8s %-9d %-8d %-10d %-12d %s\n", "level-1", 10, 1024, 4, 4096,
           human);
    printf("  %-8s %-9d %-8d %-10d %-12d %s\n", "level-0", 10, 1024, 4, 4096,
           human);
    printf("\n");
}

static void print_case(const struct case_def *c)
{
    if (c->kind == KIND_SIZE)
        print_size(c);
    else
        print_translate(c);
}

int main(int argc, char **argv)
{
    size_t ncases = (size_t)sim_case_count;
    size_t i;

    if (argc == 1) {
        for (i = 0; i < ncases; i++)
            print_case(&sim_cases[i]);
        return 0;
    }
    if (argc != 2) {
        fprintf(stderr, "usage: %s [case]\n", argv[0]);
        return 2;
    }
    for (i = 0; i < ncases; i++) {
        if (strcmp(argv[1], sim_cases[i].id) == 0) {
            print_case(&sim_cases[i]);
            return 0;
        }
    }
    fprintf(stderr, "unknown case: %s\n", argv[1]);
    return 2;
}
