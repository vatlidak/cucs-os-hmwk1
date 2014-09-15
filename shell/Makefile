CC := gcc
CFLAGS := -Wall -Werror -Iinclude
LDFLAGS :=


OBJECTS := shell.o path_utils.o
EXECUTABLE := w4118_sh

all: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(EXECUTABLE) $(OBJECTS)

%.o: src/%.c
	$(CC) $(CFLAGS) -c $^

check:
	scripts/checkpatch.pl --no-tree -f src/*

clean:
	rm -f $(EXECUTABLE)
	rm -f $(OBJECTS)
.PHONY: clean
