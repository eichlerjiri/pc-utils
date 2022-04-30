__attribute__((format(printf, 1, 2)))
static void trace_printf(const char *format, ...) {
	/*va_list valist;
	va_start(valist, format);
	vfprintf(stderr, format, valist);
	va_end(valist);*/
}

__attribute__((format(printf, 1, 2)))
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

static size_t write_safe(const void *ptr, size_t size, size_t nmemb) {
	size_t ret = fwrite(ptr, size, nmemb, stdout);
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

static int flush_safe() {
	int ret = fflush(stdout);
	if (ret) {
		fprintf(stderr, "Write error: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static int fileno_safe(FILE *stream) {
	int ret = fileno(stream);
	if (ret == -1) {
		fprintf(stderr, "Error fileno: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}
