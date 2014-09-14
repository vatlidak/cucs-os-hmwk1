/*
 * path_utils.c
 * Copyright (C) 2014 Vaggelis Atlidakis <vatlidak@columbia.cs.edu>
 *
 * Helpers for manipulation of PATH variable
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
#include "path_utils.h"

/*
 * @insert_at_start - Add a directory in PATH variable
 *
 * @list - The list containing folders of PATH variable
 * @dir - The folder to be added in PATH variable
 */
int insert_at_start(struct pnode **list, char *dir)
{
	struct pnode *temp;

	temp = calloc(1, sizeof(struct pnode));
	if (temp == NULL) {
		fprintf(stderr, "error: %s", strerror(errno));
		return NOT_OK;
	}
	temp->dir = calloc(strlen(dir) + 1, sizeof(char));
	if (temp->dir == NULL) {
		fprintf(stderr, "error: %s", strerror(errno));
		return NOT_OK;
	}
	strcpy(temp->dir, dir);
	temp->next = *list;
	*list = temp;
	return OK;
}

/*
 * @delete - Delete a directory from  PATH variable
 *
 * @list - The list containing folders of PATH variable
 * @dir - The folder to be deleted from PATH variable
 */
int delete(struct pnode **list, char *dir)
{
	struct pnode *temp;

	while (*list != NULL) {
		if (strcmp((*list)->dir, dir) == 0) {
			temp = *list;
			*list = (*list)->next;
			free(temp);
			return OK;
		}
		list = &((*list)->next);
	}
	return NOT_OK;
}

/*
 * @in_path - Check if binary is in any folder of PATH variable
 *
 * @list - The list containing folders of PATH variable
 * @bin - The binary name
 */
int in_path(struct pnode *list, char *bin)
{
	struct stat sbuf;
	char *fullpath;

	while (list != NULL) {
		fullpath = calloc(strlen(list->dir) + strlen(bin) + 2,
				  sizeof(char));
		if (fullpath == NULL) {
			fprintf(stderr, "error: %s", strerror(errno));
			return NOT_OK;
		}
		sprintf(fullpath, "%s/%s", list->dir, bin);
		if (stat(fullpath, &sbuf) == 0 && (sbuf.st_mode & S_IXUSR)) {
			free(fullpath);
			return OK;
		}
		free(fullpath);
		list = list->next;
	}
	return NOT_OK;
}

/*
 * @print - Print all folders of PATH variable
 *
 * @list - The list containing folders of PATH variable
 */
void print(struct pnode *list)
{
	while (list != NULL) {
		printf("%s", list->dir);
		list = list->next;
		if (list != NULL)
			printf(":");
		else
			printf("\n");
	}
}
