#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>


int getargs(char *line, char ***args)
{
	const char *delim = " \n";
	char *copy;
	int nargs;
	int i;

	char *copy = calloc(strlen(line), sizeof(char));
	if (copy == NULL) {
		perror("calloc");
		return -1;
	}
	strcpy(copy, line);

	/* count args */
	strtok(copy, delim);
	nargs = 1;
	while (strtok(NULL, delim) != NULL)
		nargs++;
	free(copy);

	*args = calloc(nargs + 1, sizeof(char *));
	**args = strtok(line, delim);
	for (i = 1; i < nargs; i++)
		*(*args+i) = strtok(NULL, delim);
	*(*args+i) = NULL;
	return 0;
}

int main(int argc, char **argv)
{
	char **args;
	int status;
	int len;
	pid_t	pid;

	char *line = NULL;
	size_t n = 12345;
	
	do {
		printf("$");
		len = getline(&line, &n, stdin);
		if (len < 0)
			break;
		if (len == 0)
			continue;
		if (line[0]==' ' && line[1]=='\0')
			continue;
		if (strcmp(line, "exit\n") == 0 )
			break;
		getargs(line, &args);
		if ((pid = fork()) > 0) {
			pid = wait(&status);
			free(args);
		} else if (pid == 0) {
			execvp(args[0], args);
		   	perror("exec failure");
		   	return -1;
		} else {
			perror("fork failure");
			break;
		}
	} while (1);
	return 0;
}
