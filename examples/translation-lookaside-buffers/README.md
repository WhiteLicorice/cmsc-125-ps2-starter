<!--no-pdf-->
# Translation Lookaside Buffers

This folder holds one program. `tlb.c` is a translation lookaside buffer
simulator. It implements the model below. It prints the TLB
trace and the summary for one reference string. The program is deterministic.
The same command prints the same output on every machine.

The program runs in Linux. On Windows, run it inside Ubuntu under Windows
Subsystem for Linux (WSL2). On macOS, run it inside an Ubuntu 24.04 Server
virtual machine under UTM. The bootcamp manual "Getting Your Machine Ready" has the setup for that
virtual machine. Run every command inside that Ubuntu guest.

## Requirements

- A C compiler. The outputs below come from `cc` in GCC 13.3.0.
- `make`.

## Build and Run

Build the simulator.

```bash
make
```

Build the simulator, run every case, and compare the output.

```bash
make check
```

Delete the simulator.

```bash
make clean
```

Build the simulator with the compiler directly.

```bash
cc -std=c11 -Wall -Wextra -Werror -o tlb tlb.c
```

The compile must stay clean. `-Werror` turns every warning into an error.

## How the Check Decides

`make check` runs each case in turn. It compares the output with
`expected/<case>.txt`. The simulator is deterministic, so every comparison is
exact. It prints one line for each case, then a count line. It exits 0 when
every case passes. It exits nonzero when a case fails.

## The Model

The simulator implements the model below.

- The page size is 16 bytes. A virtual address is 8 bits wide.
- The top 4 bits of an address are the virtual page number. The low 4 bits are
  the offset.
- The TLB is fully associative, so an entry can hold any page.
- The TLB starts empty.
- A miss fills the lowest-numbered empty slot, when one exists.
- Once every slot holds an entry, a miss replaces the entry that has gone
  unreached for the longest. That entry is the least recently used one.
- A hit keeps its slot and its translation.
- A hit and a miss both refresh the entry's recency. The entry the access used
  becomes the most recent one.
- A hit costs one TLB lookup and the data access. A miss costs a TLB lookup, a
  page-table access, and the data access.
- A switch between two processes empties the TLB, unless the run uses address
  space identifiers.
- The TLB holds 4 entries unless the case states another number.
- One TLB lookup costs 1 ns. One main memory access costs 100 ns.
- The effective access time is 1 ns for the lookup, plus 100 ns for the data
  access, plus the miss rate times 100 ns. The summary prints the hit rate to
  four decimal places and both times to two.

## The Columns

Each trace row is one access.

| Column | What it holds |
|---|---|
| `#` | The position of the access in the reference string |
| `proc` | The process that makes the access |
| `vaddr` | The virtual address |
| `vpn` | The virtual page number |
| `off` | The offset inside the page |
| `result` | `hit` or `miss` |
| `slot` | The entry that served the access, or the entry a miss filled |
| `tlb (mru to lru)` | The TLB after the access, from most to least recently used |

Without address space identifiers, an entry shows the page number alone. With
them, an entry shows the process label and the page number, as in `A0`.

The summary prints the access count, the hits, the misses, the hit rate, the
effective access time in nanoseconds, and the effective access time of a
machine with no TLB at all. The last figure is two memory accesses. A machine
with no TLB reads the page table for every access.

## The Cases

The simulator runs the six cases below. It accepts no other case.

Each command below prints the text under it. The text is the expected output.

### `walk`

One process reads an array of 16 integers. Each integer is 4 bytes. The array
starts at virtual address 0, so it covers four pages. The code reads the array
twice. The TLB holds four entries, so the whole array fits, and the second pass
hits every time.

```bash
./tlb --case walk
```

```
case: walk
model: page 16 B, vaddr 8 bits (vpn 4, offset 4), tlb 4 entries fully assoc LRU, asid off, tlb 1 ns, mem 100 ns
ref: A walk 0,16,4,2

trace
    # proc  vaddr  vpn  off  result slot  tlb (mru to lru)
    1 A         0    0    0  miss      0  [ 0 -- -- --]
    2 A         4    0    4  hit       0  [ 0 -- -- --]
    3 A         8    0    8  hit       0  [ 0 -- -- --]
    4 A        12    0   12  hit       0  [ 0 -- -- --]
    5 A        16    1    0  miss      1  [ 1  0 -- --]
    6 A        20    1    4  hit       1  [ 1  0 -- --]
    7 A        24    1    8  hit       1  [ 1  0 -- --]
    8 A        28    1   12  hit       1  [ 1  0 -- --]
    9 A        32    2    0  miss      2  [ 2  1  0 --]
   10 A        36    2    4  hit       2  [ 2  1  0 --]
   11 A        40    2    8  hit       2  [ 2  1  0 --]
   12 A        44    2   12  hit       2  [ 2  1  0 --]
   13 A        48    3    0  miss      3  [ 3  2  1  0]
   14 A        52    3    4  hit       3  [ 3  2  1  0]
   15 A        56    3    8  hit       3  [ 3  2  1  0]
   16 A        60    3   12  hit       3  [ 3  2  1  0]
   17 A         0    0    0  hit       0  [ 0  3  2  1]
   18 A         4    0    4  hit       0  [ 0  3  2  1]
   19 A         8    0    8  hit       0  [ 0  3  2  1]
   20 A        12    0   12  hit       0  [ 0  3  2  1]
   21 A        16    1    0  hit       1  [ 1  0  3  2]
   22 A        20    1    4  hit       1  [ 1  0  3  2]
   23 A        24    1    8  hit       1  [ 1  0  3  2]
   24 A        28    1   12  hit       1  [ 1  0  3  2]
   25 A        32    2    0  hit       2  [ 2  1  0  3]
   26 A        36    2    4  hit       2  [ 2  1  0  3]
   27 A        40    2    8  hit       2  [ 2  1  0  3]
   28 A        44    2   12  hit       2  [ 2  1  0  3]
   29 A        48    3    0  hit       3  [ 3  2  1  0]
   30 A        52    3    4  hit       3  [ 3  2  1  0]
   31 A        56    3    8  hit       3  [ 3  2  1  0]
   32 A        60    3   12  hit       3  [ 3  2  1  0]

summary
accesses         32
hits             28
misses            4
hit rate     0.8750
eat          113.50 ns
no tlb       200.00 ns
```

The first access of each page misses. The next three accesses of the same page
hit. A 16-byte page holds four integers. The second pass hits on every access.
Four misses out of 32 accesses is a hit rate of 0.8750.

### `walk-narrow`

The same reference string. The TLB holds two entries, so it can't hold all
four pages at once.

```bash
./tlb --case walk-narrow
```

```
case: walk-narrow
model: page 16 B, vaddr 8 bits (vpn 4, offset 4), tlb 2 entries fully assoc LRU, asid off, tlb 1 ns, mem 100 ns
ref: A walk 0,16,4,2

trace
    # proc  vaddr  vpn  off  result slot  tlb (mru to lru)
    1 A         0    0    0  miss      0  [ 0 --]
    2 A         4    0    4  hit       0  [ 0 --]
    3 A         8    0    8  hit       0  [ 0 --]
    4 A        12    0   12  hit       0  [ 0 --]
    5 A        16    1    0  miss      1  [ 1  0]
    6 A        20    1    4  hit       1  [ 1  0]
    7 A        24    1    8  hit       1  [ 1  0]
    8 A        28    1   12  hit       1  [ 1  0]
    9 A        32    2    0  miss      0  [ 2  1]
   10 A        36    2    4  hit       0  [ 2  1]
   11 A        40    2    8  hit       0  [ 2  1]
   12 A        44    2   12  hit       0  [ 2  1]
   13 A        48    3    0  miss      1  [ 3  2]
   14 A        52    3    4  hit       1  [ 3  2]
   15 A        56    3    8  hit       1  [ 3  2]
   16 A        60    3   12  hit       1  [ 3  2]
   17 A         0    0    0  miss      0  [ 0  3]
   18 A         4    0    4  hit       0  [ 0  3]
   19 A         8    0    8  hit       0  [ 0  3]
   20 A        12    0   12  hit       0  [ 0  3]
   21 A        16    1    0  miss      1  [ 1  0]
   22 A        20    1    4  hit       1  [ 1  0]
   23 A        24    1    8  hit       1  [ 1  0]
   24 A        28    1   12  hit       1  [ 1  0]
   25 A        32    2    0  miss      0  [ 2  1]
   26 A        36    2    4  hit       0  [ 2  1]
   27 A        40    2    8  hit       0  [ 2  1]
   28 A        44    2   12  hit       0  [ 2  1]
   29 A        48    3    0  miss      1  [ 3  2]
   30 A        52    3    4  hit       1  [ 3  2]
   31 A        56    3    8  hit       1  [ 3  2]
   32 A        60    3   12  hit       1  [ 3  2]

summary
accesses         32
hits             24
misses            8
hit rate     0.7500
eat          126.00 ns
no tlb       200.00 ns
```

At access 9 the miss fills slot 0, which held page 0. Page 0 was the least
recently used entry. The second pass then performs like the first. The TLB
keeps losing a page that the walk needs again.

### `stride`

One process reads 16 integers, one every 16 bytes. Each access lands on a
different page. No page is read twice.

```bash
./tlb --case stride
```

```
case: stride
model: page 16 B, vaddr 8 bits (vpn 4, offset 4), tlb 4 entries fully assoc LRU, asid off, tlb 1 ns, mem 100 ns
ref: A walk 0,16,16,1

trace
    # proc  vaddr  vpn  off  result slot  tlb (mru to lru)
    1 A         0    0    0  miss      0  [ 0 -- -- --]
    2 A        16    1    0  miss      1  [ 1  0 -- --]
    3 A        32    2    0  miss      2  [ 2  1  0 --]
    4 A        48    3    0  miss      3  [ 3  2  1  0]
    5 A        64    4    0  miss      0  [ 4  3  2  1]
    6 A        80    5    0  miss      1  [ 5  4  3  2]
    7 A        96    6    0  miss      2  [ 6  5  4  3]
    8 A       112    7    0  miss      3  [ 7  6  5  4]
    9 A       128    8    0  miss      0  [ 8  7  6  5]
   10 A       144    9    0  miss      1  [ 9  8  7  6]
   11 A       160   10    0  miss      2  [10  9  8  7]
   12 A       176   11    0  miss      3  [11 10  9  8]
   13 A       192   12    0  miss      0  [12 11 10  9]
   14 A       208   13    0  miss      1  [13 12 11 10]
   15 A       224   14    0  miss      2  [14 13 12 11]
   16 A       240   15    0  miss      3  [15 14 13 12]

summary
accesses         16
hits              0
misses           16
hit rate     0.0000
eat          201.00 ns
no tlb       200.00 ns
```

Every access misses, so the hit rate is 0.0000. The effective access time is
201.00 ns, which is higher than the 200.00 ns of a machine with no TLB. Each
access still pays the TLB lookup before it reads the page table.

### `switch-flush`

Two processes read the same four integers. Process A runs, then process B runs,
then A runs again. The run doesn't use address space identifiers, so the
switch empties the TLB.

```bash
./tlb --case switch-flush
```

```
case: switch-flush
model: page 16 B, vaddr 8 bits (vpn 4, offset 4), tlb 4 entries fully assoc LRU, asid off, tlb 1 ns, mem 100 ns
ref: A walk 0,4,4,1 B walk 0,4,4,1 A walk 0,4,4,1

trace
    # proc  vaddr  vpn  off  result slot  tlb (mru to lru)
    1 A         0    0    0  miss      0  [ 0 -- -- --]
    2 A         4    0    4  hit       0  [ 0 -- -- --]
    3 A         8    0    8  hit       0  [ 0 -- -- --]
    4 A        12    0   12  hit       0  [ 0 -- -- --]
    5 B         0    0    0  miss      0  [ 0 -- -- --]
    6 B         4    0    4  hit       0  [ 0 -- -- --]
    7 B         8    0    8  hit       0  [ 0 -- -- --]
    8 B        12    0   12  hit       0  [ 0 -- -- --]
    9 A         0    0    0  miss      0  [ 0 -- -- --]
   10 A         4    0    4  hit       0  [ 0 -- -- --]
   11 A         8    0    8  hit       0  [ 0 -- -- --]
   12 A        12    0   12  hit       0  [ 0 -- -- --]

summary
accesses         12
hits              9
misses            3
hit rate     0.7500
eat          126.00 ns
no tlb       200.00 ns
```

Each visit to a process starts cold. A misses on its first access, at access
1. The switch to B emptied the TLB, so B misses at access 5. The switch back to
A emptied it again, so A misses again at access 9.

### `switch-asid`

The same reference string. This run uses address space identifiers, so the TLB
keeps both processes at once.

```bash
./tlb --case switch-asid
```

```
case: switch-asid
model: page 16 B, vaddr 8 bits (vpn 4, offset 4), tlb 4 entries fully assoc LRU, asid on, tlb 1 ns, mem 100 ns
ref: A walk 0,4,4,1 B walk 0,4,4,1 A walk 0,4,4,1

trace
    # proc  vaddr  vpn  off  result slot  tlb (mru to lru)
    1 A         0    0    0  miss      0  [A0 -- -- --]
    2 A         4    0    4  hit       0  [A0 -- -- --]
    3 A         8    0    8  hit       0  [A0 -- -- --]
    4 A        12    0   12  hit       0  [A0 -- -- --]
    5 B         0    0    0  miss      1  [B0 A0 -- --]
    6 B         4    0    4  hit       1  [B0 A0 -- --]
    7 B         8    0    8  hit       1  [B0 A0 -- --]
    8 B        12    0   12  hit       1  [B0 A0 -- --]
    9 A         0    0    0  hit       0  [A0 B0 -- --]
   10 A         4    0    4  hit       0  [A0 B0 -- --]
   11 A         8    0    8  hit       0  [A0 B0 -- --]
   12 A        12    0   12  hit       0  [A0 B0 -- --]

summary
accesses         12
hits             10
misses            2
hit rate     0.8333
eat          117.67 ns
no tlb       200.00 ns
```

A's entry survives B's run, so access 9 hits. The hit rate rises from 0.7500 to
0.8333. The effective access time falls from 126.00 ns to 117.67 ns.

### `eat`

This case holds no reference string. It prints the effective access time for
several hit rates.

```bash
./tlb --case eat
```

```
case: eat
model: page 16 B, vaddr 8 bits (vpn 4, offset 4), tlb 4 entries fully assoc LRU, asid off, tlb 1 ns, mem 100 ns
effective access time
  hit rate  misses/1000  eat (ns)  vs no tlb
    1.0000            0    101.00      1.98x
    0.9900           10    102.00      1.96x
    0.9800           20    103.00      1.94x
    0.9500           50    106.00      1.89x
    0.9000          100    111.00      1.80x
    0.8000          200    121.00      1.65x
    0.7500          250    126.00      1.59x
    0.0000         1000    201.00      1.00x
```

The last column is the no-TLB cost divided by the effective access time. A
machine with no TLB reads the page table on every access and pays 200 ns.

## Files

| File | What it is |
|---|---|
| `tlb.c` | The simulator |
| `expected/<case>.txt` | The exact output that `make check` compares against |
| `Makefile` | The build and check rules |
| `README.md` | This file |
