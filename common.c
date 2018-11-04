#include "common.h"
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <iconv.h>

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

void *mallocx(size_t size) {
	void *ret = malloc(size);
	if (!ret) {
		fatal("Cannot malloc %lu: %s", size, strerror(errno));
	}
	return ret;
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

ssize_t getlinex(char **lineptr, size_t *n, FILE *stream) {
	errno = 0;
	ssize_t ret = getline(lineptr, n, stream);

	if (ret < 0 && errno == ENOMEM) {
		fatal("Cannot getline: %s", strerror(errno));
	} else if (ret > 0) {
		for (char *c = *lineptr; c < *lineptr + ret; c++) {
			if (!*c) {
				errno = EILSEQ;
				return -2;
			}
		}
	}

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

int iconv_closex(iconv_t cd) {
	int ret = iconv_close(cd);
	if (ret) {
		fatal("Cannot iconv_close: %s", strerror(errno));
	}
	return ret;
}

char *iconvx(const char *from, const char *to, char *input) {
	iconv_t cd = iconv_open(to, from);
	if (cd == (iconv_t) -1) {
		fatal("Cannot iconv_open %s => %s: %s", from, to, strerror(errno));
	}

	size_t inbytesleft = strlen(input);
	size_t outbytesleft = inbytesleft * 4;
	char *out = mallocx(outbytesleft + 1);

	char *inbuf = input;
	char *outbuf = out;

	if (iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t) -1) {
		if (errno == EILSEQ || errno == EINVAL) {
			iconv_closex(cd);
			free(out);
			return NULL;
		}
		fatal("Cannot iconv: %s", strerror(errno));
	}

	iconv_closex(cd);
	*outbuf = '\0';
	return out;
}
