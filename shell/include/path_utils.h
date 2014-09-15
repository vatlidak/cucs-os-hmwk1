/*
 * path_utils.h
 * Copyright (C) 2014 Vaggelis Atlidakis <vatlidak@columbia.cs.edu>
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>

struct pnode {
	char *dir;
	struct pnode *next;
};

int insert_at_start(struct pnode **list, char *dir);
int delete(struct pnode **list, char *dir);
int in_path(struct pnode *list, char *bin);
void print(struct pnode *list);
