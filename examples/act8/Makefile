# Build and check the act8 examples.
#
# pagesim.c holds the model and the printer. demo_cases.c holds the
# student-facing cases. A translation case prints a page table and a table of
# translated addresses. The size case prints a table of page-table sizes.
# make check compares the output of every case with the recorded output in
# expected/. This folder holds only the cases the students see.

CC       ?= cc
CFLAGS   := -std=c11 -Wall -Wextra -Werror
BIN      := pagesim
SOURCES  := pagesim.c demo_cases.c

CASES := basic offset faults sizes

.PHONY: all check clean

# Build the translator.
all: $(BIN)

$(BIN): $(SOURCES) pagesim.h
	$(CC) $(CFLAGS) -o $@ $(SOURCES)

# Run every case and compare it with its expected output.
# Exit 0 only when every case passes.
check: all
	@fail=0; count=0; \
	for c in $(CASES); do \
		count=$$((count + 1)); \
		if ./$(BIN) $$c | diff -u expected/$$c.txt - >/dev/null; then \
			echo "PASS $$c"; \
		else \
			echo "FAIL $$c"; fail=$$((fail + 1)); \
		fi; \
	done; \
	echo "$$count cases, $$((count - fail)) passed, $$fail failed"; \
	[ $$fail -eq 0 ]

# Remove the built program.
clean:
	rm -f $(BIN)
