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
		printf("error: %s", strerror(errno));
		return NOT_OK;
	}
	strcpy(copy, line);
	strtok(copy, delim);
	ntok = 1;
	/*TO DO: check what happens for 1? */
	while (strtok(NULL, delim) != NULL)
		ntok++;
	free(copy);

	*tokens = calloc(ntok + 1, sizeof(char *));
	if (*tokens == NULL) {
		printf("error: %s", strerror(errno));
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
	printf("error: %s", strerror(errno));
	return NOT_OK;
}

int main(int argc, char **argv)
{
	int status;
	int len = 0;
	pid_t	pid;
	char *line;
	char **tokens;
	size_t n = 12345;

	while (1) {
		printf("$");
		line = NULL;
		len = getline(&line, &n, stdin);
		if (len < 0) {
			printf("error: %s", strerror(errno));
			goto error;
		}
		/* Do nothing for CR */
		if (len == 1)
			continue;
		if (tokenize(line, &tokens) < 0)
			goto error;

		/* Terminate */
		if (strcmp(tokens[0], "exit") == 0) {
			free(line);
			free(tokens);
			break;
		}
		/* Do work */
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
