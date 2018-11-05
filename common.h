#ifndef COMMON_H
#define COMMON_H

#define _GNU_SOURCE
#define _POSIX_C_SOURCE

#include <stdio.h>

extern int return_code;

void error(const char *msg, ...);
void fatal(const char *msg, ...);
void *c_malloc(size_t size);
char* c_asprintf(const char *fmt, ...);
ssize_t c_getline(char **lineptr, size_t *n, FILE *stream);
void c_execvp_wait(const char *path, char *const argv[]);
char *c_iconv(const char *from, const char *to, char *input);

#endif
