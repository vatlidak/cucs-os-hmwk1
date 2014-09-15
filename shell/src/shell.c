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
#include <signal.h>
#include "path_utils.h"
#define OK 0
#define NOT_OK -1

struct pnode *PATH;

/*
 * @r_wait - Custom wait function that restarts when suspended by signal
 *
 * @stat_loc - Similar to status of ordinary wait syscall
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
 * @sigignore - Ignore ctrl-c
 */
void sigignore(void)
{
	struct sigaction act;

	act.sa_handler = SIG_IGN;
	act.sa_flags = 0;
	if ((sigemptyset(&act.sa_mask) == -1) ||
	    (sigaction(SIGINT, &act, NULL) == -1)) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		return;
	}
}

/*
 * @sigreset - Reset to default signal handler
 */
void sigreset(void)
{
	struct sigaction act;

	act.sa_handler = SIG_DFL;
	act.sa_flags = 0;
	if ((sigemptyset(&act.sa_mask) == -1) ||
	    (sigaction(SIGINT, &act, NULL) == -1)) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		return;
	}
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
	copy = calloc(strlen(line) + 1, sizeof(char));
	if (copy == NULL) {
		fprintf(stderr, "error: %s", strerror(errno));
		return NOT_OK;
	}
	strcpy(copy, line);
	if (strtok(copy, delim) == NULL)
		return NOT_OK;
	ntok = 1;
	while (strtok(NULL, delim) != NULL)
		ntok++;
	free(copy);
	copy = calloc(strlen(line) + 1, sizeof(char));
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
 * @pipeline - Execute line which contains pipe symbol(s)
 *
 * @line - The line with the piped command(s)
 */
#define READ 0
#define WRITE 1
int pipeline(char *line)
{
	pid_t pid;
	int i, j, npipes;
	int **pipes;
	char **args;

	npipes = 0;
	for (i = 0; i < strlen(line); i++)
		if (line[i] == '|')
			++npipes;
	pipes = calloc(npipes, sizeof(int *));
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
	for (i = 0; i < npipes + 1; i++) {
		pid = fork();
		if (pid == 0)
			break;
		else if (pid > 0)
			continue;
		else
			goto error;
	}
	if (pid > 0) {
		for (i = 0; i < npipes; i++) {
			if (close(pipes[i][READ]) < 0 ||
			    close(pipes[i][WRITE]) < 0)
				goto error;
		}
		sigignore();
		while (r_wait(NULL) > 0)
			;
		sigreset();
		for (i = 0; i < npipes; i++)
			free(pipes[i]);
		free(pipes);
	}
	if (pid == 0) {
		if (i != npipes)
			if (dup2(pipes[i][WRITE], 1) < 0)
				goto error;
		if (i != 0)
			if (dup2(pipes[i-1][READ], 0) < 0)
				goto error;
		for (j = 0; j < i; j++)
			if (close(pipes[j][READ]) < 0 ||
			    close(pipes[j][WRITE]) < 0)
					goto error;
		char *beg = strtok(line, "|\n");
		/* find which partition of the pipeline you should execute */
		for (j = 0; j < i; j++)
			beg = strtok(NULL, "|\n");
		if (tokenize(beg, &args, "\n\t |\"") < 0) {
			fprintf(stderr, "error: cannot parse cmd:<%s>\n", beg);
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
 * @cd - Built-in "cd"
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
 * @path - Built-in "path"
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
	int len;
	char *line;
	char **args;
	size_t n = 12345;
	pid_t pid;
	int status;

	PATH = NULL;
	while (1) {
shell:
		printf("$");
		line = NULL;
		len = getline(&line, &n, stdin);
		if (len < 0) {
			fprintf(stderr, "error: %s", strerror(errno));
			goto exit;
		}
		if (len == 1)
			goto shell;
		if (strchr(line, '|') != NULL)
			goto pipeline;
		if (tokenize(line, &args, "\n\t \"") < 0)
			goto shell;
		if (!strcmp(args[0], "exit")) {
			free(line);
			free(args);
			goto exit;
		}
		if (!strcmp(args[0], "cd")) {
			cd(args);
			free(line);
			free(args);
			goto shell;
		}
		if (!strcmp(args[0], "path")) {
			path(args);
			free(line);
			free(args);
			goto shell;
		}
		if (in_path(PATH, args[0]) < 0) {
			free(line);
			free(args);
			goto shell;
		}
		pid = fork();
		if (pid > 0) {
			sigignore();
			pid = waitpid(pid, &status, 0);
			sigreset();
			free(args);
			free(line);
			goto shell;
		} else if (pid == 0) {
			execvp(args[0], args);
			fprintf(stderr, "error: %s\n", strerror(errno));
			goto exit;
		} else {
			fprintf(stderr, "error: %s\n", strerror(errno));
			free(line);
			free(args);
			goto exit;
		}
pipeline:
		pipeline(line);
	}
exit:
	return 0;
}
