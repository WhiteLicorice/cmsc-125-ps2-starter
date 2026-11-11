<!--no-pdf-->
# Activity 8 Examples

`pagesim.c` is a deterministic paging translator. `demo_cases.c` holds the
student case list. A translation case prints one page table and one table of
translated addresses. The size case prints the size of a linear page table for
several address widths. The recorded output of every case is in `expected/`.
Every case runs on the same model, so the same input always produces the same
output on every host.

This folder holds the worked-example cases only.

## The Model

The translator uses one fixed model. The activity grades with the same model.

- A page is 256 bytes. A frame holds one page, so a frame is also 256 bytes.
- A virtual address is 12 bits wide. The top 4 bits are the virtual page
  number, or VPN. The low 8 bits are the offset.
- A physical address is 11 bits wide. The top 3 bits are the page frame number,
  or PFN. The low 8 bits are the offset.
- The virtual space holds 16 pages. Physical memory holds 8 frames.
- The page table holds 16 entries, one per VPN. Entry `i` describes VPN `i`.
- A zero valid bit marks an unmapped page. An access through it faults.
- An entry with `valid` 1 holds a PFN, a protection code, a present bit, a
  dirty bit, and an accessed bit.
- The protection code is three letters, R, W, and X, in that order. A dash
  marks a denied access.
- The hardware copies the offset through unchanged.
- A read needs R. A write needs W. An instruction fetch needs X.
- An access through a mapped page with no matching bit is a protection fault.
- The translator uses 4 bytes for one page-table entry.

## Build and Check

Run these commands in this directory.

Build the translator:

```bash
make
```

Build every case. Run it. Compare the output:

```bash
make check
```

`make check` exits 0 only when every case passes. It prints one line per case,
then a count. The count is `4 cases, 4 passed, 0 failed` on success. A failing
case prints `FAIL` with the case label, then the count at the end says how many
cases passed and how many cases failed.

Remove the built program:

```bash
make clean
```

The Makefile compiles the translator with:

```bash
cc -std=c11 -Wall -Wextra -Werror
```

## The Cases

| Case | Command | What it shows |
|:--|:--|:--|
| `basic` | `./pagesim basic` | Four addresses through the worked page table, all mapped |
| `offset` | `./pagesim offset` | Two addresses in one page, so both use one frame |
| `faults` | `./pagesim faults` | One successful access, two protection faults, and one unmapped fault |
| `sizes` | `./pagesim sizes` | The size of a linear page table at three address widths, and the two Sv32 levels |

Run the translator with no argument to print every case at once.

## Recorded Output

Each block below is the exact text in `expected/<case>.txt`.

### Basic

```text
case basic
page size 256 bytes, virtual address 12 bits, physical address 11 bits
virtual pages 16, frames 8, vpn bits 4, offset bits 8

page table
  vpn  valid  pfn   prot   present  dirty  accessed
  0    1      3     RWX    1        1      1
  1    1      0     R--    1        0      1
  2    1      7     RW-    1        1      1
  3    1      1     R-X    1        0      1
  4    1      5     RW-    1        0      1
  5    0      -     -      -        -      -
  6    0      -     -      -        -      -
  7    0      -     -      -        -      -
  8    0      -     -      -        -      -
  9    0      -     -      -        -      -
  10   0      -     -      -        -      -
  11   0      -     -      -        -      -
  12   0      -     -      -        -      -
  13   0      -     -      -        -      -
  14   0      -     -      -        -      -
  15   0      -     -      -        -      -

translation
  vaddr   bits           vpn  offset  op  result
  0x0C5   0000 11000101  0    0xC5    R   frame 3, physical 0x3C5
  0x2A0   0010 10100000  2    0xA0    R   frame 7, physical 0x7A0
  0x4FF   0100 11111111  4    0xFF    W   frame 5, physical 0x5FF
  0x3B4   0011 10110100  3    0xB4    X   frame 1, physical 0x1B4

```

### Offset

```text
case offset
page size 256 bytes, virtual address 12 bits, physical address 11 bits
virtual pages 16, frames 8, vpn bits 4, offset bits 8

page table
  vpn  valid  pfn   prot   present  dirty  accessed
  0    1      3     RWX    1        1      1
  1    1      0     R--    1        0      1
  2    1      7     RW-    1        1      1
  3    1      1     R-X    1        0      1
  4    1      5     RW-    1        0      1
  5    0      -     -      -        -      -
  6    0      -     -      -        -      -
  7    0      -     -      -        -      -
  8    0      -     -      -        -      -
  9    0      -     -      -        -      -
  10   0      -     -      -        -      -
  11   0      -     -      -        -      -
  12   0      -     -      -        -      -
  13   0      -     -      -        -      -
  14   0      -     -      -        -      -
  15   0      -     -      -        -      -

translation
  vaddr   bits           vpn  offset  op  result
  0x100   0001 00000000  1    0x00    R   frame 0, physical 0x000
  0x1FF   0001 11111111  1    0xFF    R   frame 0, physical 0x0FF

```

### Faults

```text
case faults
page size 256 bytes, virtual address 12 bits, physical address 11 bits
virtual pages 16, frames 8, vpn bits 4, offset bits 8

page table
  vpn  valid  pfn   prot   present  dirty  accessed
  0    1      3     RWX    1        1      1
  1    1      0     R--    1        0      1
  2    1      7     RW-    1        1      1
  3    1      1     R-X    1        0      1
  4    1      5     RW-    1        0      1
  5    0      -     -      -        -      -
  6    0      -     -      -        -      -
  7    0      -     -      -        -      -
  8    0      -     -      -        -      -
  9    0      -     -      -        -      -
  10   0      -     -      -        -      -
  11   0      -     -      -        -      -
  12   0      -     -      -        -      -
  13   0      -     -      -        -      -
  14   0      -     -      -        -      -
  15   0      -     -      -        -      -

translation
  vaddr   bits           vpn  offset  op  result
  0x1A0   0001 10100000  1    0xA0    W   protection fault: page 1 does not allow W
  0x2A0   0010 10100000  2    0xA0    X   protection fault: page 2 does not allow X
  0x9FF   1001 11111111  9    0xFF    R   unmapped: page 9 is not valid
  0x1A0   0001 10100000  1    0xA0    R   frame 0, physical 0x0A0

```

### Sizes

```text
case sizes
linear page table size
  addr bits page bytes  vpn bits entries   pte bytes table bytes table size
  32        4096        20       1048576   4         4194304     4 MiB
  32        16384       18       262144    4         1048576     1 MiB
  20        4096        8        256       4         1024        1 KiB

sv32 two level
  virtual address 32 bits, page size 4096 bytes, offset bits 12
  level    vpn bits  entries  pte bytes  table bytes  table size
  level-1  10        1024     4          4096         4 KiB
  level-0  10        1024     4          4096         4 KiB

```

## Where to Build and Run

Build these inside Ubuntu. Run them there, on any of the three hosts:

| Host | The Ubuntu you use |
|:--|:--|
| Windows | WSL2, from b1 |
| macOS | The Ubuntu 24.04 Server virtual machine from b1 Part 4, run under UTM |
| Linux | Your own kernel |

`pagesim.c` uses the C standard library only. Build and run it inside that
Ubuntu.

## Machine Used for These Runs

```text
cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
Linux 6.6.87.2-microsoft-standard-WSL2 x86_64
GNU Make 4.3
```
