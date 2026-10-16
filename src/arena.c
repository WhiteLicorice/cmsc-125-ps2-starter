/*
 * arena.c defines the arena and its block header accessors. Do not edit it.
 *
 * The program asks the operating system for one region, once, and never asks
 * again. The region is private and anonymous, so no host address reaches the
 * output. Every address this program prints is an offset into the arena.
 *
 * A block lives at some offset in the arena. Its first 16 bytes are its
 * header: an 8-byte size, which counts the header, and an 8-byte link to the
 * next free block. A link of -1 ends the free list. The payload starts 16
 * bytes past the block. That sum is the address the allocator hands out.
 *
 *   offset 0                block size
 *   |<---------------------- size ---------------------->|
 *   +----------------+----------------+------------------+
 *   | size (8 bytes) | next (8 bytes) | payload          |
 *   +----------------+----------------+------------------+
 *   ^                                 ^
 *   the block starts here             the address handed out
 */
/*
 * macOS hides the anonymous-mapping flag from a program that asks for a strict
 * POSIX feature set. This build asks for one. Ask Darwin for its own set
 * first. A feature-test macro has to arrive before any header does, so this
 * block sits above the include of ps2.h.
 */
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE 1
#endif

#include "ps2.h"

#include <string.h>
#include <sys/mman.h>

static unsigned char *arena_base;
static long arena_bytes;

void ps2_arena_open(const ps2_script *script)
{
    void *region;

    arena_bytes = script->arena;
    region = mmap(NULL, (size_t)arena_bytes, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (region == MAP_FAILED) {
        arena_base = NULL;
        arena_bytes = 0;
        return;
    }
    arena_base = region;
}

unsigned char *ps2_arena_base(void)
{
    return arena_base;
}

long ps2_arena_size(void)
{
    return arena_bytes;
}

long ps2_round_up(long value, long unit)
{
    long remainder = value % unit;

    if (remainder == 0) {
        return value;
    }
    return value + (unit - remainder);
}

long ps2_block_size(long offset)
{
    int64_t size = 0;

    memcpy(&size, arena_base + offset, sizeof size);
    return (long)size;
}

void ps2_block_set_size(long offset, long size)
{
    int64_t value = (int64_t)size;

    memcpy(arena_base + offset, &value, sizeof value);
}

long ps2_block_next(long offset)
{
    int64_t next = 0;

    memcpy(&next, arena_base + offset + 8, sizeof next);
    return (long)next;
}

void ps2_block_set_next(long offset, long next)
{
    int64_t value = (int64_t)next;

    memcpy(arena_base + offset + 8, &value, sizeof value);
}
