#ifndef COMMANDS_H
#define COMMANDS_H

#include "utils.h"

int cd(int argc, char** argv);
int pwd(int argc, char** argv);
int printHistory(const struct StringArray* history);

#endif
