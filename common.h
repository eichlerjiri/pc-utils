#ifndef COMMON_H
#define COMMON_H

#define _GNU_SOURCE
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>

#define ERRIF if

void fatal(const char *msg, ...);
char* asprintfx(const char *fmt, ...);
void execvpx_wait(const char *path, char *const argv[]);
FILE *fopenx(const char *pathname, const char *mode);
size_t freadx(void *ptr, size_t size, size_t nmemb, FILE *stream, const char *pathname);
void fclosex(FILE *stream, const char *pathname);
int statx(const char *pathname, struct stat *statbuf);
DIR *opendirx(const char *name);
struct dirent *readdirx(DIR *dirp, const char *name);
void closedirx(DIR *dirp, const char *name);
int mkdirx(const char *pathname, mode_t mode);

#endif
