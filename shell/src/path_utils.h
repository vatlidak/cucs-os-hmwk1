#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>

#define OK 0
#define NOT_OK -1

struct pnode {
	char *dir;
	struct pnode *next;
};

int insert_at_start(struct pnode **list, char *dir);
int delete(struct pnode **list, char *dir);
int in_path(struct pnode *list, char *bin);
void print(struct pnode *list);
