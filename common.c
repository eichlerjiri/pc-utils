#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <iconv.h>

#if ENABLE_TRACE
#include "trace.c"
#endif

static int return_code;
static const char* errno_msg;

static void verror(const char *msg, va_list valist) {
	fprintf(stderr, "\033[91m");
	vfprintf(stderr, msg, valist);
	fprintf(stderr, "\033[0m\n");
}

static void error(const char *msg, ...) {
	va_list valist;
	va_start(valist, msg);
	verror(msg, valist);
	va_end(valist);

	return_code = 1;
}

static void fatal(const char *msg, ...) {
	va_list valist;
	va_start(valist, msg);
	verror(msg, valist);
	va_end(valist);

	exit(2);
}

static const char *c_strerror(int errnum) {
	if (errnum == 1234) {
		return errno_msg;
	} else {
		return strerror(errnum);
	}
}

static void *c_malloc(size_t size) {
	void *ret = malloc(size);
	if (!ret) {
		fatal("Cannot malloc %lu: %s", size, c_strerror(errno));
	}
#if ENABLE_TRACE
	trace_start("MEM", ret, "malloc");
#endif
	return ret;
}

static void *c_realloc(void *ptr, size_t size) {
	void *ret = realloc(ptr, size);
	if (!ret) {
		fatal("Cannot realloc %li: %s", size, strerror(errno));
	}
#if ENABLE_TRACE
	if (ptr != ret) {
		if (ptr) {
			trace_end("MEM", ptr, "realloc");
		}
		trace_start("MEM", ret, "realloc");
	}
#endif
	return ret;
}

static char *c_asprintf(const char *fmt, ...) {
	va_list valist;
	va_start(valist, fmt);

	char *ret;
	if (vasprintf(&ret, fmt, valist) < 0) {
		fatal("Cannot asprintf %s: %s", fmt, c_strerror(errno));
	}
#if ENABLE_TRACE
	trace_start("MEM", ret, "asprintf");
#endif

	va_end(valist);
	return ret;
}

static char *c_strdup(const char *s) {
	char *ret = strdup(s);
	if (!ret) {
		fatal("Cannot strdup %s", c_strerror(errno));
	}
#if ENABLE_TRACE
	trace_start("MEM", ret, "strdup");
#endif
	return ret;
}

static ssize_t c_getline(char **lineptr, size_t *n, FILE *stream) {
#if ENABLE_TRACE
	char *origptr = *lineptr;
#endif
	errno = 0;
	ssize_t ret = getline(lineptr, n, stream);

	if (ret < 0 && errno == ENOMEM) {
		fatal("Cannot getline: %s", c_strerror(errno));
	} else if (ret > 0) {
		for (char *c = *lineptr; c < *lineptr + ret; c++) {
			if (!*c) {
				errno = 1234;
				errno_msg = "Input contains NULL byte";
				return -1;
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

static ssize_t ce_getline(char **lineptr, size_t *n, FILE *stream) {
	ssize_t ret = c_getline(lineptr, n, stream);
	if (ret < 0 && errno) {
		fatal("Cannot getline: %s", c_strerror(errno));
	}
	return ret;
}

static void c_free(void *ptr) {
#if ENABLE_TRACE
	if (ptr) {
		trace_end("MEM", ptr, "free");
	}
#endif
	free(ptr);
}

static int c_fork() {
	int ret = fork();
	if (ret < 0) {
		fatal("Cannot fork: %s", c_strerror(errno));
	}
	return ret;
}

static void c_waitpid(pid_t pid) {
	int status;
	if (waitpid(pid, &status, 0) <= 0) {
		fatal("Cannot waitpid: %s", c_strerror(errno));
	}
	if (status) {
		fatal("Subprocess exited with code %i", status);
	}
}

static void c_execvp_wait(const char *path, char *const argv[]) {
	int pid = c_fork();
	if (pid) {
		c_waitpid(pid);
	} else {
		execvp(path, argv);
		fatal("Cannot execvp %s: %s", path, c_strerror(errno));
	}
}

static int c_iconv_close(iconv_t cd) {
	int ret = iconv_close(cd);
	if (ret) {
		fatal("Cannot iconv_close: %s", c_strerror(errno));
	}
	return ret;
}

static iconv_t c_iconv_open(const char *to, const char *from) {
	iconv_t ret = iconv_open(to, from);
	if (ret == (iconv_t) -1) {
		fatal("Cannot iconv_open %s => %s: %s", from, to, c_strerror(errno));
	}
	return ret;
}

static char *c_iconv(const char *from, const char *to, char *input) {
	size_t inbytesleft = strlen(input);
	if (SIZE_MAX / 8 < inbytesleft) { // overflow check
		errno = 1234;
		errno_msg = "Input is too long";
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
		fatal("Cannot iconv: %s", c_strerror(errno));
	}
	c_iconv_close(cd);

	*outbuf = '\0';
	return out;
}

static int c_mkstemps(char *template, int suffixlen) {
	int ret = mkstemps(template, suffixlen);
	if (!ret) {
		fatal("Cannot mkstemps: %s", c_strerror(errno));
	}
	return ret;
}

static FILE *c_fopen(const char *pathname, const char *mode) {
	FILE *ret = fopen(pathname, mode);
	if (!ret) {
		fatal("Cannot fopen: %s", c_strerror(errno));
	}
	return ret;
}

static FILE *c_fdopen(int fd, const char *mode) {
	FILE *ret = fdopen(fd, mode);
	if (!ret) {
		fatal("Cannot fdopen: %s", c_strerror(errno));
	}
	return ret;
}

static void c_fclose(FILE *stream) {
	if (fclose(stream)) {
		fatal("Cannot fclose: %s", c_strerror(errno));
	}
}

static int c_fprintf(FILE *stream, const char *format, ...) {
	va_list valist;
	va_start(valist, format);
	int ret = vfprintf(stream, format, valist);
	va_end(valist);

	if (ret < 0) {
		fatal("Cannot fprintf: %s", c_strerror(errno));
	}
	return ret;
}

static void c_remove(const char *pathname) {
	if (remove(pathname)) {
		fatal("Cannot remove %s: %s", pathname, c_strerror(errno));
	}
}
