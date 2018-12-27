#include "commands.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/limits.h>

extern struct _IO_FILE* ERROR_OUTPUT;

int cd(int argc, char** argv)
{
	if (argc < 2)
	{
		fprintf(ERROR_OUTPUT, "Too few arguments for command call.\n");
		exit(1);
	}

	int ret = chdir(argv[1]);
	if (ret == -1)
	{
		char* error = strerror(errno);
		if(error)
			fprintf(ERROR_OUTPUT, "%s\n", error);
	}

	return ret != 0;
}

int pwd(int argc, char** argv)
{
	char cwd[PATH_MAX];
	const char* path = getcwd(cwd, sizeof(cwd));
	if (path)
	{
		printf("%s\n", path);
	}
	else
	{
		char* error = strerror(errno);
		if (error)
			fprintf(ERROR_OUTPUT, "%s\n", error);
	}

	return path != NULL;
}

int printHistory(const struct StringArray* history)
{
	if (!history)
		return 1;

	for (int i = 0; i < history->size; ++i)
		printf("#%d: %s\n", i + 1, history->data[i]);

	return 0;
}