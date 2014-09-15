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
	if (line == NULL)
		return NOT_OK;
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
	return OK;
}

/*
 * @pipeline - Execute line which contains pipe commad(s)
 *
 * @line - The line with the piped command(s)
 */
int pipeline(char *line)
{
	pid_t pid;
	int i, j, npipes;
	int **pipes;
	char **args;

	npipes = 0;
	for(i = 0; i < strlen(line); i++)
		if (line[i] == '|')
			++npipes;
	pipes = calloc(npipes, sizeof(int));
	if (pipes == NULL) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		goto error;
	}
	for (i = 0; i < npipes; i++) {
		pipes[i] = calloc(2, sizeof(int));
		if (pipes[i] == NULL) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			goto error;
		}
		if (pipe(pipes[i]) < 0) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			goto error;
		}
	}
#define READ 0
#define WRITE 1
	for (i = 0; i < npipes + 1; i++) {
		pid = fork();
		if (pid == 0)
			break;
		else if (pid > 0) {
			continue;
		}
		else
			goto error;
	}
	if (pid > 0) {
		for(i = 0; i < npipes; i++) {
			close(pipes[i][READ]);
			close(pipes[i][WRITE]);
		}
		pid_t rval;
		while ((rval = r_wait(NULL)) > 0);
		//{
		//	fprintf(stderr, "%ld\n", (long) rval);
		//}
		for (i = 0; i < npipes; i++)
			free(pipes[i]);
		free(pipes);
	}
	if (pid == 0) {
		//fprintf(stderr, "chico: %ld\n", (long)getpid());
		if (i != npipes) {
			if (close(pipes[i][READ]) < 0)
				goto error;
			if (dup2(pipes[i][WRITE], 1) < 0)
				goto error;
		}
		if(i != 0) {
			if (close(pipes[i-1][WRITE]) < 0)
				goto error;
			if (dup2(pipes[i-1][READ], 0) < 0)
				goto error;
		}
		for (j = 0; j < i; j++) {
			close(pipes[j][READ]);
			close(pipes[j][WRITE]);
		}
		char *beg = strtok(line, "|\n");

		for (j = 0; j < i; j++)
			beg = strtok(NULL, "|\n");
		if (tokenize(beg, &args, "\n\t |\"") < 0) {
			fprintf(stderr, "error: cannot parse cmd: <%s>\n", beg);
			goto exit;
		}

		if (in_path(PATH, args[0]) < 0) {
			free(args);
			goto error;
		}
		execvp(args[0], args);
		goto error;
	}
	return 0;
error:
	fprintf(stderr, "error: %s\n", strerror(errno));
exit:
	exit(-1);
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
