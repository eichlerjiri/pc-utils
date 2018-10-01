#include "common.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int return_code;

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

	return_code = 1;
}

void fatal(const char *msg, ...) {
	va_list valist;
	va_start(valist, msg);
	verror(msg, valist);
	va_end(valist);

	exit(2);
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
