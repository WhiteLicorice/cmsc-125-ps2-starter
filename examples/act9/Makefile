# CMSC 125 Activity 9 examples.
#
#   make          build the simulator
#   make check    build the simulator, run every case, and compare its output
#   make clean    delete the simulator
#
# check compares each case's output against expected/<case>.txt. The simulator
# is deterministic, so every comparison is exact. Each run prints the TLB trace
# and the summary for one reference string.

CC      := cc
CFLAGS  := -std=c11 -Wall -Wextra -Werror
CASES   := walk walk-narrow stride switch-flush switch-asid eat

all: tlb

tlb: tlb.c
	$(CC) $(CFLAGS) -o $@ $<

check: tlb
	@pass=0; fail=0; \
	for case in $(CASES); do \
	  ./tlb --case $$case > tlb.tmp.out; \
	  if [ "$$(cat expected/$$case.txt)" = "$$(cat tlb.tmp.out)" ]; then \
	    echo "$$case: ok"; \
	    pass=$$((pass + 1)); \
	  else \
	    echo "$$case: FAIL (output differs)"; \
	    diff -u expected/$$case.txt tlb.tmp.out | head -20; \
	    fail=$$((fail + 1)); \
	  fi; \
	done; \
	rm -f tlb.tmp.out; \
	echo "$$pass of $$((pass + fail)) outputs match"; \
	test $$fail -eq 0

clean:
	rm -f tlb

.PHONY: all check clean
