#include "commands.h"
#include "job.h"
#include "utils.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <linux/limits.h>

struct _IO_FILE* ERROR_OUTPUT;
struct StringArray* g_history = NULL;
struct IntArray* g_commandQueue = NULL;

int g_exitShell = 0;

#define PARSING_STATE_COMMAND_NAME 0
#define PARSING_STATE_COMMAND_ARGS 1
#define PARSING_STATE_COMMAND_INPUT 2
#define PARSING_STATE_COMMAND_OUTPUT 3

static int setCommandField(struct Command* command, struct String* token, int currentState)
{
	switch (currentState)
	{
	case PARSING_STATE_COMMAND_NAME:
		if (token->size == 0)
		{
			/* TODO: specify location*/
			fprintf(ERROR_OUTPUT, "Syntax error: expected command name.\n");
			return 1;
		}

		command->name = duplicateString(token->data);
		break;

	case PARSING_STATE_COMMAND_ARGS:
		if(token->size > 0)
			addString(command->args, token->data);
		break;

	case PARSING_STATE_COMMAND_INPUT:
		if (token->size == 0)
		{
			/* TODO: specify location*/
			fprintf(ERROR_OUTPUT, "Syntax error: expected input filename.\n");
			return 1;
		}

		free(command->input);
		command->input = duplicateString(token->data);
		break;

	case PARSING_STATE_COMMAND_OUTPUT:
		if (token->size == 0)
		{
			/* TODO: specify location*/
			fprintf(ERROR_OUTPUT, "Syntax error: expected output filename.\n");
			return 1;
		}

		free(command->output);
		command->output = duplicateString(token->data);
		break;
	}

	emptyString(token);

	return 0;
}

static void trimLastNewLine(char* text)
{
	char* prev = NULL;
	while (*text != '\0')
	{
		prev = text;
		++text;
	}

	if (prev && *prev == '\n')
		*prev = '\0';
}

static int isEscapingSlash(int squotes, int dquotes, int escaped, char nextSymbol)
{
	int escaping = 0;
	if (squotes)
	{
		escaping = 0;
	}
	else if (dquotes)
	{
		if (escaped)
		{
			escaping = 0;
		}
		else
		{
			escaping = nextSymbol == '!' || nextSymbol == '"' || nextSymbol == '\\';
		}
	}
	else
	{
		escaping = !escaped;
	}

	return escaping;
}

static struct Jobs* parseProgramm(const char* text)
{
	struct Jobs* jobs = createJobs();
	struct Job* job = createJob();
	struct Command* command = createCommand();
	struct String* token = createString();

	const char* currSymbol = text;
	int escaped = 0;
	int squotes = 0;
	int dquotes = 0;
	int state = PARSING_STATE_COMMAND_NAME;
	int parsingError = 0;
	int finished = 0;

	while (!finished && !parsingError)
	{
		switch (*currSymbol)
		{
		case '\\':
		{
			char nextSymbol = *(currSymbol + 1);
			escaped = isEscapingSlash(squotes, dquotes, escaped, nextSymbol);

			if (!escaped || nextSymbol == '!')
				addSymbol(token, *currSymbol);

			++currSymbol;
		}	continue;

		case '\'':
			if (dquotes)
				addSymbol(token, *currSymbol);
			else
				squotes = !squotes;

			++currSymbol;
			break;

		case '"':
			if (squotes || escaped)
				addSymbol(token, *currSymbol);
			else
				dquotes = !dquotes;

			++currSymbol;
			break;

		case '#':
			if (!squotes && !dquotes && !escaped && (currSymbol == text || *(currSymbol - 1) == ' '))
			{
				/* skip comment*/
				while (*currSymbol != '\n' && *currSymbol != '\0')
					++currSymbol;
			}
			else
			{
				addSymbol(token, *currSymbol);
				++currSymbol;
			}

			break;

		case ' ':
			if (squotes || dquotes)
			{
				addSymbol(token, *currSymbol);
			}
			else
			{
				if (token->size > 0)
				{
					parsingError = setCommandField(command, token, state);
					state = PARSING_STATE_COMMAND_ARGS;
				}
			}

			++currSymbol;
			break;

		case '<':
			if (!squotes && !dquotes && !escaped)
			{
				parsingError = setCommandField(command, token, state);
				state = PARSING_STATE_COMMAND_INPUT;
			}
			else
			{
				addSymbol(token, *currSymbol);
			}

			++currSymbol;
			break;

		case '>':
			if (!squotes && !dquotes && !escaped)
			{
				parsingError = setCommandField(command, token, state);
				state = PARSING_STATE_COMMAND_OUTPUT;

				command->rewriteOutput = 1;
				if (*(currSymbol + 1) == '>')
				{
					command->rewriteOutput = 0;
					++currSymbol;
				}
			}
			else
			{
				addSymbol(token, *currSymbol);
			}

			++currSymbol;
			break;

		case '|':
			if (!squotes && !dquotes && !escaped)
			{
				parsingError = setCommandField(command, token, state);
				addCommand(job, command);
				command = createCommand();
				state = PARSING_STATE_COMMAND_NAME;
			}
			else
			{
				addSymbol(token, *currSymbol);
			}

			++currSymbol;
			break;

		case ';':
		case '\n':
			if (!squotes && !dquotes && !escaped)
			{
				if (!(job->size == 0 && command->name == NULL && token->size == 0))
				{
					parsingError = setCommandField(command, token, state);
					addCommand(job, command);
					addJob(jobs, job);
					command = createCommand();
					job = createJob();
					state = PARSING_STATE_COMMAND_NAME;
				}
			}
			else
			{
				/* do not add escaped line endings */
				if (*currSymbol != '\n' || !escaped)
					addSymbol(token, *currSymbol);
			}

			++currSymbol;
			break;

		case '\0':
			if (!squotes && !dquotes)
			{
				if (!(job->size == 0 && command->name == NULL && token->size == 0))
				{
					parsingError = setCommandField(command, token, state);
					addCommand(job, command);
					addJob(jobs, job);
					job = NULL;
					command = NULL;
				}
			}
			else
			{
				fprintf(ERROR_OUTPUT, "Syntax error : unexpected end of file while looking for matching %s.\n", squotes ? "'" : "\"");
				parsingError = 1;
			}

			finished = 1;
			break;

		default:
			addSymbol(token, *currSymbol);
			++currSymbol;
			break;
		}

		escaped = 0;
	}

	freeString(token);
	freeCommand(command);
	freeJob(job);

	if (parsingError)
	{
		freeJobs(jobs);
		jobs = NULL;
	}

	return jobs;
}

int substituteHistoryCommands(char* text, int size, int capacity, struct StringArray* history)
{
	int finished = 0;
	while (!finished)
	{
		finished = 1;

		char* curr = text;
		int escaped = 0;
		int squotes = 0;
		int dquotes = 0;
		while (finished && *curr != '\0')
		{
			switch (*curr)
			{
			case '\\':
				escaped = isEscapingSlash(squotes, dquotes, escaped, *(curr + 1));
				++curr;
				continue;

			case '\'':
				if (!dquotes)
					squotes = !squotes;

				++curr;
				break;

			case '"':
				if (!squotes && !escaped)
					dquotes = !dquotes;

				++curr;
				break;

			case '!':
			{
				if (!squotes && !escaped)
				{
					int nCommand = 0;
					const char* next = curr + 1;
					while (isdigit(*next))
					{
						nCommand = nCommand * 10 + (*next - '0');
						++next;
					}

					if (nCommand < 1 || nCommand > history->size)
					{
						/* TODO: specify location*/
						fprintf(ERROR_OUTPUT, "Cannot find history command with number %d. To see available history commands type \"history\".\n", nCommand);
						return 1;
					}

					const char* histItem = history->data[nCommand - 1];
					int cmdSize = (int)strlen(histItem);
					int needSize = size + cmdSize - (int)(next - curr);
					if (needSize > capacity)
					{
						text = realloc(text, (size_t)needSize);
						if (!text)
						{
							fprintf(ERROR_OUTPUT, "Application ran out of memory.\n");
							exit(1);
						}

						capacity = needSize;
					}

					/* move next symbols so we can fit history command*/
					memmove(curr + cmdSize, next, (size_t)(size - (next - text)));
					size = needSize;

					/* paste history command */
					memcpy(curr, histItem, (size_t)cmdSize);

					finished = 0;
				}

				++curr;
			}	break;
				
			default:
				++curr;
				break;
			}

			escaped = 0;
		}
	}

	return 0;
}

static char** createArgsForExec(const struct Command* command)
{
	if (!command)
		return NULL;

	int nArgs = command->args->size + 2;
	char** res = malloc((size_t)nArgs * sizeof(char*));

	res[0] = duplicateString(command->name);
	for (int i = 0; i < command->args->size; ++i)
		res[i + 1] = duplicateString(command->args->data[i]);

	res[nArgs - 1] = NULL;
	return res;
}

static void runJob(const struct Job* job)
{
	struct Command** commands = job->commands;
	int nCommands = job->size;

	emptyIntArray(g_commandQueue);

	int fd[2], pfd[2], prevfd = -1;
	for (int i = 0; !g_exitShell && (i < nCommands); ++i)
	{
		const struct Command* command = commands[i];
		char** args = createArgsForExec(command);

		if (i != nCommands - 1)
			pipe(pfd);
		else
			pfd[0] = pfd[1] = -1;

		fd[0] = i == 0 ? STDIN_FILENO : prevfd;
		fd[1] = i == nCommands - 1 ? STDOUT_FILENO : pfd[1];

		int ok = 1;
		int infd = -1, outfd = -1;
		if (command->input)
		{
			infd = fd[0] = open(command->input, O_RDONLY, 0666);
			if (fd[0] == -1)
			{
				fprintf(ERROR_OUTPUT, "Cannot open specified input file: \"%s\"\n", command->input);
				ok = 0;
			}
		}

		if (ok && command->output)
		{
			if(pfd[0] != -1)
				close(pfd[0]);
			pfd[0] = -1;

			int flags = command->rewriteOutput ? O_CREAT | O_WRONLY | O_TRUNC : O_CREAT | O_WRONLY | O_APPEND;
			outfd = fd[1] = open(command->output, flags, 0666);
			if (fd[1] == -1)
			{
				fprintf(ERROR_OUTPUT, "Cannot open specified output file: \"%s\"\n", command->output);
				ok = 0;
			}
		}

		if (ok)
		{
			int ret = 0;
			int nArgs = command->args->size + 1;
			if (!strcmp(command->name, "cd"))
			{
				/* do not run cd is a separate process */
				ret = cd(nArgs, args);
			}
			else if (!strcmp(command->name, "exit"))
			{
				g_exitShell = 1;
			}
			else
			{
				int execfd[2];
				pipe(execfd);

				pid_t cpid = fork();
				if (!cpid)
				{
					if (fd[0] != STDIN_FILENO)
						close(STDIN_FILENO);

					if (fd[0] != -1)
						dup2(fd[0], STDIN_FILENO);

					if (fd[1] != STDOUT_FILENO)
						close(STDOUT_FILENO);

					if (fd[1] != -1)
						dup2(fd[1], STDOUT_FILENO);

					close(execfd[0]);

					if (!strcmp(command->name, "pwd"))
					{
						close(execfd[1]);
						ret = pwd(nArgs, args);
					}
					else if (!strcmp(command->name, "history"))
					{
						close(execfd[1]);
						ret = printHistory(g_history);
					}
					else
					{
						/* close descriptor in case exec succeeds */
						fcntl(execfd[1], F_SETFD, FD_CLOEXEC);

						execvp(command->name, args);

						/* exec failed, notify parent process */
						int error = errno;
						write(execfd[1], &error, sizeof(error));
						close(execfd[1]);

						ret = 1;
					}

					exit(ret);
				}

				close(execfd[1]);
				int error;
				if (!read(execfd[0], &error, sizeof(error)))
				{
					addInt(g_commandQueue, cpid);
				}
				else
				{
					fprintf(ERROR_OUTPUT, "%s: %s\n", command->name, strerror(error));
				}

				close(execfd[0]);
			}
		}

		if (infd != -1)
			close(infd);

		if (outfd != -1)
			close(outfd);

		if (prevfd != -1)
			close(prevfd);

		if (pfd[1] != -1)
			close(pfd[1]);

		prevfd = pfd[0];

		if (g_exitShell && pfd[0] != -1)
			close(pfd[0]);

		/* free args */
		char** currArg = args;
		while (*currArg)
		{
			free(*currArg);
			++currArg;
		}
		free(args);
	}

	int wstatus;
	pid_t wpid;
	while ((wpid = wait(&wstatus)) != -1)
	{
		/* erase finished process from queue */
		for (int i = 0; i < g_commandQueue->size; ++i)
		{
			if (g_commandQueue->data[i] == wpid)
			{
				g_commandQueue->data[i] = -1;
				break;
			}
		}
	}

	emptyIntArray(g_commandQueue);
}

static void runJobs(const struct Jobs* jobs)
{
	for (int i = 0; !g_exitShell && (i < jobs->size); ++i)
		runJob(jobs->jobs[i]);
}

static void sigIntHanler(int sig)
{
	signal(SIGINT, sigIntHanler);

	if (!g_commandQueue)
		return;

	/* find first non-finished process */
	pid_t cpid = -1;
	for (int i = 0; i < g_commandQueue->size; ++i)
	{
		if (g_commandQueue->data[i] != -1)
		{
			cpid = g_commandQueue->data[i];
			break;
		}
	}

	if (cpid != -1)
		kill(cpid, SIGINT);
}

static void startShell(FILE* infile)
{
	signal(SIGINT, sigIntHanler);

	g_history = createStringArray();
	g_commandQueue = createIntArray();

	char cwd[PATH_MAX];
	while (!g_exitShell && !feof(infile))
	{
		const char* userName = getlogin();
		const char* path = getcwd(cwd, sizeof(cwd));
		printf("%s:%s$ ", userName ? userName : "unknown_user", path ? path : "");

		/* read commands */
		char* buffer = NULL;
		int size, index;
		if (getLine(infile, &buffer, &size, &index) || ferror(infile))
		{
			/* encountered an error during read */
			free(buffer);
			break;
		}

		if (!substituteHistoryCommands(buffer, index + 1, size, g_history))
		{
			/* add to history */
			trimLastNewLine(buffer);
			addString(g_history, buffer);

			/* parse and run */
			struct Jobs* jobs = parseProgramm(buffer);
			if (jobs)
			{
				// printJobs(jobs);
				runJobs(jobs);
			}

			freeJobs(jobs);
		}

		free(buffer);
	}

	freeIntArray(g_commandQueue);
	freeStringArray(g_history);
}

int main(int argc, char** argv)
{
	ERROR_OUTPUT = stdout;

	startShell(stdin);
    return 0;
}