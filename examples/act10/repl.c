/*
 * repl.c
 *
 * A deterministic page-replacement and page-fault simulator.
 *
 * The program prints the replacement traces for one workload. It prints one
 * table for each policy and each frame count that the case lists. The model is
 * the one the activity defines:
 *
 *   - The page size is 4 KiB and every address is 32 bits.
 *   - A reference string is a list of virtual page numbers, one access per
 *     step. Every access is a load.
 *   - The frames start empty, so the first access to a page is a miss.
 *   - A miss evicts one resident page under the policy in force, unless a
 *     frame is still empty.
 *   - A victim tie breaks to the lowest page number.
 *   - The random policy draws every victim from one fixed seed, so its trace
 *     repeats on every machine.
 *   - The clock hand starts at slot 0 and moves forward one slot at a time.
 *
 * The fault-path model follows one load from its virtual address through the
 * TLB, the page table, a page fault, and one replacement. The scenario lists
 * the frames and their resident pages in load order, the TLB entries in
 * insertion order, the dirty pages, and the virtual address. The first frame
 * is the FIFO victim. The first TLB entry is the entry that leaves.
 *
 * Build:
 *   cc -std=c11 -Wall -Wextra -Werror repl.c -o repl
 *
 * Usage:
 *   ./repl --case policies
 *
 * The three cases are policies, belady, and faultpath. Each one prints the
 * tables for its workload.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_REFS   64
#define MAX_FRAMES 8
#define MAX_ARGS   128

enum policy { OPT, FIFO, RAND, LRU, CLOCK };

static const char *policy_name(enum policy pol)
{
    switch (pol) {
    case OPT:   return "optimal";
    case FIFO:  return "fifo";
    case RAND:  return "random";
    case LRU:   return "lru";
    case CLOCK: return "clock";
    }
    return "?";
}

struct model {
    char name[64];
    int  ref[MAX_REFS];
    int  nrefs;
    int  frame_count[MAX_FRAMES];
    int  nframe_counts;
    enum policy pol[MAX_FRAMES];
    int  npols;
    int  faultpath;

    int  fp_frames;
    int  fp_resident[MAX_FRAMES];
    int  fp_nresident;
    int  fp_tlb_vpn[MAX_FRAMES];
    int  fp_tlb_frame[MAX_FRAMES];
    int  fp_ntlb;
    int  fp_tlb_size;
    int  fp_dirty[MAX_FRAMES];
    int  fp_ndirty;
    unsigned int fp_va;
};

struct result {
    enum policy pol;
    int  frames;
    int  hits;
    int  misses;
};

/* Find the slot that holds the page, or -1. */
static int find_slot(const int *frames, int nframes, int page)
{
    for (int i = 0; i < nframes; i++)
        if (frames[i] == page)
            return i;

    return -1;
}

/* The first empty slot, or -1. */
static int first_empty(const int *frames, int nframes)
{
    for (int i = 0; i < nframes; i++)
        if (frames[i] < 0)
            return i;

    return -1;
}

/* The next reference to the page at or after "from", or a large sentinel. */
static int next_use(const struct model *m, int from, int page)
{
    for (int i = from; i < m->nrefs; i++)
        if (m->ref[i] == page)
            return i;

    return m->nrefs + 1;
}

/* Pick the slot to fill on a miss. */
static int pick_victim(const struct model *m, enum policy pol, int nframes,
                       const int *frames, const long *loaded, const long *last,
                       int use[], int t, int *hand, unsigned int *rng)
{
    int empty = first_empty(frames, nframes);

    if (empty >= 0)
        return empty;

    if (pol == OPT) {
        int best = 0;
        int best_next = next_use(m, t + 1, frames[0]);

        for (int i = 1; i < nframes; i++) {
            int next = next_use(m, t + 1, frames[i]);

            if (next > best_next || (next == best_next && frames[i] < frames[best])) {
                best = i;
                best_next = next;
            }
        }
        return best;
    }

    if (pol == FIFO) {
        int best = 0;

        for (int i = 1; i < nframes; i++)
            if (loaded[i] < loaded[best])
                best = i;
        return best;
    }

    if (pol == RAND) {
        *rng = *rng * 1103515245u + 12345u;
        return (int)((*rng >> 16) % (unsigned)nframes);
    }

    if (pol == LRU) {
        int best = 0;

        for (int i = 1; i < nframes; i++) {
            if (last[i] < last[best] || (last[i] == last[best] && frames[i] < frames[best]))
                best = i;
        }
        return best;
    }

    /* Clock: advance the hand past every frame whose use bit is set. */
    for (int guard = 0; guard < nframes * 2; guard++) {
        if (use[*hand] == 0)
            break;
        use[*hand] = 0;
        *hand = (*hand + 1) % nframes;
    }
    return *hand;
}

/* Run one policy at one frame count, and print its trace. */
static struct result run_policy(const struct model *m, enum policy pol, int nframes,
                                int verbose)
{
    int  frames[MAX_FRAMES];
    int  use[MAX_FRAMES];
    long loaded[MAX_FRAMES];
    long last[MAX_FRAMES];
    int  hand = 0;
    long seq = 0;
    unsigned int rng = 1u;
    int hits = 0, misses = 0;
    int width = nframes * 2;

    for (int i = 0; i < nframes; i++) {
        frames[i] = -1;
        use[i] = 0;
        loaded[i] = 0;
        last[i] = -1;
    }

    if (verbose) {
        printf("policy %s, frames %d\n", policy_name(pol), nframes);
        printf("  %3s  %4s  %-*s  %-6s  %s\n", "ref", "page", width, "frames",
               "result", "evicted");
    }

    for (int t = 0; t < m->nrefs; t++) {
        int page = m->ref[t];
        int slot = find_slot(frames, nframes, page);
        const char *result;
        int evicted = -1;

        if (slot >= 0) {
            hits++;
            result = "hit";
            use[slot] = 1;
            last[slot] = t;
        } else {
            misses++;
            result = "miss";
            slot = pick_victim(m, pol, nframes, frames, loaded, last, use, t,
                               &hand, &rng);
            evicted = frames[slot];
            frames[slot] = page;
            use[slot] = 1;
            loaded[slot] = ++seq;
            last[slot] = t;
            if (pol == CLOCK)
                hand = (slot + 1) % nframes;
        }

        if (verbose) {
            char fb[64];
            char eb[16];
            size_t u = 0;

            for (int i = 0; i < nframes; i++) {
                if (frames[i] < 0)
                    u += (size_t)snprintf(fb + u, sizeof fb - u, "%s.", i ? " " : "");
                else
                    u += (size_t)snprintf(fb + u, sizeof fb - u, "%s%d", i ? " " : "", frames[i]);
            }
            if (evicted < 0)
                snprintf(eb, sizeof eb, "-");
            else
                snprintf(eb, sizeof eb, "%d", evicted);
            printf("  %3d  %4d  %-*s  %-6s  %s\n", t, page, width, fb, result, eb);
        }
    }

    struct result r;
    r.pol = pol;
    r.frames = nframes;
    r.hits = hits;
    r.misses = misses;

    if (verbose)
        printf("  hits %d, misses %d, hit rate %.2f%%\n\n", hits, misses,
               100.0 * hits / m->nrefs);

    return r;
}

static int page_in_list(const int *list, int n, int page)
{
    for (int i = 0; i < n; i++)
        if (list[i] == page)
            return 1;

    return 0;
}

/* Follow one load from its virtual address through the whole path. */
static void run_faultpath(const struct model *m)
{
    const int page_size = 4096;
    int offset = (int)(m->fp_va % page_size);
    int vpn = (int)(m->fp_va / page_size);
    int victim_frame = 0;
    int evicted = m->fp_resident[0];
    int dirty = page_in_list(m->fp_dirty, m->fp_ndirty, evicted);
    int frames_after[MAX_FRAMES];
    int tlb_after_vpn[MAX_FRAMES];
    int tlb_after_frame[MAX_FRAMES];
    int ntlb_after = 0;
    unsigned int pa = (unsigned int)(victim_frame * page_size + offset);

    for (int i = 0; i < m->fp_frames; i++)
        frames_after[i] = m->fp_resident[i];
    frames_after[victim_frame] = vpn;

    /* The new entry leaves room for itself, so the oldest entries leave first. */
    int keep = m->fp_ntlb - (m->fp_tlb_size - 1);

    if (keep < 0)
        keep = 0;
    for (int i = keep; i < m->fp_ntlb; i++) {
        tlb_after_vpn[ntlb_after] = m->fp_tlb_vpn[i];
        tlb_after_frame[ntlb_after] = m->fp_tlb_frame[i];
        ntlb_after++;
    }
    tlb_after_vpn[ntlb_after] = vpn;
    tlb_after_frame[ntlb_after] = victim_frame;
    ntlb_after++;

    printf("model: page 4 KiB, frames %d, tlb %d entries fifo, replacement fifo\n",
           m->fp_frames, m->fp_tlb_size);
    printf("       the frames hold pages");
    for (int i = 0; i < m->fp_frames; i++)
        printf("%s%d", i ? ", " : " ", m->fp_resident[i]);
    printf(" in load order\n");
    printf("       the tlb holds, oldest first,");
    for (int i = 0; i < m->fp_ntlb; i++)
        printf("%s vpn %d -> frame %d", i ? " and" : "", m->fp_tlb_vpn[i],
               m->fp_tlb_frame[i]);
    printf("\n");
    printf("       the dirty pages are");
    if (m->fp_ndirty == 0)
        printf(" none");
    for (int i = 0; i < m->fp_ndirty; i++)
        printf("%s%d", i ? ", " : " ", m->fp_dirty[i]);
    printf("; every other page is clean\n\n");

    printf("load  0x%08x (vpn %d, offset %d)\n\n", m->fp_va, vpn, offset);

    printf("  %3s  %-30s  %s\n", "step", "action", "detail");
    printf("  %3d  %-30s  vpn %d, offset %d\n", 1, "split the virtual address",
           vpn, offset);
    printf("  %3d  %-30s  the tlb holds vpn", 2, "look up the vpn in the tlb");
    printf(" %d", m->fp_tlb_vpn[0]);
    for (int i = 1; i < m->fp_ntlb; i++)
        printf(" and vpn %d", m->fp_tlb_vpn[i]);
    printf(", so vpn %d misses\n", vpn);
    printf("  %3d  %-30s  the pte for vpn %d has present bit 0\n", 3,
           "look up the page table", vpn);
    printf("  %3d  %-30s  %s\n", 4, "raise a page fault", "trap to the kernel");
    printf("  %3d  %-30s  vpn %d sits in swap\n", 5, "locate the page", vpn);
    printf("  %3d  %-30s  frame %d holds vpn %d, loaded first\n", 6,
           "pick a victim (fifo)", victim_frame, evicted);
    if (dirty)
        printf("  %3d  %-30s  vpn %d is dirty, write it back to swap\n", 7,
               "check the victim dirty bit", evicted);
    else
        printf("  %3d  %-30s  vpn %d is clean, no write-back\n", 7,
               "check the victim dirty bit", evicted);
    printf("  %3d  %-30s  vpn %d: present 0, copy in swap\n", 8,
           "clear the victim's present bit", evicted);
    printf("  %3d  %-30s  vpn %d into frame %d\n", 9, "read the page from swap",
           vpn, victim_frame);
    printf("  %3d  %-30s  vpn %d -> frame %d, present 1\n", 10,
           "update the page table", vpn, victim_frame);
    if (keep > 0)
        printf("  %3d  %-30s  insert vpn %d -> frame %d, evict vpn %d\n", 11,
               "update the tlb (fifo)", vpn, victim_frame, m->fp_tlb_vpn[0]);
    else
        printf("  %3d  %-30s  insert vpn %d -> frame %d, no eviction\n", 11,
               "update the tlb (fifo)", vpn, victim_frame);
    printf("  %3d  %-30s  tlb hit, frame %d\n", 12, "restart the load",
           victim_frame);
    printf("  %3d  %-30s  frame %d * 4 KiB + %d\n", 13,
           "form the physical address", victim_frame, offset);
    printf("\n");
    printf("note: vpn %d holds no tlb entry, so no victim invalidation runs\n",
           evicted);

    printf("frames after the load:");
    for (int i = 0; i < m->fp_frames; i++)
        printf("%s page %d in frame %d", i ? "," : "", frames_after[i], i);
    printf("\n");
    printf("tlb after the load:");
    for (int i = 0; i < ntlb_after; i++)
        printf("%s vpn %d -> frame %d", i ? "," : "", tlb_after_vpn[i],
               tlb_after_frame[i]);
    printf("\n");
    printf("physical address 0x%08x\n", pa);
    printf("counts: tlb misses 1, page-table lookups 1, page faults 1, "
           "evictions 1, victim ptes cleared 1, write-backs %d\n", dirty ? 1 : 0);
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

/* Parse a list of vpn:frame pairs, separated by commas. */
static int parse_pairs(const char *text, int *vpn, int *frame, int maximum)
{
    int n = 0;
    const char *p = text;

    while (*p != '\0') {
        char *end = NULL;
        long a = strtol(p, &end, 10);

        if (end == p || *end != ':')
            return -1;
        p = end + 1;
        long b = strtol(p, &end, 10);
        if (end == p)
            return -1;
        if (n >= maximum)
            return -1;
        vpn[n] = (int)a;
        frame[n] = (int)b;
        n++;
        p = end;
        if (*p == ',')
            p++;
        else if (*p != '\0')
            return -1;
    }

    return n;
}

static int parse_policy(const char *text, enum policy *out)
{
    if (strcmp(text, "optimal") == 0)  { *out = OPT;   return 0; }
    if (strcmp(text, "fifo") == 0)     { *out = FIFO;  return 0; }
    if (strcmp(text, "random") == 0)   { *out = RAND;  return 0; }
    if (strcmp(text, "lru") == 0)      { *out = LRU;   return 0; }
    if (strcmp(text, "clock") == 0)    { *out = CLOCK; return 0; }

    return -1;
}

static int parse_policies(const char *text, struct model *m)
{
    char buf[128];
    char *tok;

    snprintf(buf, sizeof buf, "%s", text);
    for (tok = strtok(buf, ","); tok != NULL; tok = strtok(NULL, ",")) {
        if (m->npols >= MAX_FRAMES)
            return -1;
        if (parse_policy(tok, &m->pol[m->npols]) != 0)
            return -1;
        m->npols++;
    }

    return m->npols > 0 ? 0 : -1;
}

static int parse(struct model *m, int argc, char **argv)
{
    m->nrefs = 0;
    m->nframe_counts = 0;
    m->npols = 0;
    m->faultpath = 0;
    m->fp_tlb_size = 0;
    m->fp_ndirty = 0;
    m->fp_nresident = 0;
    m->fp_ntlb = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--ref") == 0 && i + 1 < argc) {
            m->nrefs = parse_numbers(argv[++i], m->ref, MAX_REFS);
            if (m->nrefs < 1)
                return -1;
        } else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            m->nframe_counts = parse_numbers(argv[++i], m->frame_count, MAX_FRAMES);
            if (m->nframe_counts < 1)
                return -1;
        } else if (strcmp(argv[i], "--policy") == 0 && i + 1 < argc) {
            if (parse_policies(argv[++i], m) != 0)
                return -1;
        } else if (strcmp(argv[i], "--name") == 0 && i + 1 < argc) {
            snprintf(m->name, sizeof m->name, "%s", argv[++i]);
        } else if (strcmp(argv[i], "--faultpath") == 0) {
            m->faultpath = 1;
        } else if (strcmp(argv[i], "--fp-frames") == 0 && i + 1 < argc) {
            m->fp_frames = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--fp-resident") == 0 && i + 1 < argc) {
            m->fp_nresident = parse_numbers(argv[++i], m->fp_resident, MAX_FRAMES);
            if (m->fp_nresident < 1)
                return -1;
        } else if (strcmp(argv[i], "--fp-tlb") == 0 && i + 1 < argc) {
            m->fp_ntlb = parse_pairs(argv[++i], m->fp_tlb_vpn, m->fp_tlb_frame,
                                     MAX_FRAMES);
            if (m->fp_ntlb < 0)
                return -1;
        } else if (strcmp(argv[i], "--fp-tlb-size") == 0 && i + 1 < argc) {
            m->fp_tlb_size = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--fp-dirty") == 0 && i + 1 < argc) {
            m->fp_ndirty = parse_numbers(argv[++i], m->fp_dirty, MAX_FRAMES);
            if (m->fp_ndirty < 0)
                return -1;
        } else if (strcmp(argv[i], "--fp-va") == 0 && i + 1 < argc) {
            m->fp_va = (unsigned int)strtoul(argv[++i], NULL, 0);
        } else {
            return -1;
        }
    }

    if (m->faultpath) {
        if (m->fp_frames < 1 || m->fp_nresident != m->fp_frames)
            return -1;
        if (m->fp_tlb_size < 1)
            return -1;
        return 0;
    }
    if (m->nrefs < 1 || m->nframe_counts < 1 || m->npols < 1)
        return -1;

    for (int i = 0; i < m->nframe_counts; i++) {
        if (m->frame_count[i] < 1 || m->frame_count[i] > MAX_FRAMES)
            return -1;
    }

    return 0;
}

static const char *case_body(const char *name)
{
    static const struct {
        const char *name;
        const char *body;
    } cases[] = {
        { "policies",
          "--ref 0,1,2,0,1,3,0,1,2,4,0,1 --frames 3"
          " --policy optimal,fifo,random,lru,clock" },
        { "belady",
          "--ref 1,2,3,4,1,2,5,1,2,3,4,5 --frames 3,4"
          " --policy fifo,optimal" },
        { "faultpath",
          "--faultpath --fp-frames 3 --fp-resident 2,5,7 --fp-tlb-size 2"
          " --fp-tlb 5:1,7:2 --fp-dirty 2 --fp-va 0x00003044" },
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
            "usage: repl --case NAME\n"
            "\n"
            "cases: policies belady faultpath\n");
}

/* Run one model: every policy at every frame count, then the summary. */
static int run_model(struct model *m)
{
    struct result results[MAX_FRAMES * MAX_FRAMES];
    int n = 0;

    printf("case: %s\n", m->name);

    if (m->faultpath) {
        run_faultpath(m);
        return 0;
    }

    printf("model: page 4 KiB, reference");
    for (int i = 0; i < m->nrefs; i++)
        printf(" %d", m->ref[i]);
    printf("\n");
    printf("       frames start empty, first access to a page misses, "
           "ties break to the lowest page number\n\n");

    for (int p = 0; p < m->npols; p++)
        for (int f = 0; f < m->nframe_counts; f++)
            results[n++] = run_policy(m, m->pol[p], m->frame_count[f], 1);

    printf("summary\n");
    printf("  %-8s %6s  %5s  %6s  %8s\n", "policy", "frames", "hits",
           "misses", "hit rate");
    for (int i = 0; i < n; i++)
        printf("  %-8s %6d  %5d  %6d  %7.2f%%\n", policy_name(results[i].pol),
               results[i].frames, results[i].hits, results[i].misses,
               100.0 * results[i].hits / m->nrefs);

    return 0;
}

int main(int argc, char **argv)
{
    struct model model;
    char body[512];
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
        fprintf(stderr, "repl: no case named %s\n", argv[2]);
        return 1;
    }

    memset(&model, 0, sizeof model);
    snprintf(model.name, sizeof model.name, "%s", argv[2]);
    snprintf(body, sizeof body, "%s", source);

    expanded[expanded_count++] = argv[0];
    added = split_words(body, expanded + expanded_count,
                        MAX_ARGS - expanded_count);
    if (added < 0) {
        fprintf(stderr, "repl: the case has too many arguments\n");
        return 1;
    }
    expanded_count += added;

    if (parse(&model, expanded_count, expanded) != 0) {
        fprintf(stderr, "repl: the case is not valid\n");
        return 1;
    }

    return run_model(&model) == 0 ? 0 : 1;
}
