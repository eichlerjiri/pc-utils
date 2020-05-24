static char *strdup_safe(const char *s) {
	char *ret = strdup(s);
	if (!ret) {
		fprintf(stderr, "Error allocating memory: %s\n", strerror(errno));
		exit(3);
	}
	return ret;
}
