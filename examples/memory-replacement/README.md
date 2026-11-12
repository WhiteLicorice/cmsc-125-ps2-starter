<!--no-pdf-->
# Memory Replacement

This folder holds one program. `repl.c` is a page-replacement simulator. It
implements the model below. It prints the replacement traces
and the fault path for the three cases below. The program is deterministic, so
the same command prints the same output on every machine.

The program runs on Linux. On Windows, run it inside Ubuntu under Windows
Subsystem for Linux (WSL2). On macOS, run it inside an Ubuntu 24.04 Server
virtual machine under UTM. The bootcamp manual "Getting Your Machine Ready" has
the setup for that virtual machine. On Linux, run it in your own terminal. Run
every command inside the Ubuntu guest, on Windows or on a Mac.

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
cc -std=c11 -Wall -Wextra -Werror -o repl repl.c
```

The compile must stay clean. `-Werror` turns every warning into an error.

## How the Check Decides

`make check` runs each case in turn. It compares the output with
`expected/<case>.txt`. The simulator is deterministic, so every comparison is
exact. It prints one line for each case, then a count line. It exits 0 when
every case passes. It exits nonzero when a case fails.

## The Model

The simulator implements the model below.

- The page size is 4 KiB. Every address is 32 bits.
- A reference string is a list of virtual page numbers. One step accesses one
  page. Every access is a load.
- The frames start empty. The first access to a page is a miss.
- A miss fills an empty frame when one exists. Otherwise the miss evicts one
  resident page under the policy in force.
- A victim tie breaks to the lowest page number.
- The random policy draws every victim from one fixed seed, so its trace
  repeats on every machine.
- The clock hand starts at slot 0 and moves forward one slot at a time. A
  reference sets the use bit of its frame. On a miss, the hand clears use bits
  until it finds one that's already clear, and it evicts that frame.
- The trace prints the frames in slot order. A dot marks an empty frame.
- The fault path adds a TLB with two entries and a FIFO replacement rule. It
  follows one load through the TLB, the page table, one page fault, and one
  replacement. It clears the victim's present bit before the frame is reused.

## The Cases

The simulator runs the three cases below. It takes no other workload.

Each command below prints the text under it. The text is the expected output.

### `policies`

The first case runs one reference string under five policies.

```bash
./repl --case policies
```

```
case: policies
model: page 4 KiB, reference 0 1 2 0 1 3 0 1 2 4 0 1
       frames start empty, first access to a page misses, ties break to the lowest page number

policy optimal, frames 3
  ref  page  frames  result  evicted
    0     0  0 . .   miss    -
    1     1  0 1 .   miss    -
    2     2  0 1 2   miss    -
    3     0  0 1 2   hit     -
    4     1  0 1 2   hit     -
    5     3  0 1 3   miss    2
    6     0  0 1 3   hit     -
    7     1  0 1 3   hit     -
    8     2  0 1 2   miss    3
    9     4  0 1 4   miss    2
   10     0  0 1 4   hit     -
   11     1  0 1 4   hit     -
  hits 6, misses 6, hit rate 50.00%

policy fifo, frames 3
  ref  page  frames  result  evicted
    0     0  0 . .   miss    -
    1     1  0 1 .   miss    -
    2     2  0 1 2   miss    -
    3     0  0 1 2   hit     -
    4     1  0 1 2   hit     -
    5     3  3 1 2   miss    0
    6     0  3 0 2   miss    1
    7     1  3 0 1   miss    2
    8     2  2 0 1   miss    3
    9     4  2 4 1   miss    0
   10     0  2 4 0   miss    1
   11     1  1 4 0   miss    2
  hits 2, misses 10, hit rate 16.67%

policy random, frames 3
  ref  page  frames  result  evicted
    0     0  0 . .   miss    -
    1     1  0 1 .   miss    -
    2     2  0 1 2   miss    -
    3     0  0 1 2   hit     -
    4     1  0 1 2   hit     -
    5     3  0 1 3   miss    2
    6     0  0 1 3   hit     -
    7     1  0 1 3   hit     -
    8     2  2 1 3   miss    0
    9     4  4 1 3   miss    2
   10     0  0 1 3   miss    4
   11     1  0 1 3   hit     -
  hits 5, misses 7, hit rate 41.67%

policy lru, frames 3
  ref  page  frames  result  evicted
    0     0  0 . .   miss    -
    1     1  0 1 .   miss    -
    2     2  0 1 2   miss    -
    3     0  0 1 2   hit     -
    4     1  0 1 2   hit     -
    5     3  0 1 3   miss    2
    6     0  0 1 3   hit     -
    7     1  0 1 3   hit     -
    8     2  0 1 2   miss    3
    9     4  4 1 2   miss    0
   10     0  4 0 2   miss    1
   11     1  4 0 1   miss    2
  hits 4, misses 8, hit rate 33.33%

policy clock, frames 3
  ref  page  frames  result  evicted
    0     0  0 . .   miss    -
    1     1  0 1 .   miss    -
    2     2  0 1 2   miss    -
    3     0  0 1 2   hit     -
    4     1  0 1 2   hit     -
    5     3  3 1 2   miss    0
    6     0  3 0 2   miss    1
    7     1  3 0 1   miss    2
    8     2  2 0 1   miss    3
    9     4  2 4 1   miss    0
   10     0  2 4 0   miss    1
   11     1  1 4 0   miss    2
  hits 2, misses 10, hit rate 16.67%

summary
  policy   frames   hits  misses  hit rate
  optimal       3      6       6    50.00%
  fifo          3      2      10    16.67%
  random        3      5       7    41.67%
  lru           3      4       8    33.33%
  clock         3      2      10    16.67%
```

### `belady`

The second case runs one reference string under FIFO at three frames and at
four frames.

```bash
./repl --case belady
```

```
case: belady
model: page 4 KiB, reference 1 2 3 4 1 2 5 1 2 3 4 5
       frames start empty, first access to a page misses, ties break to the lowest page number

policy fifo, frames 3
  ref  page  frames  result  evicted
    0     1  1 . .   miss    -
    1     2  1 2 .   miss    -
    2     3  1 2 3   miss    -
    3     4  4 2 3   miss    1
    4     1  4 1 3   miss    2
    5     2  4 1 2   miss    3
    6     5  5 1 2   miss    4
    7     1  5 1 2   hit     -
    8     2  5 1 2   hit     -
    9     3  5 3 2   miss    1
   10     4  5 3 4   miss    2
   11     5  5 3 4   hit     -
  hits 3, misses 9, hit rate 25.00%

policy fifo, frames 4
  ref  page  frames    result  evicted
    0     1  1 . . .   miss    -
    1     2  1 2 . .   miss    -
    2     3  1 2 3 .   miss    -
    3     4  1 2 3 4   miss    -
    4     1  1 2 3 4   hit     -
    5     2  1 2 3 4   hit     -
    6     5  5 2 3 4   miss    1
    7     1  5 1 3 4   miss    2
    8     2  5 1 2 4   miss    3
    9     3  5 1 2 3   miss    4
   10     4  4 1 2 3   miss    5
   11     5  4 5 2 3   miss    1
  hits 2, misses 10, hit rate 16.67%

policy optimal, frames 3
  ref  page  frames  result  evicted
    0     1  1 . .   miss    -
    1     2  1 2 .   miss    -
    2     3  1 2 3   miss    -
    3     4  1 2 4   miss    3
    4     1  1 2 4   hit     -
    5     2  1 2 4   hit     -
    6     5  1 2 5   miss    4
    7     1  1 2 5   hit     -
    8     2  1 2 5   hit     -
    9     3  3 2 5   miss    1
   10     4  3 4 5   miss    2
   11     5  3 4 5   hit     -
  hits 5, misses 7, hit rate 41.67%

policy optimal, frames 4
  ref  page  frames    result  evicted
    0     1  1 . . .   miss    -
    1     2  1 2 . .   miss    -
    2     3  1 2 3 .   miss    -
    3     4  1 2 3 4   miss    -
    4     1  1 2 3 4   hit     -
    5     2  1 2 3 4   hit     -
    6     5  1 2 3 5   miss    4
    7     1  1 2 3 5   hit     -
    8     2  1 2 3 5   hit     -
    9     3  1 2 3 5   hit     -
   10     4  4 2 3 5   miss    1
   11     5  4 2 3 5   hit     -
  hits 6, misses 6, hit rate 50.00%

summary
  policy   frames   hits  misses  hit rate
  fifo          3      3       9    25.00%
  fifo          4      2      10    16.67%
  optimal       3      5       7    41.67%
  optimal       4      6       6    50.00%
```

### `faultpath`

The third case follows one load from its virtual address through the TLB, the
page table, one page fault, and one replacement.

```bash
./repl --case faultpath
```

```
case: faultpath
model: page 4 KiB, frames 3, tlb 2 entries fifo, replacement fifo
       the frames hold pages 2, 5, 7 in load order
       the tlb holds, oldest first, vpn 5 -> frame 1 and vpn 7 -> frame 2
       the dirty pages are 2; every other page is clean

load  0x00003044 (vpn 3, offset 68)

  step  action                          detail
    1  split the virtual address       vpn 3, offset 68
    2  look up the vpn in the tlb      the tlb holds vpn 5 and vpn 7, so vpn 3 misses
    3  look up the page table          the pte for vpn 3 has present bit 0
    4  raise a page fault              trap to the kernel
    5  locate the page                 vpn 3 sits in swap
    6  pick a victim (fifo)            frame 0 holds vpn 2, loaded first
    7  check the victim dirty bit      vpn 2 is dirty, write it back to swap
    8  clear the victim's present bit  vpn 2: present 0, copy in swap
    9  read the page from swap         vpn 3 into frame 0
   10  update the page table           vpn 3 -> frame 0, present 1
   11  update the tlb (fifo)           insert vpn 3 -> frame 0, evict vpn 5
   12  restart the load                tlb hit, frame 0
   13  form the physical address       frame 0 * 4 KiB + 68

note: vpn 2 holds no tlb entry, so no victim invalidation runs
frames after the load: page 3 in frame 0, page 5 in frame 1, page 7 in frame 2
tlb after the load: vpn 7 -> frame 2, vpn 3 -> frame 0
physical address 0x00000044
counts: tlb misses 1, page-table lookups 1, page faults 1, evictions 1, victim ptes cleared 1, write-backs 1
```

## Where to Build and Run

Build and run these inside Ubuntu, on any of the three hosts:

| Host | The Ubuntu you use |
|:--|:--|
| Windows | WSL2 Ubuntu |
| macOS | An Ubuntu 24.04 Server virtual machine under UTM |
| Linux | Your own kernel |

`repl.c` uses the C standard library only. It builds and runs on native macOS
and on native Linux too.

## Files

| File | What it is |
|---|---|
| `repl.c` | The simulator |
| `expected/<case>.txt` | The exact output that `make check` compares against |
| `Makefile` | The build and check rules |
| `README.md` | This file |

## Machine Used for These Runs

```
cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
Linux 6.6.87.2-microsoft-standard-WSL2 x86_64
GNU Make 4.3
```
