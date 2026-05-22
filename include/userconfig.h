#ifndef USERCONFIG_H
#define USERCONFIG_H

#include <stddef.h>

char *UserConfigAuthPathGet(void);
int UserConfigAuthLoad(char **username, char **password, char *error, size_t errorSize);

#endif