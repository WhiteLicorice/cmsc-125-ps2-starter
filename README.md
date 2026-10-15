<!--no-pdf-->
# CMSC 125 Problem Set 2 Starter

This C17 project is the starter for the CMSC 125 Problem Set 2 allocator and
address translator. One program holds two machines. The allocator serves a
fixed arena with a free list, splits blocks, coalesces neighbours, and follows
one placement policy. The translator turns an offset inside a segment into a
physical address, or reports the fault the access raises. A script drives both.

This project runs on Linux, on macOS, and on Windows through WSL2 Ubuntu. It
uses the POSIX `mmap` call and nothing else outside the C standard library.

## Pair

Replace the two entries below. An assigned trio adds one entry.

- Full Name (`@github-username`)
- Full Name (`@github-username`)

## Files You May Change

You may change only these files and folders:

- `src/script.c`
- `src/alloc.c`
- `src/translate.c`
- `src/report.c`
- `ANALYSIS.md`, which you create
- The Pair section of `README.md`
- `student-cases/`, which holds the one case of your own

Everything else is fixed. `include/ps2.h`, `src/main.c`, and `src/arena.c` are
given to you complete. `cases/`, `tools/check_cases.py`, and the workflow are
the grading corpus and its runner. The manual states every rule these files
follow.

## Layout

```
include/ps2.h         fixed interface: constants, codes, structures, prototypes
src/main.c            complete: the command line, the request loop, the exit codes
src/arena.c           complete: the arena, the header accessors, the rounding
src/script.c          yours: read and validate the script file
src/alloc.c           yours: the free list, the policies, splitting, coalescing
src/translate.c       yours: the segment table and the translations
src/report.c          yours: every line the program writes
cases/                the complete public corpus
student-cases/        the one boundary or failure case of your own
tools/check_cases.py  the runner that grades the corpus
ANALYSIS.md           the joint analysis that you create
Makefile run check.sh  the run contract
```

## Commands

```bash
make                      # build build/cmsc-125-ps2
make clean                # remove the build directory
./run --policy first cases/arena/first_block.script
./check.sh                # build, then run every published case in cases/
./check.sh student-cases  # build, then run the cases in student-cases/
```

`./run` passes its arguments to the binary.

`check.sh` is the complete public automated check. No hidden suite exists. Read
`tools/check_cases.py` before you trust a green run. The rubric separately
assesses the joint analysis and the collaboration history.

## Exit Codes

| Code | Meaning |
|---|---|
| 0 | Every request ran. |
| 65 | The command line or the script file was refused. |
| 70 | An ownership error, or the arena ran out. |

## The First Run

A fresh clone builds without warnings. It passes 6 of the 104 cases and exits 1.
That number needs care. The 6 passing cases test nothing you wrote. They are the
command-line refusals, and `src/main.c` is given complete, so a bad option is
refused before your code runs.

The other 98 cases need `src/script.c`, `src/alloc.c`, `src/translate.c`, and
`src/report.c`. The workflow in the Actions tab is red for the same reason. That
state is correct for a starter. Your fork passes after you implement the four
files.

On the fork, open the Actions tab once and enable workflows. GitHub disables
them on every fork, and the workflow run is part of the submission.

## Hosts

### Linux

```bash
sudo apt update
sudo apt install -y build-essential make python3
```

### macOS

The Xcode command line tools carry the compiler and `make`.

```bash
xcode-select --install
```

Check the Python 3 version. macOS ships Python 3 with the command line tools.

```bash
python3 --version
```

### Windows through WSL2 Ubuntu

Install WSL2 with Ubuntu from PowerShell.

```powershell
wsl --install -d Ubuntu
```

Open the Ubuntu terminal and install the tools inside it.

```bash
sudo apt update
sudo apt install -y build-essential make python3
```

Work inside the Ubuntu file system, which is your home directory. A checkout
under `/mnt/c` works but it is slower.

### Check

```bash
cc --version
make --version
python3 --version
```

## The Corpus

The 104 cases under `cases/` are the cases the instructor grades with. Each
`.script` file holds a machine description and a list of requests. Each `.args`
file holds the command line. Each `.expect` file holds the expected exit code,
the expected diagnostic, and every expected output line.

The one case your pair writes lives under `student-cases/`. That folder holds a
short README and an example in the same three-file form. Run it with
`./check.sh student-cases`.

The runner compares every output line exactly. It also derives the run's own
numbers from its own output. It checks the alignment of each address, the size
of each block against the split rule, the tiling of the arena by the live blocks
and the free list, every dump against the free space the live blocks leave, the
summary against the tiling, and every translation against its segment row. A
case file that agrees with a wrong program still fails.

