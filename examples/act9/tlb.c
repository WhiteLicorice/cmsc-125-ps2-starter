/*
 * tlb.c
 *
 * A deterministic translation lookaside buffer simulator.
 *
 * The program prints one reference string's TLB trace and its summary. The
 * model is the one the activity defines:
 *
 *   - A virtual address is split into a virtual page number and an offset.
 *   - The TLB is fully associative, so an entry can hold any page.
 *   - An empty entry fills before an occupied one is evicted.
 *   - A hit refreshes the entry's recency.
 *   - A miss fills the entry that has gone unreached for the longest.
 *   - A switch between two processes empties the TLB unless the run uses
 *     address space identifiers.
 *   - A TLB lookup costs tlb_ns. A main memory access costs mem_ns.
 *
 * The reference string is built from three directives:
 *
 *   --proc NAME              set the process for the accesses that follow
 *   --walk BASE,COUNT,SIZE,PASSES
 *                            read COUNT words at BASE + i*SIZE, PASSES times
 *   --eat LIST               print the cost table for these hit rates
 *
 * Build:
 *   cc -std=c11 -Wall -Wextra -Werror tlb.c -o tlb
 *
 * Usage:
 *   ./tlb --case walk
 *
 * The six cases are walk, walk-narrow, stride, switch-flush, switch-asid,
 * and eat.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENTRIES 8
#define MAX_ACCESS  1024
#define MAX_PROCS   4
#define MAX_ARGS    80
#define MAX_SEGS    16
#define MAX_RATES   16

enum { SEG_WALK };

struct access {
    int proc;
    int vaddr;
};

struct segment {
    int kind;
    int proc;
    int base;
    int count;
    int size;
    int passes;
};

struct entry {
    int valid;
    int proc;
    int vpn;
    int stamp;
};

struct model {
    char         name[64];
    int          page;
    int          vbits;
    int          entries;
    int          asid;
    int          tlb_ns;
    int          mem_ns;
    int          nprocs;
    char         procs[MAX_PROCS][8];
    int          nsegs;
    struct segment segs[MAX_SEGS];
    int          nrates;
    double       rates[MAX_RATES];
    int          naccess;
    struct access trace[MAX_ACCESS];
};

struct sim {
    const struct model *m;
    struct entry tlb[MAX_ENTRIES];
    int current_proc;
    int clock;
    int hits;
    int misses;
    int slot;
    int last_hit;
};

static int offset_bits(const struct model *m)
{
    int bits = 0;
    int size = m->page;

    while (size > 1) {
        size >>= 1;
        bits++;
    }

    return bits;
}

static int proc_index(struct model *m, const char *name)
{
    for (int i = 0; i < m->nprocs; i++)
        if (strcmp(m->procs[i], name) == 0)
            return i;

    if (m->nprocs >= MAX_PROCS)
        return -1;

    snprintf(m->procs[m->nprocs], sizeof m->procs[0], "%s", name);

    return m->nprocs++;
}

static void flush(struct sim *s)
{
    for (int i = 0; i < s->m->entries; i++)
        s->tlb[i].valid = 0;
}

/* Find the entry that holds this page. Without identifiers, every entry
 * belongs to the current process. With identifiers, the tag must match. */
static int lookup(const struct sim *s, int proc, int vpn)
{
    for (int i = 0; i < s->m->entries; i++) {
        const struct entry *e = &s->tlb[i];

        if (e->valid && e->vpn == vpn && (!s->m->asid || e->proc == proc))
            return i;
    }

    return -1;
}

/* Return the slot a miss fills. An empty slot wins. Otherwise it is the
 * entry with the smallest stamp, which is the least recently used one. */
static int slot_for_miss(const struct sim *s)
{
    int slot = -1;
    int oldest = 0;

    for (int i = 0; i < s->m->entries; i++) {
        const struct entry *e = &s->tlb[i];

        if (!e->valid)
            return i;
        if (slot < 0 || e->stamp < oldest) {
            slot = i;
            oldest = e->stamp;
        }
    }

    return slot;
}

static void access_one(struct sim *s, int proc, int vaddr)
{
    int shift = offset_bits(s->m);
    int vpn = vaddr >> shift;
    int hit;

    if (!s->m->asid && s->current_proc >= 0 && s->current_proc != proc)
        flush(s);
    s->current_proc = proc;
    s->clock++;

    hit = lookup(s, proc, vpn);
    if (hit >= 0) {
        s->tlb[hit].stamp = s->clock;
        s->slot = hit;
        s->last_hit = 1;
        s->hits++;
        return;
    }

    s->slot = slot_for_miss(s);
    s->tlb[s->slot].valid = 1;
    s->tlb[s->slot].proc = proc;
    s->tlb[s->slot].vpn = vpn;
    s->tlb[s->slot].stamp = s->clock;
    s->last_hit = 0;
    s->misses++;
}

/* Print the entries from most recently used to least recently used. */
static void print_tlb(char *out, size_t size, const struct sim *s)
{
    int order[MAX_ENTRIES];
    int n = 0;
    size_t used = 0;

    for (int i = 0; i < s->m->entries; i++)
        if (s->tlb[i].valid)
            order[n++] = i;

    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            if (s->tlb[order[j]].stamp > s->tlb[order[i]].stamp) {
                int swap = order[i];

                order[i] = order[j];
                order[j] = swap;
            }

    used += (size_t)snprintf(out + used, size - used, "[");
    if (used >= size)
        return;

    for (int i = 0; i < s->m->entries; i++) {
        char label[12];
        int written;

        if (i < n) {
            if (s->m->asid)
                snprintf(label, sizeof label, "%s%d",
                         s->m->procs[s->tlb[order[i]].proc],
                         s->tlb[order[i]].vpn);
            else
                snprintf(label, sizeof label, "%d", s->tlb[order[i]].vpn);
        } else {
            snprintf(label, sizeof label, "--");
        }

        written = snprintf(out + used, size - used, "%s%2s",
                           i == 0 ? "" : " ", label);
        if (written < 0)
            return;
        used += (size_t)written;
        if (used >= size)
            return;
    }

    snprintf(out + used, size - used, "]");
}

static void build_trace(struct model *m)
{
    m->naccess = 0;
    for (int i = 0; i < m->nsegs; i++) {
        const struct segment *g = &m->segs[i];

        for (int pass = 0; pass < g->passes; pass++) {
            for (int k = 0; k < g->count; k++) {
                if (m->naccess >= MAX_ACCESS)
                    return;
                m->trace[m->naccess].proc = g->proc;
                m->trace[m->naccess].vaddr = g->base + k * g->size;
                m->naccess++;
            }
        }
    }
}

static void report_model(const struct model *m)
{
    int shift = offset_bits(m);

    printf("case: %s\n", m->name);
    printf("model: page %d B, vaddr %d bits (vpn %d, offset %d), "
           "tlb %d entries fully assoc LRU, asid %s, tlb %d ns, mem %d ns\n",
           m->page, m->vbits, m->vbits - shift, shift, m->entries,
           m->asid ? "on" : "off", m->tlb_ns, m->mem_ns);
}

static void report_prog(const struct model *m)
{
    printf("ref:");

    for (int i = 0; i < m->nsegs; i++) {
        const struct segment *g = &m->segs[i];

        if (g->kind == SEG_WALK)
            printf(" %s walk %d,%d,%d,%d", m->procs[g->proc], g->base,
                   g->count, g->size, g->passes);
    }

    printf("\n");
}

static void report_trace(struct model *m, struct sim *s)
{
    int shift = offset_bits(m);

    printf("\ntrace\n");
    printf("  %3s %-4s %6s %4s %4s  %-6s %4s  %s\n", "#", "proc", "vaddr",
           "vpn", "off", "result", "slot", "tlb (mru to lru)");

    for (int i = 0; i < m->naccess; i++) {
        char tlb[64];

        access_one(s, m->trace[i].proc, m->trace[i].vaddr);
        print_tlb(tlb, sizeof tlb, s);

        printf("  %3d %-4s %6d %4d %4d  %-6s %4d  %s\n", i + 1,
               m->procs[m->trace[i].proc],
               m->trace[i].vaddr,
               m->trace[i].vaddr >> shift,
               m->trace[i].vaddr & (m->page - 1),
               s->last_hit ? "hit" : "miss", s->slot, tlb);
    }
}

static void report_summary(const struct model *m, int hits, int misses)
{
    double total = (double)(hits + misses);
    double rate = total > 0 ? (double)hits / total : 0.0;
    double eat = (double)m->tlb_ns + (double)m->mem_ns
                 + (1.0 - rate) * (double)m->mem_ns;

    printf("\nsummary\n");
    printf("%-10s %8d\n", "accesses", hits + misses);
    printf("%-10s %8d\n", "hits", hits);
    printf("%-10s %8d\n", "misses", misses);
    printf("%-10s %8.4f\n", "hit rate", rate);
    printf("%-10s %8.2f ns\n", "eat", eat);
    printf("%-10s %8.2f ns\n", "no tlb", 2.0 * (double)m->mem_ns);
}

static void report_eat(const struct model *m)
{
    printf("effective access time\n");
    printf("  %8s %12s %9s %10s\n", "hit rate", "misses/1000", "eat (ns)",
           "vs no tlb");

    for (int i = 0; i < m->nrates; i++) {
        double rate = m->rates[i];
        double eat = (double)m->tlb_ns + (double)m->mem_ns
                     + (1.0 - rate) * (double)m->mem_ns;

        printf("  %8.4f %12d %9.2f %9.2fx\n", rate,
               (int)((1.0 - rate) * 1000.0 + 0.5), eat,
               2.0 * (double)m->mem_ns / eat);
    }
}

static int run_model(struct model *m)
{
    struct sim s;
    int shift = offset_bits(m);

    if (m->page < 2 || (m->page & (m->page - 1)) != 0 ||
        m->vbits < shift + 1 || m->vbits > 16 ||
        m->entries < 1 || m->entries > MAX_ENTRIES ||
        m->tlb_ns < 0 || m->mem_ns < 0)
        return -1;

    for (int i = 0; i < m->nsegs; i++) {
        const struct segment *g = &m->segs[i];
        int last = g->base + (g->count - 1) * g->size;

        if (g->count < 1 || g->size < 1 || g->passes < 1 || g->base < 0)
            return -1;
        if (last + g->size > (1 << m->vbits))
            return -1;
        if (m->naccess + g->count * g->passes > MAX_ACCESS)
            return -1;
    }

    build_trace(m);
    if (m->naccess + m->nsegs > MAX_ACCESS)
        return -1;

    memset(&s, 0, sizeof s);
    s.m = m;
    s.current_proc = -1;

    report_model(m);
    if (m->nsegs > 0)
        report_prog(m);

    if (m->naccess > 0) {
        report_trace(m, &s);
        report_summary(m, s.hits, s.misses);
    }

    if (m->nrates > 0) {
        if (m->naccess > 0)
            printf("\n");
        report_eat(m);
    }

    return 0;
}

static int parse_numbers(const char *text, int *out, int maximum)
{
    int n = 0;
    const char *p = text;

    while (*p != '\0') {
        char *end = NULL;
        long value = strtol(p, &end, 10);

        if (end == p || value < 0)
            return -1;
        if (n >= maximum)
            return -1;
        out[n++] = (int)value;
        p = end;
        if (*p == ',')
            p++;
        else if (*p != '\0')
            return -1;
    }

    return n;
}

static int parse_doubles(const char *text, double *out, int maximum)
{
    int n = 0;
    const char *p = text;

    while (*p != '\0') {
        char *end = NULL;
        double value = strtod(p, &end);

        if (end == p || value < 0.0 || value > 1.0)
            return -1;
        if (n >= maximum)
            return -1;
        out[n++] = value;
        p = end;
        if (*p == ',')
            p++;
        else if (*p != '\0')
            return -1;
    }

    return n;
}

/* The caller zeroes the model first. This function supplies the defaults and
 * reads the flags, so the two programs that build a model agree on both. */
static int parse(struct model *m, int argc, char **argv)
{
    int proc = 0;

    m->page = 16;
    m->vbits = 8;
    m->entries = 4;
    m->asid = 0;
    m->tlb_ns = 1;
    m->mem_ns = 100;
    snprintf(m->procs[0], sizeof m->procs[0], "A");
    m->nprocs = 1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--page") == 0 && i + 1 < argc) {
            m->page = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--vbits") == 0 && i + 1 < argc) {
            m->vbits = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--entries") == 0 && i + 1 < argc) {
            m->entries = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--asid") == 0 && i + 1 < argc) {
            m->asid = atoi(argv[++i]) ? 1 : 0;
        } else if (strcmp(argv[i], "--tlb") == 0 && i + 1 < argc) {
            m->tlb_ns = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--mem") == 0 && i + 1 < argc) {
            m->mem_ns = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--proc") == 0 && i + 1 < argc) {
            int index = proc_index(m, argv[++i]);

            if (index < 0)
                return -1;
            proc = index;
        } else if (strcmp(argv[i], "--walk") == 0 && i + 1 < argc) {
            int values[4];
            int n = parse_numbers(argv[++i], values, 4);

            if (n != 4 || m->nsegs >= MAX_SEGS)
                return -1;
            m->segs[m->nsegs].kind = SEG_WALK;
            m->segs[m->nsegs].proc = proc;
            m->segs[m->nsegs].base = values[0];
            m->segs[m->nsegs].count = values[1];
            m->segs[m->nsegs].size = values[2];
            m->segs[m->nsegs].passes = values[3];
            m->nsegs++;
        } else if (strcmp(argv[i], "--eat") == 0 && i + 1 < argc) {
            int n = parse_doubles(argv[++i], m->rates, MAX_RATES);

            if (n < 1)
                return -1;
            m->nrates = n;
        } else if (strcmp(argv[i], "--name") == 0 && i + 1 < argc) {
            snprintf(m->name, sizeof m->name, "%s", argv[++i]);
        } else {
            return -1;
        }
    }

    if (m->nsegs == 0 && m->nrates == 0)
        return -1;

    return 0;
}

static const char *case_body(const char *name)
{
    static const struct {
        const char *name;
        const char *body;
    } cases[] = {
        { "walk",
          "--page 16 --vbits 8 --entries 4 --walk 0,16,4,2" },
        { "walk-narrow",
          "--page 16 --vbits 8 --entries 2 --walk 0,16,4,2" },
        { "stride",
          "--page 16 --vbits 8 --entries 4 --walk 0,16,16,1" },
        { "switch-flush",
          "--page 16 --vbits 8 --entries 4 --proc A --walk 0,4,4,1"
          " --proc B --walk 0,4,4,1 --proc A --walk 0,4,4,1" },
        { "switch-asid",
          "--page 16 --vbits 8 --entries 4 --asid 1 --proc A --walk 0,4,4,1"
          " --proc B --walk 0,4,4,1 --proc A --walk 0,4,4,1" },
        { "eat",
          "--page 16 --vbits 8 --entries 4"
          " --eat 1.0,0.99,0.98,0.95,0.9,0.8,0.75,0.0" },
    };

    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++)
        if (strcmp(cases[i].name, name) == 0)
            return cases[i].body;

    return NULL;
}

static int split_words(char *line, char **out, int maximum)
{
    int n = 0;
    char *p = line;

    while (*p != '\0') {
        while (*p == ' ')
            p++;
        if (*p == '\0')
            break;
        if (n >= maximum)
            return -1;
        out[n++] = p;
        while (*p != '\0' && *p != ' ')
            p++;
        if (*p != '\0')
            *p++ = '\0';
    }

    return n;
}

static void usage(void)
{
    fprintf(stderr,
            "usage: tlb --case NAME\n"
            "\n"
            "cases: walk walk-narrow stride switch-flush switch-asid eat\n");
}

int main(int argc, char **argv)
{
    struct model model;
    char body[1024];
    char *expanded[MAX_ARGS];
    int expanded_count = 0;
    int added;
    const char *source;

    if (argc != 3 || strcmp(argv[1], "--case") != 0) {
        usage();
        return 1;
    }

    source = case_body(argv[2]);
    if (source == NULL) {
        fprintf(stderr, "tlb: no case named %s\n", argv[2]);
        return 1;
    }

    memset(&model, 0, sizeof model);
    snprintf(body, sizeof body, "%s", source);
    snprintf(model.name, sizeof model.name, "%s", argv[2]);

    expanded[expanded_count++] = argv[0];
    added = split_words(body, expanded + expanded_count,
                        MAX_ARGS - expanded_count);
    if (added < 0) {
        fprintf(stderr, "tlb: the case has too many arguments\n");
        return 1;
    }
    expanded_count += added;

    if (parse(&model, expanded_count, expanded) != 0) {
        fprintf(stderr, "tlb: the case is not valid\n");
        return 1;
    }

    return run_model(&model) == 0 ? 0 : 1;
}
