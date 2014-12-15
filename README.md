Simple bash-like shell.
==
Copyright (C) 2014 V. Atlidakis, G. Koloventzos, A. Papancea

COMS-W4118 Columbia University

## Project structure

include/path_utils.h: Header file
Makefile: Slightly modified Makefile to fit my structure
scripts/checkpatch.pl: Format checking script
src/path_utils.c: Helpers for manipulation of path variable
src/shell.c: Implementation of shell (main)
tests/test.py: Test suite

## Workflow

Compile: make
Check structure: make clean
Run tests: cd tests; python ./test.py
Run shell: ./w4118_sh

## Notes
Signal SIGINT handling imlemented: The parent ignores
SIGINT while there are running children. As soon as all
his children terminated SIGINT handling is reseted to
default.

Support for quotes in commands: Commands like grep "PATTERN" [FILE...]
have support for quotes; e.g., grep "l" README will search character l,
not "l".

Pipeline is implemented using fork(2) and pipe(2). In specific each
command is executed by a separate child and STDIN/STDOUT is duplicated
using dup2 to the READ/WRITE end of a pipe respectively. For example, a
line of the form: "a | b" will result in 3 children being forked and one pipe.
In the same spirit, a line of the form "a | b | c" will result in 3 children
being forked and 2 pipes. The pipes are dynamically created in the parent
according to the occurences of the pipe symbol ("|"). Afterwards, the parent
forks and the filedescriptors and inherited in the children.

## test Run
$python ./test.py
25 of 25 tests passed

## checkpatch run
$make check
scripts/checkpatch.pl --no-tree -f src/*
total: 0 errors, 0 warnings, 115 lines checked

src/path_utils.c has no obvious style problems and is ready for submission.
total: 0 errors, 0 warnings, 336 lines checked

src/shell.c has no obvious style problems and is ready for submission.
