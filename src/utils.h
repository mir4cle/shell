#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

/* MATH */
int min(int a, int b);
int max(int a, int b);

/* string */
char* duplicateString(const char* str);

struct String
{
	char* data;
	int size;
	int capacity;
};

struct String* createString();
void emptyString(struct String* s);
void freeString(struct String* s);
void addSymbol(struct String* s, char symbol);

/* string array */
struct StringArray
{
	char** data;
	int size;
	int capacity;
};

struct StringArray* createStringArray();
void emptyStringArray(struct StringArray* sa);
void freeStringArray(struct StringArray* sa);
void addString(struct StringArray* sa, const char* str);

/* int array */
struct IntArray
{
	int* data;
	int size;
	int capacity;
};

struct IntArray* createIntArray();
void emptyIntArray(struct IntArray* ia);
void freeIntArray(struct IntArray* ia);
void addInt(struct IntArray* ia, int value);

/* FILE IO*/
int getLine(FILE* file, char** buffer, int* size, int* index);

#endif // UTILS_H
