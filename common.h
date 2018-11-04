#ifndef COMMON_H
#define COMMON_H

#define _GNU_SOURCE
#define _POSIX_C_SOURCE

#include <stdio.h>

extern int return_code;

void error(const char *msg, ...);
void fatal(const char *msg, ...);
void *mallocx(size_t size);
char* asprintfx(const char *fmt, ...);
ssize_t getlinex(char **lineptr, size_t *n, FILE *stream);
void execvpx_wait(const char *path, char *const argv[]);
char *iconvx(const char *from, const char *to, char *input);

#endif
