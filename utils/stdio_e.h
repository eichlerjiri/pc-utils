static int printf_e(const char *format, ...) {
	va_list valist;
	va_start(valist, format);
	int ret = vprintf(format, valist);
	va_end(valist);
	if (ret < 0) {
		fprintf(stderr, "Error printf\n");
		exit(3);
	}
	return ret;
}

static size_t fwrite_e(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
	size_t ret = fwrite(ptr, size, nmemb, stream);
	if (ret != nmemb) {
		fprintf(stderr, "Error fwrite\n");
		exit(3);
	}
	return ret;
}

static int putchar_e(int c) {
	int ret = putchar(c);
	if (ret == -1) {
		fprintf(stderr, "Error putchar\n");
		exit(3);
	}
	return ret;
}

static int fprintf_e(FILE *stream, const char *format, ...) {
	va_list valist;
	va_start(valist, format);
	int ret = vfprintf(stream, format, valist);
	va_end(valist);
	if (ret < 0) {
		fprintf(stderr, "Error fprintf\n");
		exit(3);
	}
	return ret;
}

static ssize_t getline_e(char **lineptr, size_t *n, FILE *stream) {
	errno = 0;
	ssize_t ret = getline(lineptr, n, stream);
	if (ret == -1 && errno) {
		fprintf(stderr, "Error getline: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static ssize_t getline_em(char **lineptr, size_t *n, FILE *stream) {
	errno = 0;
	ssize_t ret = getline(lineptr, n, stream);
	if (ret == -1 && errno == ENOMEM) {
		fprintf(stderr, "Error getline: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static int fflush_e(FILE *stream) {
	int ret = fflush(stream);
	if (ret) {
		fprintf(stderr, "Error fflush: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static FILE *fopen_e(const char *pathname, const char *mode) {
	FILE *ret = fopen(pathname, mode);
	if (!ret) {
		fprintf(stderr, "Error fopen: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static FILE *fdopen_e(int fd, const char *mode) {
	FILE *ret = fdopen(fd, mode);
	if (!ret) {
		fprintf(stderr, "Error fdopen: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static int fclose_e(FILE *stream) {
	int ret = fclose(stream);
	if (ret) {
		fprintf(stderr, "Error fclose: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static int remove_e(const char *pathname) {
	int ret = remove(pathname);
	if (ret) {
		fprintf(stderr, "Error remove: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

#ifdef _GNU_SOURCE
static int asprintf_e(char **strp, const char *fmt, ...) {
	va_list valist;
	va_start(valist, fmt);
	int ret = vasprintf(strp, fmt, valist);
	va_end(valist);
	if (ret < 0) {
		fprintf(stderr, "Error asprintf: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}
#endif
