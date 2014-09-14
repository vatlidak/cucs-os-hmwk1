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
 * @tokenize - Parse line into tokens
 *
 * @line - The line to be parsed
 * @tokens - The tokens retrieved line
 *
 * An array, whose starting address is pointed by "*tokens",
 * is allocated to store any tokens found in line. This array
 * should by freed by the calling method.
 */
int tokenize(char *line, char ***tokens)
{
	int i;
	int ntok;
	const char *delim = "\n\t ";
	char *copy;

	/*
	 * strtok libc function modifies the initial buffer it is
	 * requested to tokenize. Thus, we make a copy of the
	 * initial buffer and use it to count number of tokens.
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
	*tokens = calloc(ntok + 1, sizeof(char *));
	if (*tokens == NULL) {
		fprintf(stderr, "error: %s", strerror(errno));
		return NOT_OK;
	}
	**tokens = strtok(line, delim);
	for (i = 1; i < ntok; i++)
		*(*tokens + i) = strtok(NULL, delim);
	*(*tokens + i) = NULL;
	/* printf("ntok:%d\n", ntok); */
	return OK;
}

/*
 * @execute - Execute a command
 *
 * @args - argv[0] is the command's name; arguments follow subsequently.
 */
int execute(char **args)
{
	execvp(args[0], args);
	fprintf(stderr, "error: %s", strerror(errno));
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
	int status;
	int len = 0;
	pid_t	pid;
	char *line;
	char **tokens;
	size_t n = 12345;

	PATH = NULL;
	while (1) {
		printf("$");
		line = NULL;
		len = getline(&line, &n, stdin);
		if (len < 0) {
			fprintf(stderr, "error: %s", strerror(errno));
			goto error;
		}
		if (len == 1)
			continue;
		if (tokenize(line, &tokens) < 0)
			goto error;
		if (!strcmp(tokens[0], "exit")) {
			free(line);
			free(tokens);
			break;
		}
		if (!strcmp(tokens[0], "cd")) {
			cd(tokens);
			free(line);
			free(tokens);
			continue;
		}
		if (!strcmp(tokens[0], "path")) {
			path(tokens);
			free(line);
			free(tokens);
			continue;
		}
		/* bin is not an absolute path; check PATH variable */
		if (isalpha(tokens[0][0]))
			if (in_path(PATH, tokens[0]) < 0) {
				fprintf(stderr, "error: executabe not in PATH\n");
				continue;
			}
		pid = fork();
		if (pid > 0) {
			/* TO DO error  here */
			pid = waitpid(pid, &status, 0);
			free(line);
			free(tokens);
		} else if (pid == 0) {
			if (execute(tokens) < 0)
				goto error;
		} else
			goto error;
	}
	return 0;
error:
	printf("\n");
	return -1;
}
