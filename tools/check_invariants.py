#!/usr/bin/env python3
"""
check_invariants.py: checks the invariants outside the published case corpus.

These checks are part of the check. `check.sh` runs them after the public
corpus.

These checks cover claims the case corpus cannot state by itself. The
case corpus holds small cases and a fixed set of them:

1. Two runs of one script agree byte for byte, and no line prints a host
   address. An offset into the arena or a physical address from the segment
   table is all the output may hold.
2. A long stress trace keeps the arena tiled under every policy. The check
   replays the run from its own output with the published runner, so the
   corpus and the check agree about the contract.
3. The translation lines depend on the segment table and nothing else. The
   same translation requests under three policies give the same three lines.
4. Filling the arena with the smallest allocations and releasing them all
   returns the free list to one block of the whole arena. A byte is never lost
   and never invented.

Usage: python3 check_invariants.py <work directory with a build>
Exit code: 0 when every check passes, 1 otherwise.
"""

import importlib.util
import random
import subprocess
import sys
import tempfile
from pathlib import Path

TIMEOUT = 60
MAX_PHYS = 1048576

SEG = ["seg code 4096 1024 up r-x",
       "seg heap 8192 2048 up rw-",
       "seg stack 12288 512 down rw-"]

MACHINE = {"arena": 4096,
           "segments": {"code": {"base": 4096, "bound": 1024, "grow": "up",
                                 "prot": "r-x"},
                        "heap": {"base": 8192, "bound": 2048, "grow": "up",
                                 "prot": "rw-"},
                        "stack": {"base": 12288, "bound": 512, "grow": "down",
                                  "prot": "rw-"}}}


def load_checker(work):
    """Loads the published case runner from the work copy."""
    path = work / "tools" / "check_cases.py"
    spec = importlib.util.spec_from_file_location("check_cases", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def run_script(work, policy, name):
    result = subprocess.run(["./run", "--policy", policy, name], cwd=work,
                            capture_output=True, text=True, timeout=TIMEOUT)
    return result.stdout, result.stderr, result.returncode


def lines_of(text):
    return [line for line in text.splitlines() if line.strip()]


def check_two_runs_agree(work, scratch):
    """Two runs agree byte for byte, and no line holds a host address."""
    script = ["arena 4096"] + SEG + [
        "alloc A 40", "alloc B 80", "alloc C 24", "free B",
        "alloc D 16", "dump", "alloc E 300", "free A", "dump",
        "translate code 900 read", "translate stack 0 write", "touch D"]
    path = scratch / "twice.script"
    path.write_text("\n".join(script) + "\n", encoding="utf-8")

    problems = []
    for policy in ("first", "best", "next"):
        first, stderr, code = run_script(work, policy, path)
        if code != 0:
            problems.append(f"{policy}: the run failed with exit {code}: "
                            f"{stderr.strip()}")
            continue
        second, _, _ = run_script(work, policy, path)
        if first != second:
            problems.append(f"{policy}: two runs of one script disagree")
        for line in lines_of(first):
            fields = line.split()
            if fields and fields[0] in ("alloc", "free", "freeat", "touch"):
                address = int(fields[fields.index("addr") + 1]) \
                    if "addr" in fields else None
                if address is not None and address > MACHINE["arena"]:
                    problems.append(f"{policy}: '{line}' holds an address past "
                                    "the arena")
            if fields and fields[0] == "translate" and "addr" in fields:
                address = int(fields[fields.index("addr") + 1])
                if address > MAX_PHYS:
                    problems.append(f"{policy}: '{line}' holds an address past "
                                    "physical memory")
    return "; ".join(problems)


def make_stress():
    """Builds a long deterministic trace with many splits and merges."""
    generator = random.Random(1252027)
    lines = ["arena 4096"] + SEG
    live = []
    counter = 0
    for step in range(300):
        if live and (len(live) >= 16 or generator.random() < 0.45):
            lines.append(f"free {live.pop(generator.randrange(len(live)))}")
        else:
            counter += 1
            name = f"H{counter}"
            lines.append(f"alloc {name} {generator.randrange(8, 65)}")
            live.append(name)
        if step % 25 == 24:
            lines.append("dump")
    lines.append("dump")
    return lines


def check_stress_tilings(work, scratch):
    """A long trace keeps the arena tiled, under every policy."""
    checker = load_checker(work)
    stress = make_stress()
    path = scratch / "stress.script"
    path.write_text("\n".join(stress) + "\n", encoding="utf-8")
    problems = []
    for policy in ("first", "best", "next"):
        stdout, stderr, code = run_script(work, policy, path)
        if code not in (0, 70):
            problems.append(f"{policy}: the run failed with exit {code}: "
                            f"{stderr.strip()}")
            continue
        found = []
        checker.derive(lines_of(stdout), MACHINE, policy, code, found)
        if found:
            problems.append(f"{policy}: " + "; ".join(found[:4]))
    return "; ".join(problems)


def check_translation_independent(work, scratch):
    """The translation lines depend on the segment table and nothing else."""
    requests = ["translate code 0 read", "translate heap 2047 write",
                "translate stack 0 write", "translate code 1024 read",
                "translate code 12 write"]
    path = scratch / "trans.script"
    path.write_text(
        "\n".join(["arena 512"] + SEG + ["alloc A 32"] + requests) + "\n",
        encoding="utf-8")
    outputs = []
    for policy in ("first", "best", "next"):
        stdout, stderr, code = run_script(work, policy, path)
        if code != 0:
            return f"{policy}: the run failed with exit {code}: {stderr.strip()}"
        outputs.append([line for line in lines_of(stdout)
                        if line.startswith("translate ")])
    if not outputs[0]:
        return "the run printed no translation lines"
    if outputs[0] != outputs[1] or outputs[0] != outputs[2]:
        return ("the translation lines changed with the placement policy: "
                f"{outputs[0]} then {outputs[1]}")
    return ""


def check_full_cycle(work, scratch):
    """Filling the arena and releasing it all returns one block of it."""
    names = [f"B{index}" for index in range(1, 11)]
    script = ["arena 256"] + SEG
    script.extend(f"alloc {name} 1" for name in names)
    script.extend(f"free {name}" for name in reversed(names))
    script.append("dump")
    path = scratch / "cycle.script"
    path.write_text("\n".join(script) + "\n", encoding="utf-8")

    stdout, stderr, code = run_script(work, "first", path)
    if code != 0:
        return f"the run failed with exit {code}: {stderr.strip()}"
    problems = []
    lines = lines_of(stdout)
    if "dump 0 256" not in lines:
        problems.append("the free list after the cycle is not one block of the "
                        "whole arena")
    want = ("summary allocated 0 free 256 largest 256 blocks 1 internal 0 "
            "external 0.000000")
    if want not in lines:
        found = [line for line in lines if line.startswith("summary ")]
        problems.append(f"the summary is '{found[0] if found else 'missing'}', "
                        f"expected '{want}'")
    checker = load_checker(work)
    checker.derive(lines, {"arena": 256, "segments": MACHINE["segments"]},
                   "first", code, problems)
    return "; ".join(problems)


CHECKS = [
    ("two runs agree byte for byte, with no host address",
     check_two_runs_agree),
    ("a long stress trace keeps the arena tiled",
     check_stress_tilings),
    ("the translation lines do not depend on the policy",
     check_translation_independent),
    ("a full cycle returns the arena to one free block",
     check_full_cycle),
]


def main():
    if len(sys.argv) != 2:
        print("usage: python3 check_invariants.py <work directory>", file=sys.stderr)
        return 1
    work = Path(sys.argv[1]).resolve()
    if not (work / "run").exists():
        print(f"'{work}' holds no run entry point.", file=sys.stderr)
        return 1

    failures = 0
    with tempfile.TemporaryDirectory(prefix="ps2-invariants-") as scratch:
        scratch = Path(scratch)
        for title, check in CHECKS:
            try:
                problem = check(work, scratch)
            except (OSError, subprocess.SubprocessError) as error:
                problem = f"the check could not run: {error}"
            if problem:
                failures += 1
                print(f"[FAIL] {title}\n       {problem}")
            else:
                print(f"[PASS] {title}")
    total = len(CHECKS)
    print(f"\n{total - failures}/{total} invariant checks passed.")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
