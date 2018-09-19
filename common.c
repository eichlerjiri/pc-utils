#include "common.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

void verror(const char *msg, va_list valist) {
	fprintf(stderr, "\033[91m");
	vfprintf(stderr, msg, valist);
	fprintf(stderr, "\033[0m\n");
}

void error(const char *msg, ...) {
	va_list valist;
	va_start(valist, msg);
	verror(msg, valist);
	va_end(valist);
}

void fatal(const char *msg, ...) {
	va_list valist;
	va_start(valist, msg);
	verror(msg, valist);
	va_end(valist);

	exit(1);
}

char* asprintfx(const char *fmt, ...) {
	va_list valist;
	va_start(valist, fmt);

	char *ret;
	if (vasprintf(&ret, fmt, valist) < 0) {
		fatal("Cannot asprintf %s: %s", fmt, strerror(errno));
	}

	va_end(valist);
	return ret;
}

int forkx() {
	int ret = fork();
	if (ret < 0) {
		fatal("Cannot fork: %s", strerror(errno));
	}
	return ret;
}

void waitpidx(pid_t pid) {
	int status;
	if (waitpid(pid, &status, 0) <= 0) {
		fatal("Cannot waitpid: %s", strerror(errno));
	}
	if (status) {
		fatal("Subprocess exited with code %i", status);
	}
}

void execvpx_wait(const char *path, char *const argv[]) {
	int pid = forkx();
	if (pid) {
		waitpidx(pid);
	} else {
		execvp(path, argv);
		fatal("Cannot execvp %s: %s", path, strerror(errno));
	}
}

FILE *fopenx(const char *pathname, const char *mode) {
	FILE *ret = fopen(pathname, mode);
	if (!ret) {
		fatal("Cannot fopen %s: %s", pathname, strerror(errno));
	}
	return ret;
}

size_t freadx(void *ptr, size_t size, size_t nmemb, FILE *stream, const char *pathname) {
	size_t ret = fread(ptr, size, nmemb, stream);
	if (!ret && ferror(stream)) {
		fatal("Cannot fread %s: %s", pathname, strerror(errno));
	}
	return ret;
}

void fclosex(FILE *stream, const char *pathname) {
	if (fclose(stream)) {
		fatal("Cannot fclose %s: %s", pathname, strerror(errno));
	}
}

int statx(const char *pathname, struct stat *statbuf) {
	int ret = stat(pathname, statbuf);
	if (ret) {
		error("Cannot stat %s: %s", pathname, strerror(errno));
	}
	return ret;
}

DIR *opendirx(const char *name) {
	DIR *ret = opendir(name);
	if (!ret) {
		error("Cannot opendir %s: %s", name, strerror(errno));
	}
	return ret;
}

struct dirent *readdirx(DIR *dirp, const char *name) {
	errno = 0;
	struct dirent *ret = readdir(dirp);
	if (!ret && errno) {
		error("Cannot readdir %s: %s", name, strerror(errno));
	}
	return ret;
}

void closedirx(DIR *dirp, const char *name) {
	if (closedir(dirp)) {
		error("Cannot closedir %s: %s", name, strerror(errno));
	}
}

int mkdirx(const char *pathname, mode_t mode) {
	int ret = mkdir(pathname, mode);
	if (ret && errno != EEXIST) {
		error("Cannot mkdir %s: %s", pathname, strerror(errno));
		return ret;
	}
	return 0;
}
