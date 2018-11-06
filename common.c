#include "common.h"
#if ENABLE_TRACE
#include "trace.h"
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
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

void *c_malloc(size_t size) {
	void *ret = malloc(size);
	if (!ret) {
		fatal("Cannot malloc %lu: %s", size, strerror(errno));
	}
#if ENABLE_TRACE
	trace_start("MEM", ret, "malloc");
#endif
	return ret;
}

char *c_asprintf(const char *fmt, ...) {
	va_list valist;
	va_start(valist, fmt);

	char *ret;
	if (vasprintf(&ret, fmt, valist) < 0) {
		fatal("Cannot asprintf %s: %s", fmt, strerror(errno));
	}
#if ENABLE_TRACE
	trace_start("MEM", ret, "asprintf");
#endif

	va_end(valist);
	return ret;
}

char *c_strdup(const char *s) {
	char *ret = strdup(s);
	if (!ret) {
		fatal("Cannot strdup %s", strerror(errno));
	}
#if ENABLE_TRACE
	trace_start("MEM", ret, "strdup");
#endif
	return ret;
}

ssize_t c_getline(char **lineptr, size_t *n, FILE *stream) {
#if ENABLE_TRACE
	char *origptr = *lineptr;
#endif
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
#if ENABLE_TRACE
	if (origptr != *lineptr) {
		if (origptr) {
			trace_end("MEM", origptr, "getline");
		}
		trace_start("MEM", *lineptr, "getline");
	}
#endif

	return ret;
}

void c_free(void *ptr) {
#if ENABLE_TRACE
	if (ptr) {
		trace_end("MEM", ptr, "free");
	}
#endif
	free(ptr);
}

int c_fork() {
	int ret = fork();
	if (ret < 0) {
		fatal("Cannot fork: %s", strerror(errno));
	}
	return ret;
}

void c_waitpid(pid_t pid) {
	int status;
	if (waitpid(pid, &status, 0) <= 0) {
		fatal("Cannot waitpid: %s", strerror(errno));
	}
	if (status) {
		fatal("Subprocess exited with code %i", status);
	}
}

void c_execvp_wait(const char *path, char *const argv[]) {
	int pid = c_fork();
	if (pid) {
		c_waitpid(pid);
	} else {
		execvp(path, argv);
		fatal("Cannot execvp %s: %s", path, strerror(errno));
	}
}

int c_iconv_close(iconv_t cd) {
	int ret = iconv_close(cd);
	if (ret) {
		fatal("Cannot iconv_close: %s", strerror(errno));
	}
	return ret;
}

iconv_t c_iconv_open(const char *to, const char *from) {
	iconv_t ret = iconv_open(to, from);
	if (ret == (iconv_t) -1) {
		fatal("Cannot iconv_open %s => %s: %s", from, to, strerror(errno));
	}
	return ret;
}

char *c_iconv(const char *from, const char *to, char *input) {
	size_t inbytesleft = strlen(input);
	if (SIZE_MAX / 8 < inbytesleft) { // overflow check
		errno = E2BIG;
		return NULL;
	}

	size_t outbytesleft = inbytesleft * 4;
	char *out = c_malloc(outbytesleft + 1);

	char *inbuf = input;
	char *outbuf = out;

	iconv_t cd = c_iconv_open(to, from);
	if (iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t) -1) {
		if (errno == EILSEQ || errno == EINVAL) {
			c_iconv_close(cd);
			c_free(out);
			return NULL;
		}
		fatal("Cannot iconv: %s", strerror(errno));
	}
	c_iconv_close(cd);

	*outbuf = '\0';
	return out;
}
