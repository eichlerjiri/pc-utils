static void *malloc_e(size_t size) {
	void *ret = malloc(size);
	if (!ret) {
		fprintf(stderr, "Error malloc: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static void *realloc_e(void *ptr, size_t size) {
	void *ret = realloc(ptr, size);
	if (!ret) {
		fprintf(stderr, "Error realloc: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}

static int mkstemps_e(char *template, int suffixlen) {
	int ret = mkstemps(template, suffixlen);
	if (!ret) {
		fprintf(stderr, "Error mkstemps: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}
