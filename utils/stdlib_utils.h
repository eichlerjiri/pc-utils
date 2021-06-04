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
