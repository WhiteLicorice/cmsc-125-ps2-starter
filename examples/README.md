<!--no-pdf-->
# Activity Examples

This folder holds the worked examples for the activities that precede Problem
Set 2, Memory Mechanisms, Allocation, and Freeing. Each activity has its own
folder. Each folder holds its own `make` and `make check`.

These examples are not graded. They are not one of the stages of the problem
set. Use them to see the mechanism before you write the code.

| Folder | Activity |
|:--|:--|
| `act8` | Paging, Page Tables, and Address Translation |
| `act9` | Translation Lookaside Buffers |
| `act10` | Beyond Physical Memory, and the Whole Memory Path |

Build and check one folder:

```bash
cd examples/act8
make
make check
make clean
```

`make` builds the examples. `make check` builds them and compares the output
with the recorded output. `make clean` removes the build output. Read the
`README.md` in each folder for the model and the cases.

Run these commands inside Ubuntu. The examples need Linux.
