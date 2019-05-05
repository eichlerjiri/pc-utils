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
#include <regex.h>
#include <dirent.h>
#include <sys/stat.h>

#if ENABLE_TRACE
#include "trace.c"
#endif

static void fatal(const char *msg, ...) {
	va_list valist;
	va_start(valist, msg);
	fprintf(stderr, "\033[91m");
	vfprintf(stderr, msg, valist);
	fprintf(stderr, "\033[0m\n");
	va_end(valist);

	exit(1);
}

static void *c_malloc(size_t size) {
	void *ret = malloc(size);
	if (!ret) {
		fatal("Cannot malloc %lu: %s", size, strerror(errno));
	}
#if ENABLE_TRACE
	trace_start("MEM", ret, "malloc");
#endif
	return ret;
}

static void *c_realloc(void *ptr, size_t size) {
	void *ret = realloc(ptr, size);
	if (!ret) {
		fatal("Cannot realloc %lu: %s", size, strerror(errno));
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
		fatal("Cannot asprintf: %s", strerror(errno));
	}
	va_end(valist);
#if ENABLE_TRACE
	trace_start("MEM", ret, "asprintf");
#endif
	return ret;
}

static char *c_strdup(const char *s) {
	char *ret = strdup(s);
	if (!ret) {
		fatal("Cannot strdup: %s", strerror(errno));
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
	if (ret == -1 && errno) {
		fatal("Cannot getline: %s", strerror(errno));
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

static ssize_t c_getline_nonull(char **lineptr, size_t *n, FILE *stream) {
	ssize_t ret = c_getline(lineptr, n, stream);
	if (ret != -1 && ret != strlen(*lineptr)) {
		fatal("Cannot getline: Line contains NULL byte");
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
		fatal("Cannot fork: %s", strerror(errno));
	}
	return ret;
}

static void c_waitpid(pid_t pid) {
	int status;
	if (waitpid(pid, &status, 0) <= 0) {
		fatal("Cannot waitpid: %s", strerror(errno));
	}
	if (status) {
		fatal("Subprocess exited with code %i", status);
	}
}

static void c_execvp(const char *file, char *const argv[]) {
	execvp(file, argv);
	fatal("Cannot execvp %s: %s", file, strerror(errno));
}

static iconv_t c_iconv_open(const char *tocode, const char *fromcode) {
	iconv_t ret = iconv_open(tocode, fromcode);
	if (ret == (iconv_t) -1) {
		fatal("Cannot iconv_open %s => %s: %s", fromcode, tocode, strerror(errno));
	}
#if ENABLE_TRACE
	trace_start("ICONV", ret, "iconv_open");
#endif
	return ret;
}

static size_t c_iconv_tryin(iconv_t cd, char *inbuf, size_t inbytesleft, char *outbuf, size_t outbytesleft) {
	char *out = outbuf;
	size_t ret = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
	if (ret == (size_t) -1) {
		if (errno == EILSEQ || errno == EINVAL) {
			return (size_t) -1;
		} else {
			fatal("Cannot iconv: %s", strerror(errno));
		}
	}
	return (size_t) (outbuf - out);
}

static int c_iconv_close(iconv_t cd) {
#if ENABLE_TRACE
	trace_end("ICONV", cd, "iconv_close");
#endif
	int ret = iconv_close(cd);
	if (ret) {
		fatal("Cannot iconv_close: %s", strerror(errno));
	}
	return ret;
}

static int c_mkstemps(char *template, int suffixlen) {
	int ret = mkstemps(template, suffixlen);
	if (!ret) {
		fatal("Cannot mkstemps %s: %s", template, strerror(errno));
	}
	return ret;
}

static FILE *c_fopen(const char *pathname, const char *mode) {
	FILE *ret = fopen(pathname, mode);
	if (!ret) {
		fatal("Cannot fopen %s: %s", pathname, strerror(errno));
	}
#if ENABLE_TRACE
	trace_start("FILE", ret, "fopen");
#endif
	return ret;
}

static FILE *c_fdopen(int fd, const char *mode) {
	FILE *ret = fdopen(fd, mode);
	if (!ret) {
		fatal("Cannot fdopen: %s", strerror(errno));
	}
#if ENABLE_TRACE
	trace_start("FILE", ret, "fdopen");
#endif
	return ret;
}

static void c_fclose(FILE *stream) {
#if ENABLE_TRACE
	trace_end("FILE", stream, "fclose");
#endif
	if (fclose(stream)) {
		fatal("Cannot fclose: %s", strerror(errno));
	}
}

static int c_fprintf(FILE *stream, const char *format, ...) {
	va_list valist;
	va_start(valist, format);
	int ret = vfprintf(stream, format, valist);
	va_end(valist);
	if (ret < 0) {
		fatal("Cannot fprintf: %s", strerror(errno));
	}
	return ret;
}

static void c_remove(const char *pathname) {
	if (remove(pathname)) {
		fatal("Cannot remove %s: %s", pathname, strerror(errno));
	}
}

static void c_regcomp(regex_t *preg, const char *regex) {
	int code = regcomp(preg, regex, REG_EXTENDED);
	if (code) {
		char buffer[256];
		regerror(code, preg, buffer, sizeof(buffer));
		fatal("Cannot compile regex %s: %s", regex, buffer);
	}
#if ENABLE_TRACE
	trace_start("REGEX", preg, "regcomp");
#endif
}

static void c_regfree(regex_t *preg) {
#if ENABLE_TRACE
	trace_end("REGEX", preg, "regfree");
#endif
	regfree(preg);
}

static size_t c_fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	size_t ret = fread(ptr, size, nmemb, stream);
	if (ret != nmemb && !feof(stream)) {
		fatal("Cannot fread: %s", strerror(errno));
	}
	return ret;
}

static size_t c_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
	size_t ret = fwrite(ptr, size, nmemb, stream);
	if (ret != nmemb) {
		fatal("Cannot fwrite: %s", strerror(errno));
	}
	return ret;
}

static int c_printf(const char *format, ...) {
	va_list valist;
	va_start(valist, format);
	int ret = vprintf(format, valist);
	va_end(valist);
	if (ret < 0) {
		fatal("Cannot printf: %s", strerror(errno));
	}
	return ret;
}

static int c_fflush(FILE *stream) {
	int ret = fflush(stream);
	if (ret) {
		fatal("Cannot fflush: %s", strerror(errno));
	}
	return ret;
}

static int c_putchar(int c) {
	int ret = putchar(c);
	if (ret == -1) {
		fatal("Cannot putchar: %s", strerror(errno));
	}
	return ret;
}

static int c_rename(const char *old, const char *new) {
	int ret = rename(old, new);
	if (ret) {
		fatal("Cannot rename %s => %s: %s", old, new, strerror(errno));
	}
	return ret;
}

static int c_mkdir_tryexist(const char *path, mode_t mode) {
	int ret = mkdir(path, mode);
	if (ret && errno != EEXIST) {
		fatal("Cannot create directory %s: %s", path, strerror(errno));
	}
	return ret;
}

static int c_stat(const char *pathname, struct stat *statbuf) {
	int ret = stat(pathname, statbuf);
	if (ret) {
		fatal("Cannot stat %s: %s", pathname, strerror(errno));
	}
	return ret;
}

static DIR *c_opendir(const char *name) {
	DIR *ret = opendir(name);
	if (!ret) {
		fatal("Cannot open dir %s: %s", name, strerror(errno));
	}
#if ENABLE_TRACE
	trace_start("DIR", ret, "opendir");
#endif
	return ret;
}

static struct dirent *c_readdir(DIR *dirp) {
	errno = 0;
	struct dirent *ret = readdir(dirp);
	if (!ret && errno) {
		fatal("Cannot read dir: %s", strerror(errno));
	}
	return ret;
}

static int c_closedir(DIR *dirp) {
	int ret = closedir(dirp);
	if (ret) {
		fatal("Cannot close dir: %s", strerror(errno));
	}
#if ENABLE_TRACE
	trace_end("DIR", dirp, "closedir");
#endif
	return ret;
}
