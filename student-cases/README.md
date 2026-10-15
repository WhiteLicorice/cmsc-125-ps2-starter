<!--no-pdf-->
# Your Own Case

This folder holds the one boundary or failure case your pair writes for the
rubric's implementation row. The published corpus lives in `cases/`, and it
stays unchanged.

The three files named `example` form one case. Replace them with your own case.
A case you did not write earns no marks, so keep the example only until you
have a better one.

## One Case, Three Files

A case is a `.script` file with two neighbours.

- `name.script` holds the machine rows and the requests.
- `name.args` holds the command line, such as `--policy best`.
- `name.expect` holds the expected exit code and the expected output.

The manual's "The Script File", "The Allocator", and "The Report" sections state
every rule. The runner reads the same three files for the published corpus, so a
case here is written the same way.

An `.expect` file holds one directive on a line.

| Directive | Meaning |
|---|---|
| `exit <code>` | the exit code the run must return |
| `stderr <text>` | a substring stderr must hold, repeatable |
| `stdout <line>` | the next line of standard output, in order |
| `path <relative>` | pass this script path in place of the case file |
| `argv <arguments>` | run this command line instead of the case's own |

Name a boundary the published corpus misses. A split that leaves exactly the
split threshold, a request the arena cannot hold even though the free total says
it can, and a translation at the highest legal offset are all good choices.

## Run Your Case

```bash
make
./check.sh student-cases
```

`./check.sh` with no argument runs the published corpus in `cases/`. With
`student-cases` it runs the cases in this folder instead.

## Show It Detects a Defect

The rubric asks for the failing record, not only a green case. Break one
mechanism in `src/`, run `./check.sh student-cases`, and keep the output that
names your case. Revert the break. Paste the record into `ANALYSIS.md`.

A case that stays green with the defect present proves nothing.
