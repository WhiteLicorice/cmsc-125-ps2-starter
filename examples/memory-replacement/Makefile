# Memory replacement examples.
#
#   make          build the simulator
#   make check    build the simulator, run every case, and compare its output
#   make clean    delete the simulator
#
# check compares each case's output against expected/<case>.txt. The simulator
# is deterministic, so every comparison is exact. Each case prints the
# replacement traces or the fault path for one workload.

CC      := cc
CFLAGS  := -std=c11 -Wall -Wextra -Werror
CASES   := policies belady faultpath

all: repl

repl: repl.c
	$(CC) $(CFLAGS) -o $@ $<

check: repl
	@pass=0; fail=0; \
	for case in $(CASES); do \
	  ./repl --case $$case > repl.tmp.out; \
	  if [ "$$(cat expected/$$case.txt)" = "$$(cat repl.tmp.out)" ]; then \
	    echo "$$case: ok"; \
	    pass=$$((pass + 1)); \
	  else \
	    echo "$$case: FAIL (output differs)"; \
	    diff -u expected/$$case.txt repl.tmp.out | head -20; \
	    fail=$$((fail + 1)); \
	  fi; \
	done; \
	rm -f repl.tmp.out; \
	echo "$$pass of $$((pass + fail)) outputs match"; \
	test $$fail -eq 0

clean:
	rm -f repl

.PHONY: all check clean
