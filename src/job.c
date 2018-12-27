#include "job.h"
#include "utils.h"

#include <stdlib.h>

extern struct _IO_FILE* ERROR_OUTPUT;

#define MIN_ARRAY_SIZE 8
#define MAX_ARRAY_GROW_SIZE 64
#define ARRAY_GROWTH_FACTOR 1.5

struct Command* createCommand()
{
	struct Command* command = malloc(sizeof(struct Command));
	command->args = createStringArray();
	command->name = command->input = command->output = NULL;
	command->rewriteOutput = 0;
	return command;
}

void freeCommand(struct Command* command)
{
	if (!command)
		return;

	free(command->name);
	free(command->input);
	free(command->output);
	freeStringArray(command->args);
	free(command);
}

void addCommand(struct Job* job, struct Command* command)
{
	if (job->size == job->capacity)
	{
		int newSize = min(max((int)(job->capacity * ARRAY_GROWTH_FACTOR), MIN_ARRAY_SIZE), job->capacity + MAX_ARRAY_GROW_SIZE);
		job->commands = realloc(job->commands, (size_t)newSize * sizeof(struct Command*));
		if (!job->commands)
		{
			fprintf(ERROR_OUTPUT, "Application ran out of memory.\n");
			exit(1);
		}

		job->capacity = newSize;
	}

	job->commands[job->size] = command;
	job->size++;
}

struct Job* createJob()
{
	struct Job* job = malloc(sizeof(struct Job));
	job->commands = NULL;
	job->size = job->capacity = 0;
	return job;
}

void addJob(struct Jobs* jobs, struct Job* job)
{
	if (jobs->size == jobs->capacity)
	{
		int newSize = min(max((int)(jobs->capacity * ARRAY_GROWTH_FACTOR), MIN_ARRAY_SIZE), jobs->capacity + MAX_ARRAY_GROW_SIZE);
		jobs->jobs = realloc(jobs->jobs, (size_t)newSize * sizeof(struct Job*));
		if (!jobs->jobs)
		{
			fprintf(ERROR_OUTPUT, "Application ran out of memory.\n");
			exit(1);
		}

		jobs->capacity = newSize;
	}

	jobs->jobs[jobs->size] = job;
	jobs->size++;
}

void freeJob(struct Job* job)
{
	if (!job)
		return;

	for (int i = 0; i < job->size; ++i)
		freeCommand(job->commands[i]);

	free(job->commands);
	free(job);
}

struct Jobs* createJobs()
{
	struct Jobs* jobs = malloc(sizeof(struct Jobs));
	jobs->jobs = NULL;
	jobs->size = jobs->capacity = 0;
	return jobs;
}

void freeJobs(struct Jobs* jobs)
{
	if (!jobs)
		return;

	for (int i = 0; i < jobs->size; ++i)
		freeJob(jobs->jobs[i]);

	free(jobs->jobs);
	jobs->jobs = NULL;
	jobs->size = jobs->capacity = 0;

	free(jobs);
}

void printJobs(const struct Jobs* jobs)
{
	printf("********JOBS*******:\n");

	for (int i = 0; i < jobs->size; ++i)
	{
		printf("JOB%d\n", i + 1);
		struct Job* job = jobs->jobs[i];
		int nCommands = job->size;
		for (int j = 0; j < nCommands; ++j)
		{
			struct Command* command = job->commands[j];
			printf("  cmd: %s\n", command->name);
			printf("  args:\n");
			struct StringArray* args = command->args;
			int nArgs = args->size;
			for (int k = 0; k < nArgs; ++k)
				printf("    %s\n", args->data[k]);

			printf("  input: %s\n", command->input ? command->input : "none");
			printf("  output: %s\n", command->output ? command->output : "none");
			printf("  overwrite: %d\n\n", command->rewriteOutput);
		}
	}

	printf("****END OF JOBS****:\n\n");
}