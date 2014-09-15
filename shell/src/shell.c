/*
 * shell.c
 * Copyright (C) 2014 Vaggelis Atlidakis <vatlidak@columbia.cs.edu>
 *
 * A simple bash-like shell
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

struct pnode *PATH;

/*
 * @r_wait - Custom wait function that restarts when suspended by signal
 *
 * @stat_loc - Similar to status
 *
 * Source code from: UNIX SYSTEMS Programming, K. A. Robbins, S. Robbins
 */
pid_t r_wait(int *stat_loc)
{
   pid_t retval;

   do {
	retval = wait(stat_loc);
   } while (retval == -1 && errno == EINTR);
   return retval;
}

/*
 * @tokenize - Parse line into args separated by delimiter(s)
 *
 * @line - The line to be parsed
 * @args - The args retrieved by the line
 * @delim - A set of delimiters
 *
 * An array, whose starting address is pointed by "*args",
 * is allocated to store any args found in line. This array
 * should by freed by the calling method.
 */
int tokenize(char *line, char ***args, char *delim)
{
	int i;
	int ntok;
	char *copy;

	/*
	 * strtok libc function modifies the initial buffer it is
	 * requested to tokenize. Thus, we make a copy of the
	 * initial buffer and use it to count number of args
	 * and in the next iteration then retrieve the args.
	 */
	copy = calloc(strlen(line), sizeof(char));
	if (copy == NULL) {
		fprintf(stderr, "error: %s", strerror(errno));
		return NOT_OK;
	}
	strcpy(copy, line);
	strtok(copy, delim);
	ntok = 1;
	while (strtok(NULL, delim) != NULL)
		ntok++;
	free(copy);

	copy = calloc(strlen(line), sizeof(char));
	if (copy == NULL) {
		fprintf(stderr, "error: %s", strerror(errno));
		return NOT_OK;
	}
	strcpy(copy, line);
	*args = calloc(ntok + 1, sizeof(char *));
	if (*args == NULL) {
		fprintf(stderr, "error: %s", strerror(errno));
		return NOT_OK;
	}
	**args = strtok(copy, delim);
	for (i = 1; i < ntok; i++)
		*(*args + i) = strtok(NULL, delim);
	*(*args + i) = NULL;
	/* printf("ntok:%d\n", ntok); */
	return OK;
}

/*
 * @execute - Execute command(s) as specified in line
 *
 * @line - A pointer to a string specifying command(s) to be executed
 */
int execute(char *line)
{
	char **args;// = NULL;
	pid_t pid;
	int status;

	pid = fork();
	if (pid > 0) {
		pid = waitpid(pid, &status, 0);
		return OK;
	} else if (pid == 0) {
		if (tokenize(line, &args, "\n\t \"") < 0)
			goto error;
		execvp(args[0], args);
	} else {
		goto error;
	}
error:
	fprintf(stderr, "error: %s", strerror(errno));
	fprintf(stderr, "execute\n");
	return NOT_OK;
}

/*
 * @cd - Implement built-in "cd"
 *
 * @args - argv[0] is the command's name; arguments follow subsequently.
 */
void cd(char **args)
{
	char *path;

	/*
	 * When a user tries to change directory  without specifying
	 * any, by default GNU/Linux uses user's home directory.
	 */
	if (args[1] == '\0') {
		path = calloc(strlen(getlogin()) + strlen("/home/") + 1,
			      sizeof(char));
		if (path == NULL) {
			fprintf(stderr, "error: %s", strerror(errno));
			printf("\n");
		}
		sprintf(path, "/home/%s", getlogin());
	} else {
		path = args[1];
	}
	if (chdir(path) < 0) {
		fprintf(stderr, "error: %s", strerror(errno));
		printf("\n");
	}
}

/*
 * @path - Implement built-in "path"
 *
 * @args - argv[0] is the command's name; arguments follow subsequently.
 */
void path(char **args)
{
	if (args[1] == NULL) {
		print(PATH);
	} else if (args[1][0] == '+' && args[3] == NULL) {
		if (insert_at_start(&PATH, args[2]) < 0)
			fprintf(stderr, "error: cannot add path\n");
	} else if (args[1][0] == '-' && args[3] == NULL) {
		if (delete(&PATH, args[2]) < 0)
			fprintf(stderr, "error: cannot remove path\n");
	} else {
		fprintf(stderr, "error: syntrax error in path command\n");
	}
}

int main(int argc, char **argv)
{
	int len = 0;
	char *line;
	char **args;
	size_t n = 12345;

	PATH = NULL;
	while (1) {
		printf("$");
		line = NULL;
		len = getline(&line, &n, stdin);
		if (len < 0)
			fprintf(stderr, "error: %s", strerror(errno));
		if (len == 1)
			continue;
		if (tokenize(line, &args, "\n\t ") < 0)
			continue;//goto error;
		if (!strcmp(args[0], "exit")) {
			free(line);
			free(args);
			break;
		}
		if (!strcmp(args[0], "cd")) {
			cd(args);
			free(line);
			free(args);
			continue;
		}
		if (!strcmp(args[0], "path")) {
			path(args);
			free(line);
			free(args);
			continue;
		}
		/*if we get here it is not a build in cmd */
		/* bin is not an absolute path; check PATH variable */
		if (in_path(PATH, args[0]) < 0) {
			free(args);
			continue;
		}
		free(args);
		execute(line);
		free(line);
	}
	return 0;
}
