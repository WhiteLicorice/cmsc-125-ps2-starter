#!/usr/bin/env python3
"""
check_cases.py: the CMSC 125 Problem Set 2 public case runner.

The runner runs the student's own ./run entry point on each .script file under
the case directory. Each case names its command line in a matching .args file
and its expectations in a matching .expect file.

An .expect file holds one directive on each line:

    exit <code>          the exit code the run must return
    stderr <text>        a substring stderr must hold (repeatable)
    stdout <line>        the next line of standard output, in order
    path <relative>      pass this script path in place of the case file
    argv <arguments>     run this command line instead of the case's own

Every line the program writes to standard output is compared, in order and
exactly, with the stdout directives. That comparison alone would let a case
file and a broken program agree, so the runner also derives the run's own
numbers from its own output and checks them:

  * the arena line states the contract's alignment, header, and split threshold,
    and the policy the command line asked for;
  * each allocation's address is a multiple of the alignment, sits a header
    past the block's offset, and the block is either exactly what the request
    needs or a whole block that the split rule refused to divide;
  * no two live blocks overlap, and the live blocks and the free list tile the
    arena with no gap and no double count;
  * every dump lists the free blocks the live blocks leave, in address order,
    and nothing else;
  * an allocation that follows a dump takes the block the placement policy
    picks from the free list that dump showed;
  * the summary's allocated, free, largest, blocks, and internal follow from
    that tiling, and its external ratio follows from free and largest;
  * every translation's physical address follows from the segment row, and
    every fault follows from the bound and the protection field;
  * the result line agrees with the exit code.

A difference the derivation explains is a failure, whatever the case file
says. The derivation is what keeps a case from being a frozen copy of one
program's output.

Exit code: 0 when every case passes, 1 otherwise.
"""

import argparse
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

# The contract's constants, restated here so the runner can check them.
ALIGN = 8
HEADER = 16
MIN_BLOCK = 32

TIMEOUT_SECONDS = 30

POLICIES = ("first", "best", "next")
SEGMENTS = ("code", "heap", "stack")
OPERATIONS = ("read", "write", "exec")

ARENA_RE = re.compile(
    r"^arena (\d+) align (\d+) header (\d+) min_block (\d+) "
    r"policy (first|best|next)$"
)
SEG_RE = re.compile(
    r"^seg (code|heap|stack) base (\d+) bound (\d+) grow (up|down) "
    r"prot ([rwx-]{3})$"
)
ALLOC_RE = re.compile(r"^alloc (\S+) size (\d+) addr (\d+) block (\d+)$")
FREE_RE = re.compile(r"^free (\S+) addr (\d+) block (\d+)$")
FREEAT_RE = re.compile(r"^freeat (\d+) block (\d+)$")
TOUCH_RE = re.compile(r"^touch (\S+) addr (\d+)$")
TRANSLATE_RE = re.compile(
    r"^translate (code|heap|stack) offset (\d+) (read|write|exec) "
    r"(?:addr (\d+)|fault (bounds|protect))$"
)
SUMMARY_RE = re.compile(
    r"^summary allocated (\d+) free (\d+) largest (\d+) blocks (\d+) "
    r"internal (\d+) external (\d+\.\d{6})$"
)
RESULT_RE = re.compile(r"^result (complete|faulted)$")


class CaseError(Exception):
    """A problem in a case file, reported as a message rather than a traceback."""


# --------------------------------------------------------------------------
# Case files
# --------------------------------------------------------------------------


def parse_expect(path):
    """Reads an .expect file into a dict of directives."""
    spec = {"exit": 0, "stderr": [], "stdout": [], "path": None, "argv": None}
    for line in path.read_text(encoding="utf-8").splitlines():
        text = line.strip()
        if not text or text.startswith("#"):
            continue
        word, _, rest = text.partition(" ")
        rest = rest.strip()
        if word == "exit":
            spec["exit"] = int(rest)
        elif word == "stderr":
            spec["stderr"].append(rest)
        elif word == "stdout":
            spec["stdout"].append(rest)
        elif word == "path":
            spec["path"] = rest
        elif word == "argv":
            spec["argv"] = rest.split()
        else:
            raise CaseError(f"'{path.name}' holds an unknown directive '{word}'.")
    return spec


def parse_args_file(path):
    """Reads a .args file. Absent means policy first."""
    if not path.exists():
        return ["--policy", "first"]
    text = path.read_text(encoding="utf-8")
    return text.split("#", 1)[0].split()


def policy_of(args):
    if "--policy" in args:
        index = args.index("--policy") + 1
        if index < len(args):
            return args[index]
    return "first"


def parse_script(path):
    """Reads the machine rows of a script, leniently.

    A malformed script is still a case: the refusal cases are scripts the
    program must reject, so this reader takes what it can and leaves the
    checking to the derivation.
    """
    machine = {"arena": None, "segments": {}}
    for line in path.read_text(encoding="utf-8").splitlines():
        text = line.split("#", 1)[0].strip()
        if not text:
            continue
        fields = text.split()
        if fields[0] == "arena" and len(fields) == 2 and fields[1].isdigit():
            machine["arena"] = int(fields[1])
        elif fields[0] == "seg" and len(fields) == 6:
            machine["segments"][fields[1]] = {
                "base": int(fields[2]) if fields[2].isdigit() else None,
                "bound": int(fields[3]) if fields[3].isdigit() else None,
                "grow": fields[4],
                "prot": fields[5],
            }
    return machine


# --------------------------------------------------------------------------
# Running a case
# --------------------------------------------------------------------------


def run_program(cmd, repo_root):
    """Runs the entry point and returns (stdout, stderr, code)."""
    try:
        result = subprocess.run(cmd, cwd=repo_root, capture_output=True,
                                text=True, encoding="utf-8", errors="replace",
                                timeout=TIMEOUT_SECONDS)
    except subprocess.TimeoutExpired:
        return "", f"TIMEOUT: the run took more than {TIMEOUT_SECONDS}s.\n", -1
    except OSError as error:
        return "", f"the run could not start: {error}\n", -1
    return result.stdout, result.stderr, result.returncode


def round_up(value, unit):
    remainder = value % unit
    if remainder == 0:
        return value
    return value + (unit - remainder)


def parse_dump(line):
    """Reads one dump line into (offset, size) pairs, or None.

    The line holds the word dump, then one offset and one size for every free
    block, in address order. An empty free list prints "dump none" instead, so
    one line is always one snapshot.
    """
    if line == "dump none":
        return []
    if not line.startswith("dump "):
        return None
    fields = line[len("dump "):].split()
    if not fields or len(fields) % 2 != 0:
        return None
    try:
        values = [int(field) for field in fields]
    except ValueError:
        return None
    return list(zip(values[0::2], values[1::2]))


# --------------------------------------------------------------------------
# Comparison
# --------------------------------------------------------------------------


def compare_output(actual, expected, problems):
    """Compares the run's lines with the case's lines, in order and exactly."""
    if actual == expected:
        return
    for index in range(max(len(actual), len(expected))):
        got = actual[index] if index < len(actual) else None
        want = expected[index] if index < len(expected) else None
        if got == want:
            continue
        if want is None:
            problems.append(f"stdout line {index + 1} is '{got}', but the case "
                            "expects no line there")
        elif got is None:
            problems.append(f"stdout ends after {len(actual)} lines, but the "
                            f"case expects '{want}' at line {index + 1}")
        else:
            problems.append(f"stdout line {index + 1} is '{got}', expected "
                            f"'{want}'")


# --------------------------------------------------------------------------
# The derivation
# --------------------------------------------------------------------------


def gaps_of(live, arena):
    """The free space the live blocks leave, in address order."""
    spans = sorted((start, start + block) for (start, block, _) in live.values())
    gaps = []
    cursor = 0
    overlap = None
    for start, end in spans:
        if start < cursor:
            overlap = (cursor, start)
            break
        if start > cursor:
            gaps.append((cursor, start - cursor))
        cursor = end
    if overlap is None and cursor < arena:
        gaps.append((cursor, arena - cursor))
    return gaps, overlap


def choose_from(free_blocks, needed, policy, cursor):
    """The offset the policy picks from a known free list, or None."""
    if policy == "first":
        for offset, size in free_blocks:
            if size >= needed:
                return offset
        return None
    if policy == "best":
        best = None
        for offset, size in free_blocks:
            if size >= needed and (best is None or size < best[1]):
                best = (offset, size)
        return best[0] if best is not None else None
    for offset, size in free_blocks:
        if offset >= cursor and size >= needed:
            return offset
    for offset, size in free_blocks:
        if size >= needed:
            return offset
    return None


def check_dump(dumped, live, arena, label, problems):
    """Checks one dump against the free space the live blocks leave."""
    seen = [offset for offset, _ in dumped]
    for offset, size in dumped:
        if size < HEADER + ALIGN:
            problems.append(f"{label}: a free block of {size} bytes is smaller "
                            f"than the {HEADER + ALIGN} bytes the smallest "
                            "allocation needs")
        if offset % ALIGN != 0:
            problems.append(f"{label}: the free block at {offset} does not start "
                            f"on a multiple of {ALIGN}")
    for first, second in zip(seen, seen[1:]):
        if second <= first:
            problems.append(f"{label}: the free blocks are not in address "
                            f"order: {first} then {second}")
    if arena is None:
        return
    gaps, overlap = gaps_of(live, arena)
    if overlap is not None:
        problems.append(f"{label}: two live blocks overlap at {overlap[1]}")
        return
    if dumped != gaps:
        want = ", ".join(f"({o}, {s})" for o, s in gaps) or "none"
        got = ", ".join(f"({o}, {s})" for o, s in dumped) or "none"
        problems.append(f"{label}: the free list is {got}, but the live blocks "
                        f"leave {want}")


def derive(lines, machine, policy, exit_code, problems):
    """Derives the run's numbers from its own output and checks them.

    Runs only when the program printed a report. Everything here follows from
    the lines themselves plus the contract's arithmetic, so a case file that
    agrees with a wrong run still fails.
    """
    arena = machine["arena"]
    live = {}
    summary = None
    result = None
    saw_arena = False
    saw_arena_size = None
    segment_order = []
    number = 0
    # The free list the last dump showed, kept only while nothing has changed
    # since. An allocation that follows a dump must choose from that list.
    shown_free = None
    cursor = 0

    while number < len(lines):
        line = lines[number]
        number += 1
        label = f"line {number}"

        match = ARENA_RE.match(line)
        if match:
            saw_arena = True
            saw_arena_size = int(match.group(1))
            got = (int(match.group(2)), int(match.group(3)), int(match.group(4)))
            if got != (ALIGN, HEADER, MIN_BLOCK):
                problems.append(
                    f"{label}: the arena line says align {got[0]} header "
                    f"{got[1]} min_block {got[2]}, but the contract states "
                    f"{ALIGN}, {HEADER}, and {MIN_BLOCK}")
            if match.group(5) != policy:
                problems.append(f"{label}: the arena line says policy "
                                f"{match.group(5)}, but the command line asked "
                                f"for {policy}")
            if arena is not None and saw_arena_size != arena:
                problems.append(f"{label}: the arena line says "
                                f"{saw_arena_size} bytes, but the script states "
                                f"{arena}")
            continue

        match = SEG_RE.match(line)
        if match:
            name = match.group(1)
            segment_order.append(name)
            row = machine["segments"].get(name)
            if row is None:
                problems.append(f"{label}: the script holds no {name} row")
                continue
            printed = (int(match.group(2)), int(match.group(3)), match.group(4),
                       match.group(5))
            stated = (row["base"], row["bound"], row["grow"], row["prot"])
            if printed != stated:
                problems.append(
                    f"{label}: the {name} row printed as base {printed[0]} bound "
                    f"{printed[1]} grow {printed[2]} prot {printed[3]}, but the "
                    f"script states {stated[0]}, {stated[1]}, {stated[2]}, "
                    f"{stated[3]}")
            continue

        match = ALLOC_RE.match(line)
        if match:
            handle = match.group(1)
            size = int(match.group(2))
            addr = int(match.group(3))
            block = int(match.group(4))
            needed = HEADER + round_up(size, ALIGN)
            if handle in live:
                problems.append(f"{label}: '{handle}' is allocated while it is "
                                "still live")
                continue
            if addr % ALIGN != 0:
                problems.append(f"{label}: '{handle}' has address {addr}, which "
                                f"is not a multiple of {ALIGN}")
            if block < needed:
                problems.append(f"{label}: '{handle}' asks for {size} bytes, "
                                f"which needs {needed}, but its block is {block}")
            elif block != needed and block - needed >= MIN_BLOCK:
                problems.append(
                    f"{label}: '{handle}' has a block of {block} for a request "
                    f"that needs {needed}, and the {block - needed} bytes left "
                    f"over would hold a block of {MIN_BLOCK} or more, so it had "
                    "to split")
            if arena is not None and size > arena:
                problems.append(f"{label}: '{handle}' asks for {size} bytes from "
                                f"an arena of {arena}")
            start = addr - HEADER
            if start < 0:
                problems.append(f"{label}: '{handle}' has address {addr}, which "
                                "is inside no block's payload")
                continue
            if shown_free is not None:
                choice = choose_from(shown_free, needed, policy, cursor)
                if choice is None:
                    problems.append(
                        f"{label}: the free list here holds nothing that holds "
                        f"{needed} bytes, so this allocation had to fail")
                elif choice != start:
                    problems.append(
                        f"{label}: {policy} fit picks the block at {choice} from "
                        f"the free list shown here, but the run took the block "
                        f"at {start}")
            shown_free = None
            cursor = start
            live[handle] = (start, block, size)
            continue

        match = FREE_RE.match(line)
        if match:
            handle = match.group(1)
            if handle not in live:
                problems.append(f"{label}: 'free {handle}' releases a handle that "
                                "is not live")
                continue
            start, block, _ = live.pop(handle)
            if int(match.group(2)) != start + HEADER or int(match.group(3)) != block:
                problems.append(f"{label}: 'free {handle}' says addr "
                                f"{match.group(2)} block {match.group(3)}, but "
                                f"the live block is addr {start + HEADER} block "
                                f"{block}")
            shown_free = None
            continue

        match = FREEAT_RE.match(line)
        if match:
            address = int(match.group(1))
            block = int(match.group(2))
            owner = None
            for handle, (start, _, _) in live.items():
                if start + HEADER == address:
                    owner = handle
                    break
            if owner is None:
                problems.append(f"{label}: 'freeat {address}' releases an address "
                                "no live block was handed out at")
                continue
            start, span, _ = live.pop(owner)
            if block != span:
                problems.append(f"{label}: 'freeat {address}' says block {block}, "
                                f"but the live block is {span}")
            shown_free = None
            continue

        match = TOUCH_RE.match(line)
        if match:
            handle = match.group(1)
            if handle not in live:
                problems.append(f"{label}: 'touch {handle}' reads a handle that "
                                "is not live")
                continue
            start, _, _ = live[handle]
            if int(match.group(2)) != start + HEADER:
                problems.append(f"{label}: 'touch {handle}' says addr "
                                f"{match.group(2)}, but the live block is at addr "
                                f"{start + HEADER}")
            continue

        dumped = parse_dump(line)
        if dumped is not None:
            check_dump(dumped, live, arena, f"{label}: the free list", problems)
            shown_free = list(dumped)
            continue

        match = TRANSLATE_RE.match(line)
        if match:
            name = match.group(1)
            offset = int(match.group(2))
            operation = match.group(3)
            row = machine["segments"].get(name)
            if row is None or row["base"] is None or row["bound"] is None:
                problems.append(f"{label}: the script holds no usable {name} row "
                                "to translate against")
                continue
            wanted = None
            if offset >= row["bound"]:
                wanted = "bounds"
            elif row["prot"][OPERATIONS.index(operation)] == "-":
                wanted = "protect"
            if match.group(5) is not None:
                if match.group(5) != wanted:
                    problems.append(
                        f"{label}: the {name} row raises {match.group(5)} for "
                        f"offset {offset} and {operation}, but it must raise "
                        f"{wanted}")
            elif wanted is not None:
                problems.append(f"{label}: the {name} row translated offset "
                                f"{offset} to {match.group(4)}, but it must raise "
                                f"{wanted}")
            else:
                want = (row["base"] + offset if row["grow"] == "up"
                        else row["base"] + row["bound"] - 1 - offset)
                if int(match.group(4)) != want:
                    problems.append(f"{label}: the {name} row translated offset "
                                    f"{offset} to {match.group(4)}, but the row "
                                    f"gives {want}")
            continue

        match = SUMMARY_RE.match(line)
        if match:
            summary = match
            continue

        match = RESULT_RE.match(line)
        if match:
            result = match.group(1)
            continue

        problems.append(f"{label}: '{line}' is not a line the contract defines")

    if not saw_arena:
        problems.append("the report holds no arena line")
        return
    if summary is None:
        problems.append("the report holds no summary line")
        return
    if result is None:
        problems.append("the report holds no result line")
        return

    if result == "complete" and exit_code != 0:
        problems.append(f"the result line says complete, but the exit code is "
                        f"{exit_code}")
    if result == "faulted" and exit_code != 70:
        problems.append(f"the result line says faulted, but the exit code is "
                        f"{exit_code}")
    if segment_order and segment_order != list(SEGMENTS):
        problems.append("the report lists the segments as "
                        f"{', '.join(segment_order)}, but the table is code, "
                        "heap, and stack")

    if arena is None:
        problems.append("the script states no arena, so the runner cannot check "
                        "the totals")
        return

    gaps, overlap = gaps_of(live, arena)
    if overlap is not None:
        problems.append(f"two live blocks overlap at {overlap[1]}")
        return

    allocated = sum(block for (_, block, _) in live.values())
    free_bytes = sum(size for (_, size) in gaps)
    largest = max((size for (_, size) in gaps), default=0)

    if int(summary.group(1)) != allocated:
        problems.append(f"the summary says {summary.group(1)} bytes allocated, "
                        f"but the live blocks hold {allocated}")
    if int(summary.group(2)) != free_bytes:
        problems.append(f"the summary says {summary.group(2)} bytes free, but "
                        f"the live blocks leave {free_bytes}")
    if int(summary.group(3)) != largest:
        problems.append(f"the summary says the largest free block is "
                        f"{summary.group(3)}, but the live blocks leave {largest}")
    if int(summary.group(4)) != len(gaps):
        problems.append(f"the summary says {summary.group(4)} free blocks, but "
                        f"the live blocks leave {len(gaps)}")
    if allocated + free_bytes != arena:
        problems.append(f"the live blocks hold {allocated} bytes and the free "
                        f"list {free_bytes}, which is {allocated + free_bytes} "
                        f"and not the arena's {arena}")

    internal = sum(block - (HEADER + round_up(size, ALIGN))
                   for (_, block, size) in live.values())
    if int(summary.group(5)) != internal:
        problems.append(f"the summary says {summary.group(5)} bytes of internal "
                        f"waste, but the live blocks waste {internal}")

    expected_external = 0.0 if free_bytes == 0 else \
        (free_bytes - largest) / free_bytes
    if abs(float(summary.group(6)) - expected_external) > 0.000001:
        problems.append(f"the summary says the external ratio is "
                        f"{summary.group(6)}, but free {free_bytes} and largest "
                        f"{largest} give {expected_external:.6f}")


# --------------------------------------------------------------------------
# The case loop
# --------------------------------------------------------------------------


def run_case(script_path, repo_root):
    """Runs one case and returns a list of problems, empty when it passes."""
    problems = []
    name = script_path.relative_to(repo_root)

    spec = parse_expect(script_path.with_suffix(".expect"))
    args = parse_args_file(script_path.with_suffix(".args"))
    machine = parse_script(script_path)
    policy = policy_of(args)

    try:
        target = script_path.relative_to(repo_root).as_posix()
    except ValueError:
        target = str(script_path)
    if spec["path"] is not None:
        target = spec["path"]

    if spec["argv"] is not None:
        cmd = ["./run"] + spec["argv"]
    else:
        cmd = ["./run"] + args + [target]
    stdout, stderr, code = run_program(cmd, repo_root)

    if code != spec["exit"]:
        problems.append(f"exit code: expected {spec['exit']}, got {code}")
    for wanted in spec["stderr"]:
        if wanted not in stderr:
            problems.append(f"stderr: expected to find '{wanted}', got "
                            f"'{stderr.strip()}'")

    lines = [line for line in stdout.splitlines() if line.strip()]
    compare_output(lines, spec["stdout"], problems)

    # The derivation runs on the run's own output, whatever the case says, so
    # a case file and a wrong program cannot agree their way to a pass.
    if any(ARENA_RE.match(line) or SUMMARY_RE.match(line) for line in lines):
        derive(lines, machine, policy, code, problems)

    return name, problems


def main():
    parser = argparse.ArgumentParser(description="CMSC 125 Problem Set 2 case "
                                                 "runner.")
    parser.add_argument("case_folder", help="the folder of .script cases, e.g. "
                                            "cases")
    parser.add_argument("--repo-root", default=".",
                        help="the repository root. Defaults to the current "
                             "directory.")
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    case_folder = Path(args.case_folder).resolve()

    if not case_folder.exists():
        print(f"ERROR: case folder '{case_folder}' does not exist.",
              file=sys.stderr)
        return 1

    entry = repo_root / "run"
    if not entry.exists():
        print("ERROR: './run' is missing. Is this the starter's root directory?",
              file=sys.stderr)
        return 1
    if not os.access(entry, os.X_OK):
        print("ERROR: './run' is not executable. Run 'chmod +x run check.sh'.",
              file=sys.stderr)
        return 1

    script_files = sorted(case_folder.rglob("*.script"))
    if not script_files:
        print(f"ERROR: no .script files under '{case_folder}'.", file=sys.stderr)
        return 1

    failures = []
    for script_path in script_files:
        try:
            name, problems = run_case(script_path, repo_root)
        except CaseError as error:
            name, problems = script_path.relative_to(repo_root), [str(error)]
        if problems:
            failures.append(name)
            print(f"[FAIL] {name}")
            for line in "\n".join(problems).splitlines():
                print(f"       {line}")
        else:
            print(f"[PASS] {name}")

    total = len(script_files)
    print(f"\n{total - len(failures)}/{total} cases passed.")
    return 0 if not failures else 1


if __name__ == "__main__":
    sys.exit(main())
