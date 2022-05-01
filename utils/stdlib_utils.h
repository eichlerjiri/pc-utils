static void *malloc_safe(size_t size) {
	void *ret = malloc(size);
	if (!ret) {
		fprintf(stderr, "Error allocating memory: %s\n", strerror(errno));
		exit(3);
	}
	trace_printf("A MEM %p malloc (%lu)\n", ret, size);
	return ret;
}

static void *calloc_safe(size_t nmemb, size_t size) {
	void *ret = calloc(nmemb, size);
	if (!ret) {
		fprintf(stderr, "Error allocating memory: %s\n", strerror(errno));
		exit(3);
	}
	trace_printf("A MEM %p calloc (%lu * %lu)\n", ret, nmemb, size);
	return ret;
}

static void *realloc_safe(void *ptr, size_t size) {
	if (ptr) {
		trace_printf("F MEM %p realloc\n", ptr);
	}
	void *ret = realloc(ptr, size);
	if (!ret) {
		fprintf(stderr, "Error allocating memory: %s\n", strerror(errno));
		exit(3);
	}
	trace_printf("A MEM %p realloc (%lu)\n", ret, size);
	return ret;
}

static void free_trace(void *ptr) {
	if (ptr) {
		trace_printf("F MEM %p free\n", ptr);
	}
	free(ptr);
}

static int mkstemps_trace(char *template, int suffixlen) {
	int ret = mkstemps(template, suffixlen);
	if (ret) {
		trace_printf("A FD %i mkstemps (%s)\n", ret, template);
		trace_printf("A DISKFILE %s mkstemps\n", template);
	}
	return ret;
}
