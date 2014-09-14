CC := gcc
CFLAGS := -Wall -Werror -Iinclude
LDFLAGS :=


OBJECTS := shell.o path_utils.o

all: w4118_sh


w4118_sh: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS)

%.o: src/%.c
	$(CC) $(CFLAGS) -c $^

check:
	scripts/checkpatch.pl --no-tree -f src/*

clean:
	rm -f w4118_sh
	rm -f shell.o

.PHONY: clean
