<!--no-pdf-->
# Worked Examples

This folder holds the worked examples for three Unit 1 subtopics that Problem
Set 2, Memory Mechanisms, Allocation, and Freeing, draws on in its Background
section. Each subtopic has its own folder. Each folder holds its own `make` and
`make check`.

These examples aren't graded. They aren't part of the work you submit. Use them
to see the mechanism before you write the code.

| Folder | Subtopic |
|:--|:--|
| `paging-and-page-tables` | Paging and Page Tables |
| `translation-lookaside-buffers` | Translation Lookaside Buffers |
| `memory-replacement` | Memory Replacement |

Build and check one folder:

```bash
cd examples/paging-and-page-tables
make
make check
make clean
```

`make` builds the examples. `make check` builds them and compares the output
with the recorded output. `make clean` removes the build output. Read the
`README.md` in each folder for the model and the cases.

Run these commands inside Ubuntu. The examples need Linux.
