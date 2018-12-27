#ifndef JOB_H
#define JOB_H

struct Command
{
	char* name;
	struct StringArray* args;

	char* input;
	char* output;
	int rewriteOutput;
};

struct Job
{
	struct Command** commands;
	int size;
	int capacity;
};

struct Jobs
{
	struct Job** jobs;
	int size;
	int capacity;
};

struct Command* createCommand();
void freeCommand(struct Command* command);
void addCommand(struct Job* job, struct Command* command);

struct Job* createJob();
void addJob(struct Jobs* jobs, struct Job* job);
void freeJob(struct Job* job);

struct Jobs* createJobs();
void freeJobs(struct Jobs* jobs);
void printJobs(const struct Jobs* jobs);

#endif