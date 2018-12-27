#include "utils.h"

#include <stdlib.h>
#include <string.h>

extern struct _IO_FILE* ERROR_OUTPUT;

#define MIN_MEMORY_ALLOCATION_SIZE 1024
#define MAX_MEMORY_GROW_SIZE 65536
#define MEMORY_GROWTH_FACTOR 1.5

#define MIN_SIZE 8
#define MAX_GROW_SIZE 64
#define GROWTH_FACTOR 1.5

int min(int a, int b)
{
	return a < b ? a : b;
}

int max(int a, int b)
{
	return a > b ? a : b;
}

char* duplicateString(const char* str)
{
	if (!str)
		return NULL;

	size_t size = strlen(str);
	char* ret = malloc(size + 1);
	for (int i = 0; i < size; ++i)
		ret[i] = str[i];

	ret[size] = '\0';
	return ret;
}

struct String* createString()
{
	struct String* ret = malloc(sizeof(struct String));
	size_t capacity = MIN_SIZE;
	ret->data = malloc(capacity);
	memset(ret->data, 0, capacity);
	ret->capacity = (int)capacity;
	ret->size = 0;
	return ret;
}

void emptyString(struct String* s)
{
	memset(s->data, 0, (size_t)s->capacity);
	s->size = 0;
}

void freeString(struct String* s)
{
	if (!s)
		return;

	free(s->data);
	free(s);
}

void addSymbol(struct String* s, char symbol)
{
	if (s->size == s->capacity - 1)
	{
		int newCapacity = min(max((int)(s->capacity * GROWTH_FACTOR), MIN_SIZE), s->capacity + MAX_GROW_SIZE);
		s->data = realloc(s->data, (size_t)newCapacity);
		if (!s->data)
		{
			fprintf(ERROR_OUTPUT, "Application ran out of memory.\n");
			exit(1);
		}

		memset(s->data + s->capacity, 0, (size_t)(newCapacity - s->capacity));
		s->capacity = newCapacity;
	}

	s->data[s->size] = symbol;
	s->size++;
}

struct StringArray* createStringArray()
{
	struct StringArray* ret = malloc(sizeof(struct StringArray));
	ret->data = NULL;
	ret->size = 0;
	ret->capacity = 0;
	return ret;
}

void emptyStringArray(struct StringArray* sa)
{
	if (!sa)
		return;

	for (int i = 0; i < sa->size; ++i)
		free(sa->data[i]);

	memset(sa->data, 0, (size_t)sa->capacity * sizeof(char*));
	sa->size = 0;
}

void freeStringArray(struct StringArray* sa)
{
	if (!sa)
		return;

	for (int i = 0; i < sa->size; ++i)
		free(sa->data[i]);

	free(sa->data);
	free(sa);
}

void addString(struct StringArray* sa, const char* str)
{
	if (!sa || !str)
		return;

	if (sa->size == sa->capacity)
	{
		int newCapacity = min(max((int)(sa->capacity * GROWTH_FACTOR), MIN_SIZE), sa->capacity + MAX_GROW_SIZE);
		sa->data = realloc(sa->data, (size_t)newCapacity * sizeof(char*));
		if (!sa->data)
		{
			fprintf(ERROR_OUTPUT, "Application ran out of memory.\n");
			exit(1);
		}

		memset(sa->data + sa->capacity, 0, (size_t)(newCapacity - sa->capacity));
		sa->capacity = newCapacity;
	}

	sa->data[sa->size] = duplicateString(str);
	sa->size++;
}

struct IntArray* createIntArray()
{
	struct IntArray* ret = malloc(sizeof(struct IntArray));
	ret->data = NULL;
	ret->size = 0;
	ret->capacity = 0;
	return ret;
}

void emptyIntArray(struct IntArray* ia)
{
	if (!ia)
		return;

	memset(ia->data, 0, (size_t)ia->capacity * sizeof(int));
	ia->size = 0;
}

void freeIntArray(struct IntArray* ia)
{
	if (!ia)
		return;

	free(ia->data);
	free(ia);
}

void addInt(struct IntArray* ia, int value)
{
	if (!ia)
		return;

	if (ia->size == ia->capacity)
	{
		int newCapacity = min(max((int)(ia->capacity * GROWTH_FACTOR), MIN_SIZE), ia->capacity + MAX_GROW_SIZE);
		ia->data = realloc(ia->data, (size_t)newCapacity * sizeof(int));
		if (!ia->data)
		{
			fprintf(ERROR_OUTPUT, "Application ran out of memory.\n");
			exit(1);
		}

		memset(ia->data + ia->capacity, 0, (size_t)(newCapacity - ia->capacity));
		ia->capacity = newCapacity;
	}

	ia->data[ia->size] = value;
	ia->size++;
}

int getLine(FILE* file, char** buffer, int* size, int* index)
{
	if (!buffer || !size || !index || (*buffer && *size <= 0))
		return 1;

	if (!*buffer)
	{
		*size = MIN_MEMORY_ALLOCATION_SIZE;
		*buffer = malloc((size_t)*size);
		*index = 0;
	}

	if (*index >= *size)
		return 1;

	int c;
	int escaped = 0;
	while ((c = fgetc(file)) != EOF)
	{
		if (*index == *size - 1)
		{
			*size = min(max((int)(*size * MEMORY_GROWTH_FACTOR), MIN_MEMORY_ALLOCATION_SIZE), *size + MAX_MEMORY_GROW_SIZE);
			char* newBuffer = realloc(*buffer, (size_t)*size);
			if (!newBuffer)
			{
				free(*buffer);
				fprintf(ERROR_OUTPUT, "Application ran out of memory.\n");
				exit(1);
			}

			*buffer = newBuffer;
		}

		if (((*buffer)[(*index)++] = (char)c) == '\n' && !escaped)
		{

			escaped = c == '\\';
			break;
		}

		escaped = c == '\\';
	}

	(*buffer)[*index] = '\0';

	return 0;
}