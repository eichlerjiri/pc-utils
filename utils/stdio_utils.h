static int printf_safe(const char *format, ...) {
	va_list valist;
	va_start(valist, format);
	int ret = vprintf(format, valist);
	va_end(valist);
	if (ret < 0) {
		fprintf(stderr, "Write error: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static size_t fwrite_safe(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
	size_t ret = fwrite(ptr, size, nmemb, stream);
	if (ret != nmemb && (ret != 0 || size != 0)) {
		fprintf(stderr, "Write error: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static int putchar_safe(int c) {
	int ret = putchar(c);
	if (ret == -1) {
		fprintf(stderr, "Write error: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static int fprintf_safe(FILE *stream, const char *format, ...) {
	va_list valist;
	va_start(valist, format);
	int ret = vfprintf(stream, format, valist);
	va_end(valist);
	if (ret < 0) {
		fprintf(stderr, "Write error: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static int fflush_safe(FILE *stream) {
	int ret = fflush(stream);
	if (ret) {
		fprintf(stderr, "Write error: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static size_t getline_no_eol(char **lineptr, size_t *n, FILE *stream) {
	size_t ret = (size_t) getline(lineptr, n, stream);
	if (ret != (size_t) -1 && (*lineptr)[ret - 1] == '\n') {
		(*lineptr)[--ret] = '\0';
	}
	return ret;
}

static size_t getline_no_eol_safe(char **lineptr, size_t *n, FILE *stream) {
	size_t ret = (size_t) getline_no_eol(lineptr, n, stream);
	if (ret == (size_t) -1 && !feof(stream)) {
		fprintf(stderr, "Read error: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static FILE *fopen_safe(const char *pathname, const char *mode) {
	FILE *ret = fopen(pathname, mode);
	if (!ret) {
		fprintf(stderr, "Open file error: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static FILE *fdopen_safe(int fd, const char *mode) {
	FILE *ret = fdopen(fd, mode);
	if (!ret) {
		fprintf(stderr, "Open file error: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static int fclose_safe(FILE *stream) {
	int ret = fclose(stream);
	if (ret) {
		fprintf(stderr, "Close file error: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static int remove_safe(const char *pathname) {
	int ret = remove(pathname);
	if (ret) {
		fprintf(stderr, "Remove file error: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}
