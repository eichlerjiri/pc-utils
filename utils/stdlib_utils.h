static void *malloc_safe(size_t size) {
	void *ret = malloc(size);
	if (!ret) {
		fprintf(stderr, "Error allocating memory: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static void *realloc_safe(void *ptr, size_t size) {
	void *ret = realloc(ptr, size);
	if (!ret) {
		fprintf(stderr, "Error allocating memory: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static int mkstemps_safe(char *template, int suffixlen) {
	int ret = mkstemps(template, suffixlen);
	if (!ret) {
		fprintf(stderr, "Error making temporary file: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}
