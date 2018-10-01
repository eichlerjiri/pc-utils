#ifndef COMMON_H
#define COMMON_H

#define _GNU_SOURCE

extern int return_code;

void error(const char *msg, ...);
void fatal(const char *msg, ...);
char* asprintfx(const char *fmt, ...);
void execvpx_wait(const char *path, char *const argv[]);

#endif
